#include "multiplayer_game_state.hpp"
#include "constants.hpp"
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
	, m_scene_initialized(false)
	, m_client_timeout(sf::seconds(2.f))
	, m_time_since_last_packet(sf::seconds(kClientTimeout))
	, m_broadcast_text(context.fonts->Get(FontID::kMain))
	, m_failed_connection_text(context.fonts->Get(FontID::kMain))
{
	sf::Vector2f windowSize = m_window.getView().getSize();

	float scale = Utility::CalculateScale(windowSize.x, windowSize.y);
	unsigned int broadcastFontSize = static_cast<unsigned int>(std::max(static_cast<float>(kMinimumFontSize), 24.f * scale));
	m_broadcast_text.setCharacterSize(broadcastFontSize);
	m_broadcast_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, 100.f * scale));

	unsigned int failedFontSize = static_cast<unsigned int>(std::max(static_cast<float>(kMinimumFontSize), 35.f * scale));
	m_failed_connection_text.setCharacterSize(failedFontSize);
	m_failed_connection_text.setFillColor(sf::Color::White);
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	sf::TcpSocket* socket = GetContext().socket;
	socket->setBlocking(false);

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

		if (m_scene_initialized)
		{
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

			UpdateBroadcastMessage(dt);

			GameActions::Action game_action;
			while (m_world.PollGameAction(game_action))
			{
				sf::Packet eventPacket;
				eventPacket << static_cast<uint8_t>(Client::PacketType::kGameEvent)
					<< static_cast<uint8_t>(game_action.type)
					<< game_action.position.x
					<< game_action.position.y;
				GetContext().socket->send(eventPacket);
			}

			//position updates
			if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / kNetworkUpdateRate))
			{
				//send realtime input updates for each local player
				for (uint8_t identifier : m_local_player_identifiers)
				{
					for (int i = 0; i < static_cast<int>(Action::kActionCount); ++i)
					{
						Action action = static_cast<Action>(i);

						// handle the realtime actions like moving
						if (IsRealtimeAction(action))
						{
							sf::Keyboard::Scancode boundKey = GetContext().keys1->GetAssignedKey(action);
							bool isPressed = sf::Keyboard::isKeyPressed(boundKey);

							sf::Packet input_packet;
							input_packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
							input_packet << identifier;
							input_packet << static_cast<uint8_t>(action);
							input_packet << isPressed;
							GetContext().socket->send(input_packet);
						}
					}
				}

				//send those positions updates for each player 's tank to the server so it can update the authoritative state and then send it back to all clients
				sf::Packet position_update_packet;
				position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
				position_update_packet << static_cast<uint8_t>(m_local_player_identifiers.size());

				for (uint8_t identifier : m_local_player_identifiers)
				{
					if (Tank* tank = m_world.GetTank(identifier))
					{
						std::cout << "[CLIENT] Sending StateUpdate for tank " << static_cast<int>(identifier)
							<< " at (" << tank->getPosition().x << ", " << tank->getPosition().y << ")" << std::endl;

						position_update_packet << identifier
							<< tank->getPosition().x
							<< tank->getPosition().y
							<< tank->getRotation().asDegrees()
							<< static_cast<uint8_t>(tank->GetHitPoints())
							<< static_cast<uint8_t>(tank->GetAmmoCount())
							<< tank->GetStaminaRatio();
					}
				}
				GetContext().socket->send(position_update_packet);
				m_tick_clock.restart();
			}
		}

		// Handle packets and network regardless of scene state
		sf::TcpSocket& socket = *GetContext().socket;
		sf::Packet packet;
		if (socket.receive(packet) == sf::Socket::Status::Done)
		{
			m_time_since_last_packet = sf::seconds(0.f);
			uint8_t packet_type;
			packet >> packet_type;
			HandlePacket(packet_type, packet);
			packet.clear();
		}
		else
		{
			m_time_since_last_packet += dt;
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text.setString("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text);
				m_failed_connection_clock.restart();
			}
		}
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

			std::cout << "[DEBUG] kInitialState received. Map: " << static_cast<int>(map_type) << ", Tank count: " << static_cast<int>(tank_count) << std::endl;


			//first build the map and then add tanks becauise they are being addeed to the layer
			m_world.SetCurrentMap(static_cast<MapType>(map_type));
			std::cout << "[DEBUG] SetCurrentMap done" << std::endl;
			m_world.InitializeScene();
			std::cout << "[DEBUG] InitializeScene done" << std::endl;

			for (uint8_t i = 0; i < tank_count; i++)
			{
				uint8_t identifier, tank_type;
				sf::Vector2f position;

				//read 
				packet >> identifier >> tank_type >> position.x >> position.y;
				std::cout << "[DEBUG] Tank " << static_cast<int>(i) << " - ID: " << static_cast<int>(identifier) << " Type: " << static_cast<int>(tank_type) << std::endl;


				Tank* tank = m_world.AddTank(identifier, static_cast<TankType>(tank_type));
				std::cout << "[DEBUG] AddTank done for ID: " << static_cast<int>(identifier) << std::endl;

				tank->setPosition(position);
				std::cout << "[DEBUG] setPosition done" << std::endl;

				//only add to player map if not local player, local player already created in lobby and added to world
				if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), identifier) == m_local_player_identifiers.end())
				{
					m_players[identifier].reset(new Player(&socket, identifier, nullptr));
					std::cout << "[DEBUG] Created new Player for remote tank ID: " << static_cast<int>(identifier) << std::endl;
				}
				else
				{
					std::cout << "[DEBUG] Local tank ID: " << static_cast<int>(identifier) << " - no new Player created" << std::endl;
				}
			}
			m_scene_initialized = true;
			std::cout << "[DEBUG] kInitialState complete" << std::endl;
			break;
		}

		case Server::PacketType::kUpdateClientState:
		{
			uint8_t tank_count;
			packet >> tank_count;

			std::cout << "[CLIENT] Received UpdateClientState with " << static_cast<int>(tank_count) << " tanks" << std::endl;

			for (uint8_t i = 0; i < tank_count; i++)
			{
				uint8_t identifier, hitpoints, ammo, missile_ammo;
				sf::Vector2f position;
				float rotation, stamina;

				packet >> identifier >> position.x >> position.y >> rotation >> hitpoints >> ammo >> missile_ammo >> stamina;

				std::cout << "[CLIENT] Tank " << static_cast<int>(identifier)
					<< " at (" << position.x << ", " << position.y << ")" << std::endl;

				Tank* tank = m_world.GetTank(identifier);
				if (!tank)
				{
					std::cout << "[CLIENT] Tank " << static_cast<int>(identifier) << " not found!" << std::endl;
					continue;
				}

				//check if this is clients own tank
				bool is_local = (identifier == *GetContext().local_id);

				if (is_local)
				{
					//sync positions with server 
					// little smoothing instead of snapping positions 
					sf::Vector2f correctionPos = tank->getPosition() + (position - tank->getPosition()) * 0.1f;
					tank->setPosition(correctionPos);
					tank->setRotation(sf::degrees(rotation));

					//sync stats
					tank->SetHitpoints(hitpoints);
					tank->SetAmmo(ammo);
					tank->SetMissileAmmo(missile_ammo);
				}
				else
				{
					//for remote tanks smooth the position and update them
					sf::Vector2f lerpPos = tank->getPosition() + (position - tank->getPosition()) * kNetworkInterpolation;
					tank->setPosition(lerpPos);
					tank->setRotation(sf::degrees(rotation));
					tank->SetHitpoints(hitpoints);
					tank->SetAmmo(ammo);
					tank->SetMissileAmmo(missile_ammo);
					tank->SetStamina(stamina);
				}
			}
			break;
		}
		//will need to add more packet types for things like events, pickups etc
		default:
			break;
	}
}

bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
	CommandQueue& commands = m_world.GetCommandQueue();
	for (auto& pair : m_players)
	{
		pair.second->HandleEvent(event, commands);
	}

	const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();

	if (key_pressed)
	{
		if (key_pressed->scancode == sf::Keyboard::Scancode::Enter)
		{
			DisableAllRealtimeActions(false);
			RequestStackPush(StateID::kPause); //will need to make networked pause state to handle pausing in multiplayer properly, for now just pushing normal pause state
		}
	} 
	else if (event.is<sf::Event::FocusGained>())
	{
		m_has_focus = true;
	}
	else if (event.is<sf::Event::FocusLost>())
	{
		m_has_focus = false;
	}
	return true;
}

void MultiplayerGameState::OnActivate()
{
	m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
	if (!m_is_host && m_connected)
	{
		sf::Packet packet;
		packet << static_cast<uint8_t>(Client::PacketType::kQuit);
		GetContext().socket->send(packet);
	}
}

void MultiplayerGameState::DisableAllRealtimeActions(bool enable)
{
	m_active_state = enable;
	for (uint8_t id : m_local_player_identifiers)
	{
		if (m_players.find(id) != m_players.end())
			m_players[id]->DisableAllRealtimeActions(enable);
	}
}

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
	if (m_broadcasts.empty()) return;

	m_broadcast_elapsed_time += elapsed_time;
	if (m_broadcast_elapsed_time > sf::seconds(2.f))
	{
		m_broadcasts.erase(m_broadcasts.begin());
		if (!m_broadcasts.empty())
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
}

