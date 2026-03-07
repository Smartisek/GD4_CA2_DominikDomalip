#include "tank_select_state.hpp"
#include "data_tables.hpp"
#include "button.hpp"
#include "utility.hpp"
#include <iostream>
#include <fstream>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <optional>
#include <string>


sf::IpAddress GetAddressFromFile()
{
    {
        std::ifstream input_file("ip.txt");
        std::string ip_string;
        if (input_file >> ip_string)
        {
            if (auto address = sf::IpAddress::resolve(ip_string))
            {
                return *address;
            }
        }
    }
    //if read failed create new file with local host 
    std::ofstream output_file("ip.txt");
    sf::IpAddress local_address = sf::IpAddress::LocalHost;
    output_file << local_address.toString();
    return local_address;
}

TankSelectState::TankSelectState(StateStack& stack, Context context, bool is_host)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
	, m_gui_container()
	, m_title_text(context.fonts->Get(FontID::kMain))
	, m_is_host(is_host)
    , m_connected(false)
    , m_player_id(0)
    , m_selected_map(0)
{

    //Network setup
    if (m_is_host)
    {
        m_server.reset(new GameServer(sf::Vector2f(4000.f, 4000.f)));
        m_socket.connect(sf::IpAddress::LocalHost, SERVER_PORT);
    }
    else
    {
        m_socket.connect(GetAddressFromFile(), SERVER_PORT);
    }
    m_socket.setBlocking(false);
	//background setup
    sf::Vector2f viewSize = context.window->getView().getSize();
    sf::Vector2u textSize = m_background_sprite.getTexture().getSize();
    m_background_sprite.setScale(sf::Vector2f(viewSize.x / textSize.x, viewSize.y / textSize.y));
    m_dark_overlay.setSize(viewSize);
    m_dark_overlay.setFillColor(sf::Color(0, 0, 0, 250));

    //title
    m_title_text.setFont(context.fonts->Get(FontID::kMain));
    m_title_text.setString("Multiplayer Lobby");
    m_title_text.setCharacterSize(50);
    Utility::CentreOrigin(m_title_text);
    m_title_text.setPosition(sf::Vector2f(viewSize.x / 2.f + 100.f, viewSize.y / 2.f));
    
    //for getting the data 
    const std::vector<TankData> tankData = InitializeTankData();

    // Define layout variables
    float startX = viewSize.x * 0.25f; 
    float startY = 150.f;   
    float gapY = 280.f;                

    for (int i = 0; i < static_cast<int>(TankType::kTankCount); ++i)
    {
        TankType type = static_cast<TankType>(i);
        auto button = std::make_shared<gui::Button>(context);

        // using the custom button I made for background selection
        button->SetCustomIcon(context.textures->Get(tankData[i].m_texture), tankData[i].m_texture_rect);

        // button->setScale(0.8f, 0.8f); 
        float currentY = startY + (i * gapY);
        button->setPosition(sf::Vector2f(startX, currentY));
        button->SetCallback([this, i]() { SendTankSelection(static_cast<uint8_t>(i)); });
        m_gui_container.Pack(button);

        //stats logic text
        sf::Text statText(context.fonts->Get(FontID::kMain));
        statText.setCharacterSize(20);
        statText.setFillColor(sf::Color::White);

        // format fot printign the stats
        std::stringstream ss;
        ss << "HP:      " << tankData[i].m_hitpoints << "\n"
            << "Speed:   " << static_cast<int>(tankData[i].m_speed) << "\n"
            << "Ammo:    " << tankData[i].m_ammo_amount << "\n"
            << "Stamina: " << static_cast<int>(tankData[i].m_max_stamina);

        statText.setString(ss.str());

        statText.setPosition(sf::Vector2f(startX + 200.f, currentY + 20.f ));

        //populate the vector texts
        m_stat_texts.push_back(statText);

        auto readyBtn = std::make_shared<gui::Button>(context);
        readyBtn->SetText("Ready Up");
        readyBtn->setPosition(sf::Vector2f(viewSize.x - 250.f, viewSize.y - 100.f));
        readyBtn->SetCallback([this]() { SendReadyToggle(); });
        m_gui_container.Pack(readyBtn);
    }
}

