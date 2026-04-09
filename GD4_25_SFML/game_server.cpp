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
#include "map_type.hpp"
#include "constants.hpp"
#include "turret_type.hpp"

namespace
{
	const std::vector<TankData> TankTable = InitializeTankData();
	const std::vector<ProjectileData> ProjectileTable = InitializeProjectileData();
	const std::vector<TurretData> TurretTable = InitializeTurretData();
}

// important to set to non-blocking otherwise server will hang waiting to read input from connection
GameServer::RemotePeer::RemotePeer()
	: m_ready(false)
	, m_timed_out(false)
{
	m_socket.setBlocking(false);
}

GameServer::GameServer(sf::Vector2f battlefieldSize)
	: m_listening_state(false)
	, m_client_timeout(sf::seconds(5.f))
	, m_max_connected_players(8)
	, m_connected_players(0)
	, m_game_started(false)
	, m_selected_map(0)
	, m_battlefield_size(battlefieldSize)
	, m_tank_identifier_counter(1)
	, m_waiting_thread_end(false)
	, m_peers(1)
	, m_thread(&GameServer::ExecutionThread, this)
	, m_turret_fire_cooldown(sf::Time::Zero)
	, m_turret_position(battlefieldSize.x / 2.f, battlefieldSize.y / 2.f)
	, m_turret_rotation(0.f)
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

	sf::Time stepInterval = sf::seconds(1.f / kServerPhysicsRate); //60 updates per second
	sf::Time stepTime = sf::Time::Zero;
	sf::Time tickInterval = sf::seconds(1.f / kNetworkUpdateRate); 
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
	const float dt = 1.f / kServerPhysicsRate;
	float damping = 0.6f;
	float frictionFactor = std::exp(-damping * dt * 2.0f);

	//Server will be the authoritative for the game, not clients, so i update the game logic physics here and send it to clients in the next tick
	for (auto& pair : m_tank_info)
	{
		TankInfo& info = pair.second;
		// get the tank data based on chosen tanks type
		const TankData& stats = TankTable[info.m_tank_type];

		sf::Vector2f acceleration(0.f, 0.f);
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveLeft)]) acceleration.x -= 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveRight)]) acceleration.x += 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveUp)]) acceleration.y -= 1.f;
		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kMoveDown)]) acceleration.y += 1.f;

		float currentMaxSpeed = stats.m_speed;

		if (info.m_real_time_actions[static_cast<uint8_t>(Action::kSprint)] && info.stamina > 0.f) {
			currentMaxSpeed *= stats.m_sprint_multiplier;
			info.stamina -= stats.m_drain_rate * dt;
		}
		else 
		{
			info.stamina = std::min(stats.m_max_stamina, info.stamina + stats.m_recharge_rate * dt);
		}


		if (acceleration.x != 0.f || acceleration.y != 0.f) 
		{
			float len = std::sqrt(acceleration.x * acceleration.x + acceleration.y * acceleration.y);
			info.m_velocity += (acceleration / len) * currentMaxSpeed * dt * 15.0f; 
		}

		//apply the friction 
		info.m_velocity *= frictionFactor;
		//apply movement
		info.m_position += info.m_velocity * dt;

		//rotation update
		if (std::abs(info.m_velocity.x) > 0.1f || std::abs(info.m_velocity.y) > 0.1f) 
		{
			info.m_rotation = Utility::ToDegrees(std::atan2(info.m_velocity.y, info.m_velocity.x)) + 90.f;
		}
		//boundry check
		info.m_position.x = std::clamp(info.m_position.x, 40.f, m_battlefield_size.x - 40.f);
		info.m_position.y = std::clamp(info.m_position.y, 40.f, m_battlefield_size.y - 40.f);
	}

	HandleTankCollisions(dt);
	UpdateTurret(dt);
	//reflect the projectile movement and check for collisions with tanks and boundaries
	UpdateProjectiles(dt);
	CheckForWinner();

	//pickups spawning handle 
	if (m_game_started)
	{
		m_pickup_spawn_timer -= sf::seconds(dt);
		if (m_pickup_spawn_timer <= sf::Time::Zero && m_active_pickups.size() < kMaxPickups)
		{
			SpawnPickup();
			m_pickup_spawn_timer = sf::seconds(5.f + (std::rand() % 10));
		}
	}
	HandlePickupCollisions();
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
			m_tank_info[tankID].m_collision_cooldown = 0.f;
			m_tank_info[tankID].m_map_vote = 255; // not voted yet
			//setting the tank id for new connections to receive and update
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
				packet.clear();
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
	if (!(packet >> packetType))
	{
		return;
	}

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
			if (receiving_peer.m_tank_identifiers.empty()) return;
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

			if (receiving_peer.m_tank_identifiers.empty()) return;
			uint8_t id = receiving_peer.m_tank_identifiers[0];
			
			//strore players vote
			m_tank_info[id].m_map_vote = mapType;
			std::cout << "[SERVER] Player " << (int)id << " voted for map " << (int)mapType << std::endl;
			//BroadcastLobbyUpdate();
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

		case Client::PacketType::kStateUpdate:
		{
			uint8_t tank_count;
			packet >> tank_count;

			std::cout << "[SERVER] Received StateUpdate with " << static_cast<int>(tank_count) << " tanks" << std::endl;

			for (uint8_t i = 0; i < tank_count; i++)
			{
				uint8_t identifier;
				sf::Vector2f position;
				float rotation;
				uint8_t hitpoints, ammo, missileAmmo;
				float stamina;

				packet >> identifier >> position.x >> position.y >> rotation >> hitpoints >> ammo >> missileAmmo >> stamina;

				/*if (m_tank_info.find(identifier) != m_tank_info.end())
				{
					m_tank_info[identifier].m_hitpoints = hitpoints;
					m_tank_info[identifier].m_current_ammo = ammo;
					m_tank_info[identifier].m_missile_ammo = missileAmmo;
					m_tank_info[identifier].stamina = stamina;

					std::cout << "[SERVER] Tank " << static_cast<int>(identifier)
						<< " moved to (" << position.x << ", " << position.y << ")" << std::endl;
				}*/
			}
			break;
		}

		case Client::PacketType::kPlayerRealtimeChange:
		{
			uint8_t action, identifier;
			bool actionEnabled;
			packet >> identifier >> action >> actionEnabled;

			if (m_tank_info.find(identifier) != m_tank_info.end())
			{
				m_tank_info[identifier].m_real_time_actions[action] = actionEnabled;
				std::cout << "[SERVER] Tank " << static_cast<int>(identifier)
					<< " Action " << static_cast<int>(action)
					<< " = " << (actionEnabled ? "true" : "false") << std::endl;
			}
			break;
		}
		case Client::PacketType::kKeepAlive:
		{
			// Just update the last packet time, do nothing else
			break;
		}

		case Client::PacketType::kPlayerEvent:
		{
			uint8_t tank_identifier;
			uint8_t action;
			packet >> tank_identifier >> action;
			NotifyPlayerEvent(tank_identifier, action);
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
	packet << static_cast<uint8_t>(m_selected_map);
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
		CheckIfMapVotingDone();
	}
}

