#include "multiplayer_game_state.hpp"

#include "music_player.hpp"
#include "utility.hpp"
#include <iostream>
#include <SFML/Network/IpAddress.hpp>

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
	: State(stack, context)
	, m_world(*context.window, *context.fonts, *context.sound, true)
	, m_window(*context.window)
	, m_texture_holder(*context.textures)
	, m_connected(true) //lobby handled the setup
	, m_active_state(true)
	, m_is_host(is_host)
	, m_has_focus(true)
	, m_game_started(true)
	, m_client_timeout(sf::seconds(2.f))
	, m_time_since_last_packet(sf::seconds(0.f))
	, m_broadcast_text(context.fonts->Get(FontID::kMain))
	, m_failed_connection_text(context.fonts->Get(FontID::kMain))
{
	m_broadcast_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, 100.f));
	// messagei if connection to server is lost
	m_failed_connection_text.setCharacterSize(35);
	m_failed_connection_text.setFillColor(sf::Color::White);
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	//get socekt
	sf::TcpSocket* socket = GetContext().socket;
	socket->setBlocking(false);

	//setup local player with id assinged in lobby
	uint8_t localID = *GetContext().local_id;
	m_local_player_identifiers.push_back(localID);
	m_players[localID].reset(new Player(socket, localID, GetContext().keys1));
	context.music->Play(MusicThemes::kGameTheme);
}

void MultiplayerGameState::Draw()
{
	if (m_connected)
	{
		m_world.Draw();

		// Show UI overlays (Broadcasts)
		m_window.setView(m_window.getDefaultView());
		if (!m_broadcasts.empty())
		{
			m_window.draw(m_broadcast_text);
		}
	}
	else
	{
		m_window.setView(m_window.getDefaultView());
		m_window.draw(m_failed_connection_text);
	}
}

bool MultiplayerGameState::Update(sf::Time dt)
{
	if (m_connected)
	{
		if (!m_active_state) { DisableAllRealtimeActions(true); }

		m_world.Update(dt);

		//remoiving players who got destroyed
		bool found_local_tank = false;
		for (auto itr = m_players.begin(); itr != m_players.end();)
		{
			if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), itr->first) != m_local_player_identifiers.end())
				found_local_tank = true;

			if (!m_world.GetTank(itr->first))
			{
				itr = m_players.erase(itr);
				if (m_players.empty()) RequestStackPush(StateID::kGameOver);
			}
			else
			{
				++itr;
			}
		}

		if (!found_local_tank && m_game_started) { RequestStackPush(StateID::kGameOver); }

		if (m_active_state && m_has_focus)
		{
			CommandQueue& commands = m_world.GetCommandQueue();
			for (auto& pair : m_players)
			{
				pair.second->HandleRealTimeInput(commands);
			}
		}

		//handle network input 
		CommandQueue& commands = m_world.GetCommandQueue();
		for (auto& pair : m_players)
		{
			pair.second->HandleRealtimeNetworkInput(commands);
		}

		sf::TcpSocket& socket = *GetContext().socket;
		sf::Packet packet;
		if (socket.receive(packet) == sf::Socket::Status::Done)
		{
			m_time_since_last_packet = sf::seconds(0.f);
			uint8_t packet_type;
			packet >> packet_type;
			HandlePacket(packet_type, packet);
		}
		else
		{
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text.setString("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text);

				m_failed_connection_clock.restart();
			}
		}

		UpdateBroadcastMessage(dt);

		GameActions::Action game_action;
		while (m_world.PollGameAction(game_action))
		{
			sf::Packet eventPacket;
			eventPacket << static_cast<uint8_t>(Client::PacketType::kGameEvent)
				<< static_cast<uint8_t>(game_action.type)
				<< game_action.position.x
				<< game_action.position.y;
			socket.send(eventPacket);
		}

		//position updates
		if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f)) 
		{
			sf::Packet position_update_packet;
			position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
			position_update_packet << static_cast<uint8_t>(m_local_player_identifiers.size());

			for (uint8_t identifier : m_local_player_identifiers)
			{
				if (Tank* tank = m_world.GetTank(identifier))
				{
					position_update_packet << identifier
						<< tank->getPosition().x
						<< tank->getPosition().y
						<< tank->getRotation().asDegrees()
						<< static_cast<uint8_t>(tank->GetHitPoints())
						<< static_cast<uint8_t>(tank->GetAmmoCount())
						<< tank->GetStaminaRatio();
					//think I'll need more things to update like turret etc but for now testing this
				}
			}
			socket.send(position_update_packet);
			m_tick_clock.restart();
		}
		m_time_since_last_packet += dt;
	}
	else if (m_failed_connection_clock.getElapsedTime() > sf::seconds(5.f))
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}

	return true;
}

void MultiplayerGameState::HandlePacket(uint8_t packet_type, sf::Packet& packet)
{
	sf::TcpSocket& socket = *GetContext().socket;

	switch (static_cast<Server::PacketType>(packet_type))
	{
		case Server::PacketType::kBroadcastMessage:
		{
			std::string message;
			packet >> message;
			m_broadcasts.push_back(message);
			if (m_broadcasts.size() == 1)
			{
				m_broadcast_text.setString(m_broadcasts.front());
				Utility::CentreOrigin(m_broadcast_text);
				m_broadcast_elapsed_time = sf::Time::Zero;
			}
			break;
		}

		case Server::PacketType::kPlayerDisconnect:
		{
			uint8_t identifier;
			packet >> identifier;
			m_world.RemoveTank(identifier);
			m_players.erase(identifier);
			break;
		}

		case Server::PacketType::kInitialState:
		{
			uint8_t map_type, tank_count;
			packet >> map_type >> tank_count;

			for (uint8_t i = 0; i < tank_count; i++)
			{
				uint8_t identifier, tank_type;
				sf::Vector2f position;

				//read 
				packet >> identifier >> tank_type >> position.x >> position.y;

				Tank* tank = m_world.AddTank(identifier, static_cast<TankType>(tank_type));
				tank->setPosition(position);
			}
			break;
		}
}