void TankSelectState::SendTankSelection(uint8_t tankType)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Client::PacketType::kSelectTank) << tankType;
    m_socket.send(packet);
}

void TankSelectState::SendReadyToggle()
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Client::PacketType::kToggleReady);
    m_socket.send(packet);
}

void TankSelectState::HandlePacket(uint8_t packetType, sf::Packet& packet)
{
    switch (static_cast<Server::PacketType>(packetType))
    {
        case Server::PacketType::kPlayerConnect:
        {
            uint8_t playerId;
            float posX, posY;
            if (packet >> playerId >> posX >> posY)
            {
                std::cout << "[CLIENT] kPlayerConnect received. Player ID: " << static_cast<int>(playerId) << std::endl;

                // Store own player ID when first connecting
                if (m_player_id == 0)
                {
                    m_player_id = playerId;
                    m_connected = true;
                    std::cout << "[CLIENT] Set own ID to: " << static_cast<int>(playerId) << std::endl;
                }

                // Add/update player in lobby list
                if (m_players.find(playerId) == m_players.end())
                {
                    m_players[playerId] = { 0, false };
                    std::cout << "[CLIENT] Added new player. Total: " << m_players.size() << std::endl;
                }
            }
            break;
        }

        case Server::PacketType::kPlayerDisconnect:
        {
            uint8_t playerId;
            if (packet >> playerId)
            {
                std::cout << "[CLIENT] kPlayerDisconnect received. Player ID: " << static_cast<int>(playerId) << std::endl;
                m_players.erase(playerId);
            }
            break;
        }

        case Server::PacketType::kLobbyUpdate :
        {
            uint8_t mapId, playerCount;
            if (!(packet >> mapId >> playerCount))
            {
                std::cout << "[CLIENT] Failed to read kLobbyUpdate header" << std::endl;
                break;
            }

            m_selected_map = mapId;
            for (int i = 0; i < playerCount; ++i)
            {
                uint8_t id, type;
                bool ready;
                packet >> id >> type >> ready;
                m_players[id] = { type, ready };
            }
            break;
        }

        case Server::PacketType::kInitialState:
        {
            // server will start game 
            std::cout << "[CLIENT] kInitialState received. Starting game." << std::endl;
            RequestStackPop();
            RequestStackPush(StateID::kMultiplayerGame);
            break;
        }

        default:
        {
            std::cout << "[CLIENT] Unhandled packet type: " << static_cast<int>(packetType) << std::endl;
            break;
        }
    }
}

void TankSelectState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_dark_overlay);
    window.draw(m_title_text);
    window.draw(m_gui_container);

    float drawY = 150.f;

    for (auto const& [id, data] : m_players)
    {
        sf::Text info(GetContext().fonts->Get(FontID::kMain),
            "Player " + std::to_string(id) + (data.is_ready ? " [READY]" : " [WAITING]"),
            24);

        info.setFillColor(data.is_ready ? sf::Color::Green : sf::Color::White);
        info.setPosition({ 500.f, drawY });
        window.draw(info);
        drawY += 40.f;
    }

    for (const auto& text : m_stat_texts)
    {
        window.draw(text);
    }
}

bool TankSelectState::Update(sf::Time)
{
    int packetCount = 0;

    if (m_keepalive_clock.getElapsedTime() >= m_keepalive_interval)
    {
        sf::Packet keepalive;
        keepalive << static_cast<uint8_t>(Client::PacketType::kKeepAlive);
        m_socket.send(keepalive);
        m_keepalive_clock.restart();
    }

    sf::Packet packet;
    while (m_socket.receive(packet) == sf::Socket::Status::Done)
    {
        packetCount++;
        uint8_t packetType;
        if (packet >> packetType)
        {
            std::cout << "[CLIENT] Received packet type: " << static_cast<int>(packetType)
                << " (kLobbyUpdate=1, kInitialState=2)" << std::endl;
            HandlePacket(packetType, packet);
        }
        else
        {
            std::cout << "[CLIENT] Failed to extract packet type" << std::endl;
        }
        packet.clear();
    }

    if (packetCount > 0)
    {
        std::cout << "[CLIENT] Total packets this frame: " << packetCount
            << ", m_players.size()=" << m_players.size() << std::endl;
    }

    return true;
}

bool TankSelectState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return false;
}