void GameServer::UpdateClientState()
{
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
	packet << static_cast<uint8_t>(m_tank_info.size());

	std::cout << "[SERVER] Sending UpdateClientState with " << m_tank_info.size() << " tanks" << std::endl;

	for (const auto& pair : m_tank_info)
	{
		std::cout << "  [SERVER] Tank " << static_cast<int>(pair.first)
			<< " at (" << pair.second.m_position.x << ", " << pair.second.m_position.y << ")" << std::endl;

		packet << pair.first;
		packet << pair.second.m_position.x;
		packet << pair.second.m_position.y;
		packet << pair.second.m_rotation;
		packet << pair.second.m_hitpoints;
		packet << pair.second.m_current_ammo;
		packet << pair.second.m_missile_ammo;
		packet << pair.second.stamina;
	}

	std::cout << "[SERVER] Update packet size: " << packet.getDataSize() << " bytes." << std::endl;
	SendToAll(packet);
}

void GameServer::NotifyPlayerRealtimeChange(uint8_t tank_identifier, uint8_t action, bool action_enabled)
{
	m_tank_info[tank_identifier].m_real_time_actions[action] = action_enabled;
}

void GameServer::NotifyPlayerEvent(uint8_t tank_identifier, int8_t action)
{
	//this will tell the server that a player has performed an action like shooting or dashing, so the server can do the necessary logic and then broadcast it to other clients
	sf::Packet packet;
	std::cout << "Server: Notify Player Event" << +tank_identifier << +action << std::endl;

	packet << static_cast<uint8_t>(Server::PacketType::kPlayerEvent);
	packet << tank_identifier;
	packet << action;
	packet << m_tank_info[tank_identifier].m_rotation;

	SendToAll(packet);

	if (action == static_cast<uint8_t>(Action::kBulletFire))
	{
		SpawnProjectile(tank_identifier);
	}
}

