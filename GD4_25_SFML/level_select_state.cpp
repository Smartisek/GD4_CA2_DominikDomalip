#include "level_select_state.hpp"
#include <SFML/Network/Packet.hpp>
#include "network_protocol.hpp"
#include "utility.hpp"
#include "data_tables.hpp"
#include "button.hpp"
#include <iostream>

LevelSelectState::LevelSelectState(StateStack& stack, Context context, bool isHost)
	: State(stack, context)
	, m_gui_container()
	, m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
    , m_is_host(isHost)

{
	sf::Vector2f viewSize = context.window->getView().getSize();
	sf::Vector2u textSize = m_background_sprite.getTexture().getSize();
	m_background_sprite.setScale({ viewSize.x / textSize.x, viewSize.y / textSize.y });
	//darken the background a little
	m_overlay.setSize(viewSize);
	m_overlay.setFillColor(sf::Color(0, 0, 0, 250));

	const std::vector<MapData> mapData = InitializeMapData();
	float startY = 150.f;


    for (int i = 0; i < static_cast<int>(MapType::kTypeCount); ++i)
    {
        MapType type = static_cast<MapType>(i);

        // make abutton
        auto button = std::make_shared<gui::Button>(context);

        //use the custom fuction and add the i texture to that button 
        button->SetCustomIcon(context.textures->Get(TextureID::kLandscape), mapData[i].m_theme_icon);

        //position and scales 
        button->setPosition(sf::Vector2f(viewSize.x / 2.f - 333.f, startY));
        button->setScale({ 6.f, 6.f });

        // saving selection and starting game state
        button->SetCallback([this, type, context]()
            {
                SendMapSelection(static_cast<uint8_t>(type));
                m_gui_container.Deactivate();
                std::cout << "[CLIENT] Vote sent. Waiting for results..." << std::endl;
            });

        m_gui_container.Pack(button);
        //button y spacing 
        startY += 150.f; 
    }
}

void LevelSelectState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    window.draw(m_background_sprite);
    window.draw(m_overlay);
    window.draw(m_gui_container);
}

bool LevelSelectState::Update(sf::Time)
{
    if (m_socket)
    {
        sf::Packet packet;

        while (m_socket->receive(packet) == sf::Socket::Status::Done)
        {
            uint8_t packetType;
            if (packet >> packetType && packetType == static_cast<uint8_t>(Server::PacketType::kInitialState))
            {
                //read the winning map from packet 
                uint8_t winningMap;
                packet >> winningMap;
                //255 wont be the real id so do not go next until have it 
                if (winningMap != 255)
                {
                    std::cout << "[CLIENT] Voting Finished! Winning Map: " << (int)winningMap << std::endl;

                    RequestStackClear();
                    RequestStackPush(StateID::kMultiplayerGame);
                    return false;
                }
            }
        }
    }
    return false;
}

bool LevelSelectState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);

    return false;
}

void LevelSelectState::SendMapSelection(uint8_t mapType)
{
    if (m_socket)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kSelectMap) << mapType;
        m_socket->send(packet);
        //not changing states here anmd wait for the server to send kInitialState back to everyone
    }
}