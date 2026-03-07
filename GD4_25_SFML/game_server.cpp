#include "game_server.hpp"
#include "network_protocol.hpp"
#include "tank_type.hpp"
#include "data_tables.hpp"
#include "action.hpp"
#include "utility.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include "pickup_type.hpp"
#include <iostream>

namespace
{
	const std::vector<TankData> TankTable = InitializeTankData();
}

// important to set to non-blocking otherwise server will hang waiting to read input from connection
GameServer::RemotePeer::RemotePeer()
	: m_ready(false)
	, m_timed_out(false)
{
	m_socket.setBlocking(false);
}

GameServer::GameServer(sf::Vector2f battlefieldSize)
	: m_thread(&GameServer::ExecutionThread, this)
	, m_listening_state(false)
	, m_client_timeout(sf::seconds(5.f))
	, m_max_connected_players(4)
	, m_connected_players(0)
	, m_game_started(false)
	, m_selected_map(0)
	, m_battlefield_size(battlefieldSize)
	, m_tank_identifier_counter(1)
	, m_waiting_thread_end(false)
	, m_peers(1)
{
	m_listener_socket.setBlocking(false);
	m_peers[0].reset(new RemotePeer); //reservation for server itself
}

GameServer::~GameServer()
{
	m_waiting_thread_end = true;
	m_thread.join();
}

void GameServer::SetListening(bool enable)
{
	if (enable)
	{
		if (!m_listening_state)
		{
			m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::Socket::Status::Done);
		}
	}
	else
	{
		m_listener_socket.close();
		m_listening_state = false;
	}
}

void GameServer::ExecutionThread()
{
	SetListening(true);

	sf::Time stepInterval = sf::seconds(1.f / 60.f); //60 updates per second
	sf::Time stepTime = sf::Time::Zero;
	sf::Time tickInterval = sf::seconds(1.f / 20.f); // 20 updates per second for network updates
	sf::Time tickTime = sf::Time::Zero;

	sf::Clock stepClock;
	sf::Clock tickClock;

	while (!m_waiting_thread_end)
	{
		HandleIncomingConnections();
		HandleIncomingPackets();

		stepTime += stepClock.restart();
		tickTime += tickClock.restart();

		//physics updates
		while (stepTime >= stepInterval)
		{
			if (m_game_started)
			{
				Tick();
			}
			stepTime -= stepInterval;
		}

		while (tickTime >= tickInterval)
		{
			if (m_game_started)
			{
				UpdateClientState();
			}
			tickTime -= tickInterval;
		}
		// to allow some breathing room for the thread and to prevent it from consuming 100% cpu
		sf::sleep(sf::milliseconds(1));
	}
}

void GameServer::Tick()
{
	const float dt = 1.f / 60.f;

	//Server will be the authoritative for the game, not clients, so i update the game logic physics here and send it to clients in the next tick
	for (auto& pair : m_tank_info)
	{
		TankInfo& info = pair.second;
		// get the tank data based on chosen tanks type
		const TankData& stats = TankTable[info.m_tank_type];

		sf::Vector2f direction(0.f, 0.f);
		float currentSpeed = stats.m_speed;

		//check sprinting logic and apply based on stats for tank type 
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kSprint)] && info.stamina > 0.f)
		{
			currentSpeed *= stats.m_sprint_multiplier;
			info.stamina -= stats.m_drain_rate * dt;
		}
		else
		{
			//just regenerate the stamina based on tank type stats
			info.stamina = std::min(stats.m_max_stamina, info.stamina + stats.m_recharge_rate * dt);
		}

		//Movement logic 
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveLeft)]) direction.x -= 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveRight)]) direction.x += 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveUp)]) direction.y -= 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveDown)]) direction.y += 1.f;

		if (direction.x != 0.f || direction.y != 0.f)
		{
			float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			direction /= len;

			info.m_rotation = (Utility::ToDegrees(std::atan2(direction.y, direction.x)) + 90.f);
		}
		
		//aply movement 
		info.m_position += direction * currentSpeed * dt;

		//keep inside the arena
		info.m_position.x = std::max(0.f, std::min(info.m_position.x, m_battlefield_size.x));
		info.m_position.y = std::max(0.f, std::min(info.m_position.y, m_battlefield_size.y));
	}
}

sf::Time GameServer::Now() const
{
	return m_clock.getElapsedTime();
}