void GameServer::CheckIfMapVotingDone()
{
	std::map<uint8_t, int> votePlayer;
	int totalVotes = 0;

	for (auto const& [id, info] : m_tank_info)
	{
		if (info.m_map_vote != 255)
		{
			votePlayer[info.m_map_vote]++;
			totalVotes++;
		}
	}

	//auto vote for players that only pressed ready wihtout voting 
	if (totalVotes < m_connected_players)
	{
		for (auto& [id, info] : m_tank_info)
		{
			if (info.m_map_vote == 255)
			{
				info.m_map_vote = std::rand() % static_cast<int>(MapType::kTypeCount);
				votePlayer[info.m_map_vote]++;
				totalVotes++;
			}
		}
	}

	// choose the map based on votes
	uint8_t finalMap = 0;
	if (totalVotes > 0)
	{
		int maxVotes = 0;
		std::vector<uint8_t> winners;

		for (auto const& [mapId, count] : votePlayer)
		{
			if (count > maxVotes) { maxVotes = count; winners = { mapId }; }
			else if (count == maxVotes) { winners.push_back(mapId); }
		}
		finalMap = winners[std::rand() % winners.size()];
	}
	else
	{
		//having a default map if no one votes and just presses ready 
		finalMap = std::rand() % static_cast<int>(MapType::kTypeCount);
	}

	m_selected_map = finalMap;
	m_game_started = true; // physic thread Tick() start processing 
	SetListening(false);   //nnot listening for new connections 

	std::cout << "[SERVER] Match starting on Map: " << (int)finalMap << std::endl;

	// start game packet 
	sf::Packet packetStart;
	packetStart << static_cast<uint8_t>(Server::PacketType::kStartGame);
	packetStart << m_selected_map;
	SendToAll(packetStart);

	//second packet to send initial player spawn positions and other info
	sf::Packet statePacket;
	statePacket << static_cast<uint8_t>(Server::PacketType::kInitialState);
	statePacket << m_selected_map;
	statePacket << static_cast<uint8_t>(m_tank_info.size());


	// Calculate spawn positions in a circle
	sf::Vector2f center = m_battlefield_size / 2.f;
	float radius = kSpawnRadius;
	float angleStep = 360.f / m_tank_info.size();
	int playerIndex = 0;

	for (auto& pair : m_tank_info)
	{
		float radians = Utility::toRadians(angleStep * playerIndex);
		pair.second.m_position.x = center.x + std::cos(radians) * radius;
		pair.second.m_position.y = center.y + std::sin(radians) * radius;

		// Authoritative rotation: make everyone face the center
		pair.second.m_rotation = Utility::ToDegrees(std::atan2(center.y - pair.second.m_position.y, center.x - pair.second.m_position.x)) + 90.f;

		statePacket << pair.first;
		statePacket << pair.second.m_tank_type;
		statePacket << pair.second.m_position.x;
		statePacket << pair.second.m_position.y;
		playerIndex++;
	}

	SendToAll(statePacket);
}