void GameServer::HandleIncomingConnections()
{
	if (!m_listening_state) return; //if we are not listenin just return 

	// For my type of game I don't want anyone to be able to join after the game started as it is PVP kind of game and joining mid game would be unfair 
	if (m_game_started)
	{
		sf::TcpSocket temporary_socket;
		//letting temporary socket fall out of scope will get it destroyed and automatically disconnect late joiner
		m_listener_socket.accept(temporary_socket);
		return;
	}
	
	// handle new connections if there is room in the lobby
	if (m_connected_players < m_max_connected_players)
	{
		if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::Socket::Status::Done)
		{
			uint8_t tankID = m_tank_identifier_counter++;

			m_tank_info[tankID].m_tank_type = static_cast<uint8_t>(TankType::kTank1);
			m_tank_info[tankID].m_is_ready = false;
			m_tank_info[tankID].m_position = sf::Vector2f(0.f, 0.f);

			// CRITICAL LINE - MUST be here before sending packets
			m_peers[m_connected_players]->m_tank_identifiers.push_back(tankID);
			m_peers[m_connected_players]->m_ready = true;
			m_peers[m_connected_players]->m_last_packet_time = Now();

			sf::Packet selfPacket;
			selfPacket << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
			selfPacket << tankID << 0.f << 0.f;

			InformWorldState(m_peers[m_connected_players]->m_socket);
			m_peers[m_connected_players]->m_socket.send(selfPacket);

			m_connected_players++;

			if (m_connected_players < m_max_connected_players)
			{
				m_peers.emplace_back(PeerPtr(new RemotePeer()));
			}
			else
			{
				SetListening(false);
			}
			BroadcastLobbyUpdate();
			BroadcastMessage("A new challenger has entered the lobby!");
		}
	}

}

void GameServer::SendToAll(sf::Packet& packet)
{
	for (auto& peer : m_peers)
	{
		if (peer->m_ready)
		{
			peer->m_socket.send(packet);
		}
	}
}

void GameServer::BroadcastMessage(const std::string& message)
{
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kBroadcastMessage) << message;
	SendToAll(packet);
}

void GameServer::HandleIncomingPackets()
{
	bool detected_timeout = false;

	for (PeerPtr& peer : m_peers)
	{
		if (peer->m_ready)
		{
			sf::Packet packet;
			while (peer->m_socket.receive(packet) == sf::Socket::Status::Done)
			{
				HandleIncomingPackets(packet, *peer, detected_timeout);
				peer->m_last_packet_time = Now();
			}

			if (Now() >= peer->m_last_packet_time + m_client_timeout)
			{
				peer->m_timed_out = true;
				detected_timeout = true;
			}
		}
	}

	if (detected_timeout) { HandleDisconnections(); }
}

void GameServer::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
	uint8_t packetType;
	packet >> packetType;

	switch (static_cast<Client::PacketType>(packetType))
	{
		case Client::PacketType::kQuit:
		{
			receiving_peer.m_timed_out = true;
			detected_timeout = true;
			break;
		}

		case Client::PacketType::kSelectTank:
		{
			uint8_t tankType;
			packet >> tankType;

			if (tankType >= static_cast<uint8_t>(TankType::kTankCount)) tankType = 0;

			uint8_t id = receiving_peer.m_tank_identifiers[0];
			m_tank_info[id].m_tank_type = tankType;

			// stat from data table
			const TankData& stats = TankTable[tankType];
			m_tank_info[id].m_hitpoints = stats.m_hitpoints;
			m_tank_info[id].stamina = stats.m_max_stamina;
			m_tank_info[id].m_current_ammo = stats.m_ammo_amount;
			m_tank_info[id].m_missile_ammo = 0; //no one starts with missile

			BroadcastLobbyUpdate();
			break;
		}

		case Client::PacketType::kSelectMap:
		{
			uint8_t mapType;
			packet >> mapType;
			m_selected_map = mapType;
			BroadcastLobbyUpdate();
			break;
		}

		case Client::PacketType::kToggleReady:
		{
			uint8_t id = receiving_peer.m_tank_identifiers[0];
			m_tank_info[id].m_is_ready = !m_tank_info[id].m_is_ready;

			BroadcastLobbyUpdate();
			CheckIfAllReady();
			break;
		}

		case Client::PacketType::kPlayerRealtimeChange:
		{
			uint8_t action;
			bool actionEnabled;
			packet >> action >> actionEnabled;

			uint8_t id = receiving_peer.m_tank_identifiers[0];
			m_tank_info[id].m_real_time_actions[action] = actionEnabled;
			break;
		}
		case Client::PacketType::kKeepAlive:
		{
			// Just update the last packet time, do nothing else
			break;
		}
	}
}

void GameServer::BroadcastLobbyUpdate()
{
	std::cout << "[SERVER] BroadcastLobbyUpdate() called. Tank count: " << m_tank_info.size() << std::endl;
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kLobbyUpdate);
	packet << static_cast<uint8_t>(m_selected_map);
	packet << static_cast<uint8_t>(m_tank_info.size());

	for (const auto& pair : m_tank_info)
	{
		std::cout << "  - Tank ID: " << static_cast<int>(pair.first)
			<< ", Type: " << static_cast<int>(pair.second.m_tank_type)
			<< ", Ready: " << (pair.second.m_is_ready ? "Yes" : "No") << std::endl;

		packet << static_cast<uint8_t>(pair.first); // Force ID to 1 byte
		packet << static_cast<uint8_t>(pair.second.m_tank_type);
		packet << pair.second.m_is_ready; // Booleans are safe
	}
	std::cout << "[SERVER] BroadcastLobbyUpdate sent to all ready peers" << std::endl;
	SendToAll(packet);
}

void GameServer::NotifyPlayerSpawn(uint8_t tank_identifier)
{
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
	packet << tank_identifier << m_tank_info[tank_identifier].m_position.x << m_tank_info[tank_identifier].m_position.y;
	SendToAll(packet);
}

void GameServer::HandleDisconnections()
{
	for (auto it = m_peers.begin(); it != m_peers.end(); )
	{
		if ((*it)->m_timed_out)
		{
			for (uint8_t id : (*it)->m_tank_identifiers)
			{
				m_tank_info.erase(id);

				sf::Packet packet;
				packet << static_cast<uint8_t>(Server::PacketType::kPlayerDisconnect) << id;
				SendToAll(packet);
			}

			m_connected_players--;
			BroadcastMessage("A player has disconnected");
			//remove the peer
			it = m_peers.erase(it);

			if (!m_game_started && m_connected_players < m_max_connected_players)
			{
				m_peers.emplace_back(PeerPtr(new RemotePeer()));
				SetListening(true);
				//possible UI update when player left 
				//BroadcastLobbyUpdate();
			}
		}
		else
		{
			++it;
		}
	}

}

void GameServer::InformWorldState(sf::TcpSocket& socket)
{
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kLobbyUpdate);
	packet << m_selected_map;
	packet << static_cast<uint8_t>(m_tank_info.size());

	//loop through existing playuers and send their data 
	for (const auto& pair : m_tank_info)
	{
		packet << static_cast<uint8_t>(pair.first);
		packet << static_cast<uint8_t>(pair.second.m_tank_type);
		packet << pair.second.m_is_ready;
	}

	socket.send(packet);
}

void GameServer::CheckIfAllReady()
{
	if (m_connected_players == 0 || m_game_started) return;

	bool allReady = true;
	for (const auto& pair : m_tank_info)
	{
		if (!pair.second.m_is_ready)
		{
			allReady = false;
			break;
		}
	}
	
	if (allReady)
	{
		m_game_started = true;
		SetListening(false); //locking the lobb	

		sf::Packet packet;
		packet << static_cast<uint8_t>(Server::PacketType::kInitialState);
		packet << m_selected_map;
		packet << static_cast<uint8_t>(m_tank_info.size());

		//spawn in circle around center
		sf::Vector2 center = m_battlefield_size / 2.f;
		float radius = 400.f;
		float angleStep = 360.f / m_tank_info.size();
		int playerIndex = 0;

		for (auto& pair : m_tank_info)
		{
			float radians = Utility::toRadians(angleStep * playerIndex);

			//position
			pair.second.m_position.x = center.x + std::cos(radians) * radius;
			pair.second.m_position.y = center.y + std::sin(radians) * radius;

			//rotation
			pair.second.m_rotation = Utility::toRadians(std::atan2(center.y - pair.second.m_position.y, center.x - pair.second.m_position.x)) + 90.f;

			packet << pair.first;
			packet << pair.second.m_tank_type;
			packet << pair.second.m_position.x;
			packet << pair.second.m_position.y;

			playerIndex++;
		}

		SendToAll(packet);
	}
}

void GameServer::UpdateClientState()
{
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
	packet << static_cast<uint8_t>(m_tank_info.size());

	for (const auto& pair : m_tank_info)
	{
		packet << pair.first;
		packet << pair.second.m_position.x;
		packet << pair.second.m_position.y;
		packet << pair.second.m_rotation;
		packet << pair.second.m_hitpoints;
		packet << pair.second.m_current_ammo;
		packet << pair.second.m_missile_ammo;
		packet << pair.second.stamina;
	}
	SendToAll(packet);
}

void GameServer::NotifyPlayerRealtimeChange(uint8_t tank_identifier, uint8_t action, bool action_enabled)
{
	m_tank_info[tank_identifier].m_real_time_actions[action] = action_enabled;
}

void GameServer::NotifyPlayerEvent(uint8_t tank_identifier, int8_t action)
{

}