void GameServer::SpawnPickup() {
	PickupInfo pickup;
	pickup.m_type = std::rand() % static_cast<int>(PickupType::kTypeCount);
	pickup.m_pikcup_identifier = m_pickup_id_counter++;

	float margin = 100.f; 
	pickup.m_position.x = margin + (std::rand() % (int)(m_battlefield_size.x - 2 * margin));
	pickup.m_position.y = margin + (std::rand() % (int)(m_battlefield_size.y - 2 * margin));

	m_active_pickups.push_back(pickup);

	//inform clients to spawn this pickups 
	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kSpawnPickup);
	packet << pickup.m_type << pickup.m_position.x << pickup.m_position.y << pickup.m_pikcup_identifier;
	SendToAll(packet);
}

void GameServer::HandlePickupCollisions()
{
	for (auto& tankPair : m_tank_info)
	{
		uint8_t tankID = tankPair.first;
		TankInfo& tank = tankPair.second;
		//server will check what type of pickup it was and will send the game action connected with that type of pickup 
		const TankData& stats = TankTable[tank.m_tank_type];

		for (auto it = m_active_pickups.begin(); it != m_active_pickups.end();)
		{
			//distance checks
			float dx = tank.m_position.x - it->m_position.x;
			float dy = tank.m_position.y - it->m_position.y;
			float distSqrt = dx * dx + dy * dy; //pythagoras

			const float pickupRadius = 120.f;
			if (distSqrt < pickupRadius * pickupRadius)
			{
				uint8_t gameActionType;
				uint8_t amount;

				switch (static_cast<PickupType>(it->m_type))
				{
					case PickupType::kHealthRefill:
					{
						uint8_t newHP = static_cast<uint8_t>(std::min<int>(stats.m_hitpoints, static_cast<int>(tank.m_hitpoints + 30)));
						amount = static_cast<uint8_t>(newHP - tank.m_hitpoints);
						if (amount == 0) { ++it; continue; }
						tank.m_hitpoints = newHP;
						gameActionType = GameActions::kTankHealed;
						break;
					}

					case PickupType::kBulletRefill:
					{
						uint8_t newAmmo = static_cast<uint8_t>(std::min<int>(stats.m_ammo_amount, static_cast<int>(tank.m_current_ammo + 5)));
						amount = static_cast<uint8_t>(newAmmo - tank.m_current_ammo);
						if (amount == 0) { ++it; continue; }
						tank.m_current_ammo = newAmmo;
						gameActionType = GameActions::kAmmoRefilled;
						break;
					}

					case PickupType::kMissile:
					{
						tank.m_missile_ammo += 1;
						gameActionType = GameActions::kMissileRefilled;
						amount = 1;
						break;
					}
				}

				//notify clients now 
				sf::Packet packet;
				packet << static_cast<uint8_t>(Server::PacketType::kGameEvent);
				packet << static_cast<uint8_t>(gameActionType);
				packet << tankID;
				packet << amount;
				packet << it->m_pikcup_identifier;
				SendToAll(packet);

				it = m_active_pickups.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
}

void GameServer::SpawnProjectile(uint8_t tank_identifier)
{
	auto it = m_tank_info.find(tank_identifier);
	if (it == m_tank_info.end())
	{
		return;
	}

	TankInfo& tank = it->second;
	const bool useMissile = tank.m_missile_ammo > 0; //true when player has missile ammo, false when only regular bullet

	if (!useMissile && tank.m_current_ammo == 0)
	{
		return; //out of ammo
	}

	ProjectileType type = ProjectileType::kTank1Bullet; //default bullet 

	if (useMissile)
	{
		type = ProjectileType::kMissile;
		tank.m_missile_ammo--;
	}
	else
	{
		//decide on type of bullet based on tank type and then decrease its ammo 
		switch (static_cast<TankType>(tank.m_tank_type))
		{
			case TankType::kTank1: type = ProjectileType::kTank1Bullet; break;
			case TankType::kTank2: type = ProjectileType::kTank2Bullet; break;
			case TankType::kTank3: type = ProjectileType::kTank3Bullet; break;
			case TankType::kTank4: type = ProjectileType::kTank4Bullet; break;
			default: type = ProjectileType::kTank1Bullet; break;
		}
		tank.m_current_ammo--;
	}

	const float radiansRotation = Utility::toRadians(tank.m_rotation);
	sf::Vector2f direction(std::sin(radiansRotation), -std::cos(radiansRotation));

	//spawn, give owner, speed and direction 
	ProjectileInfo projectile;
	projectile.m_type = type;
	projectile.m_owner_id = tank_identifier;
	projectile.m_position = tank.m_position + direction * 100.f;
	projectile.m_velocity = direction * ProjectileTable[static_cast<int>(type)].m_speed;

	m_projectiles.push_back(projectile);
}

void GameServer::UpdateProjectiles(float dt)
{
	for (auto it = m_projectiles.begin(); it != m_projectiles.end();)
	{
		//if missile go to the nearest enemy tank
		if (it->m_type == ProjectileType::kMissile)
		{
			TankInfo* closestEnemy = nullptr;
			float minDistance = 999999.f;

			for (auto& tankPair : m_tank_info)
			{
				if (tankPair.first == it->m_owner_id || tankPair.second.m_hitpoints == 0)
				{
					continue;
				}

				sf::Vector2f diff = tankPair.second.m_position - it->m_position;
				float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
				if (dist > 150.f && dist < minDistance)
				{
					minDistance = dist;
					closestEnemy = &tankPair.second;
				}
			}

			if (closestEnemy)
			{
				sf::Vector2f dir = Utility::Normalise(closestEnemy->m_position - it->m_position);
				it->m_velocity = dir * ProjectileTable[static_cast<int>(it->m_type)].m_speed;
			}
		}
		// update projectile position based on its velocity
		it->m_position += it->m_velocity * dt; 

		// bounry checkls 
		if (it->m_position.x < 0.f || it->m_position.y < 0.f ||
			it->m_position.x > m_battlefield_size.x || it->m_position.y > m_battlefield_size.y)
		{
			it = m_projectiles.erase(it);
			continue;
		}

		bool hit = false;
		for (auto& tankPair : m_tank_info)
		{
			uint8_t tankID = tankPair.first;
			TankInfo& tank = tankPair.second;

			//self chekc and also check if the tank is already destroyed
			if (tankID == it->m_owner_id || tank.m_hitpoints == 0)
			{
				continue;
			}

			//pythagorean check collision
			const TankData& stats = TankTable[tank.m_tank_type];
			sf::Vector2f size(
				static_cast<float>(stats.m_texture_rect.size.x),
				static_cast<float>(stats.m_texture_rect.size.y));

			//edge paddding might not need to test
			const float padding = 6.f;
			size.x += padding;
			size.y += padding;

			sf::FloatRect tankBounds(tank.m_position - size / 2.f, size);

			if (tankBounds.contains(it->m_position))
			{
				const uint8_t damage = static_cast<uint8_t>(ProjectileTable[static_cast<int>(it->m_type)].m_damage);
				const uint8_t newHP = std::max(0, static_cast<int>(tank.m_hitpoints) - damage);
				tank.m_hitpoints = static_cast<uint8_t>(newHP);

				sf::Packet packet;
				packet << static_cast<uint8_t>(Server::PacketType::kEntityDamage)
					<< tankID
					<< tank.m_hitpoints
					<< damage;


				SendToAll(packet);

				hit = true;

				if (newHP == 0)
				{
					NotifyPlayerEliminated(tankID);
				}

				break;
			}
		}

		if (hit)
		{
			it = m_projectiles.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void GameServer::HandleTankCollisions(float dt)
{
	const float pushForce = 10.0f;
	const float damageThreshold = 10.0f;
	const float cooldownSeconds = 1.0f;
	const uint8_t rammingDamage = 10;

	auto clampPosition = [this](sf::Vector2f& pos)
		{
			pos.x = std::clamp(pos.x, 40.f, m_battlefield_size.x - 40.f);
			pos.y = std::clamp(pos.y, 40.f, m_battlefield_size.y - 40.f);
		};

	auto applyDamage = [this, cooldownSeconds, rammingDamage](uint8_t id, TankInfo& tank)
		{
			if (tank.m_collision_cooldown > 0.f || tank.m_hitpoints == 0)
			{
				return false;
			}

			const int newHp = std::max(0, static_cast<int>(tank.m_hitpoints) - rammingDamage);
			if (newHp == tank.m_hitpoints)
			{
				return false;
			}

			tank.m_hitpoints = static_cast<uint8_t>(newHp);
			tank.m_collision_cooldown = cooldownSeconds;

			sf::Packet packet;
			packet << static_cast<uint8_t>(Server::PacketType::kEntityDamage)
				<< id
				<< tank.m_hitpoints
				<< rammingDamage;

			SendToAll(packet);

			if (newHp == 0)
			{
				NotifyPlayerEliminated(id);
			}

			return true;
		};

	//aabb collision logic
	auto getBounds = [](const TankInfo& tank)
		{
			const TankData& stats = TankTable[tank.m_tank_type];
			sf::Vector2f size(
				static_cast<float>(stats.m_texture_rect.size.x),
				static_cast<float>(stats.m_texture_rect.size.y));
			return sf::FloatRect(tank.m_position - size / 2.f, size);
		};

	for (auto it1 = m_tank_info.begin(); it1 != m_tank_info.end(); ++it1)
	{
		for (auto it2 = std::next(it1); it2 != m_tank_info.end(); ++it2)
		{
			uint8_t id1 = it1->first;
			uint8_t id2 = it2->first;
			TankInfo& tank1 = it1->second;
			TankInfo& tank2 = it2->second;

			if (tank1.m_hitpoints == 0 || tank2.m_hitpoints == 0)
			{
				continue;
			}

			if (!getBounds(tank1).findIntersection(getBounds(tank2)))
			{
				continue;
			}

			sf::Vector2f diff = tank1.m_position - tank2.m_position;
			float distSq = diff.x * diff.x + diff.y * diff.y;

			if (distSq < 0.01f)
			{
				continue;
			}

			float dist = std::sqrt(distSq);
			sf::Vector2f normal = diff / dist;

			tank1.m_position += normal * pushForce;
			tank2.m_position -= normal * pushForce;

			float tank1SpeedTowards = tank1.m_velocity.x * -normal.x + tank1.m_velocity.y * -normal.y;
			float tank2SpeedTowards = tank2.m_velocity.x * normal.x + tank2.m_velocity.y * normal.y;
			//each other ram
			if (tank1SpeedTowards > tank2SpeedTowards + damageThreshold)
			{
				if (applyDamage(id2, tank2))
				{
					tank2.m_velocity += -normal * (pushForce + 200.0f);
				}
			}
			//one sided rams 
			else if (tank2SpeedTowards > tank1SpeedTowards + damageThreshold)
			{
				if (applyDamage(id1, tank1))
				{
					tank1.m_velocity += normal * (pushForce + 200.0f);
				}
			}
			else if (tank1SpeedTowards > 0 && tank2SpeedTowards > 0)
			{
				if (applyDamage(id1, tank1))
				{
					tank1.m_velocity += normal * (pushForce + 50.0f);
				}
				if (applyDamage(id2, tank2))
				{
					tank2.m_velocity += -normal * (pushForce + 50.0f);
				}
			}
		
			float dot1 = tank1.m_velocity.x * normal.x + tank1.m_velocity.y * normal.y;
			float dot2 = tank2.m_velocity.x * normal.x + tank2.m_velocity.y * normal.y;

			if (dot1 < 0.f) tank1.m_velocity -= normal * dot1;
			if (dot2 > 0.f) tank2.m_velocity -= normal * dot2;

			clampPosition(tank1.m_position);
			clampPosition(tank2.m_position);
		}
	}
}

void GameServer::UpdateTurret(float dt)
{
	const TurretData& turret = TurretTable[static_cast<int>(TurretType::kStandard)];

	TankInfo* closestTank = nullptr;
	float minDist = turret.m_range;

	for (auto& pair : m_tank_info)
	{
		if (pair.second.m_hitpoints == 0)
		{
			continue;
		}

		sf::Vector2f diff = pair.second.m_position - m_turret_position;
		float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

		if (dist < minDist)
		{
			minDist = dist;
			closestTank = &pair.second;
		}
	}

	if (!closestTank)
	{
		return;
	}

	sf::Vector2f dir = Utility::Normalise(closestTank->m_position - m_turret_position);
	m_turret_rotation = Utility::ToDegrees(std::atan2(dir.y, dir.x)) + 90.f;

	// send rotation every tick
	sf::Packet turretPacket;
	turretPacket << static_cast<uint8_t>(Server::PacketType::kTurretState)
		<< m_turret_rotation;
	SendToAll(turretPacket);

	//fire only when cooldown expires
	m_turret_fire_cooldown -= sf::seconds(dt);
	if (m_turret_fire_cooldown > sf::Time::Zero)
	{
		return;
	}

	ProjectileInfo projectile;
	projectile.m_type = turret.m_bullet_type;
	projectile.m_owner_id = 255;
	projectile.m_position = m_turret_position + dir * 60.f;
	projectile.m_velocity = dir * ProjectileTable[static_cast<int>(turret.m_bullet_type)].m_speed;

	m_projectiles.push_back(projectile);
	m_turret_fire_cooldown = turret.m_fire_interval;

	sf::Packet shotPacket;
	shotPacket << static_cast<uint8_t>(Server::PacketType::kSpawnProjectile)
		<< static_cast<uint8_t>(projectile.m_type)
		<< projectile.m_position.x << projectile.m_position.y
		<< projectile.m_velocity.x << projectile.m_velocity.y
		<< projectile.m_owner_id;
	SendToAll(shotPacket);
}

void GameServer::CheckForWinner()
{
	if (m_game_finished || !m_game_started)
	{
		return;
	}

	uint8_t aliveId = 0;
	int aliveCount = 0;

	for (const auto& [id, tank] : m_tank_info)
	{
		if (tank.m_hitpoints > 0)
		{
			aliveId = id;
			++aliveCount;
		}
	}

	if (aliveCount <= 1)
	{
		m_game_finished = true;

		sf::Packet packet;
		packet << static_cast<uint8_t>(Server::PacketType::kMissionSuccess)
			<< aliveId;
		SendToAll(packet);
	}
}

void GameServer::NotifyPlayerEliminated(uint8_t tank_identifier)
{
	sf::Packet packetMessage;
	packetMessage << static_cast<uint8_t>(Server::PacketType::kBroadcastMessage)
			<< ("Player " + std::to_string(tank_identifier) + " was destroyed!");
	SendToAll(packetMessage);

	sf::Packet packet;
	packet << static_cast<uint8_t>(Server::PacketType::kPlayerEliminated)
			<< tank_identifier;
	SendToAll(packet);
}