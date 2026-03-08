#pragma once
#include "state.hpp"
#include "container.hpp"
#include "game_server.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "network_protocol.hpp"
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <vector>
#include <map>
#include <memory>

class TankSelectState : public State
{
public:
    TankSelectState(StateStack& stack, Context context, bool is_host = false);

    virtual void Draw() override;
    virtual bool Update(sf::Time dt) override;
    virtual bool HandleEvent(const sf::Event& event) override;

private:
    void HandlePacket(uint8_t packetType, sf::Packet& packet);
    void SendTankSelection(uint8_t tankType);
    void SendReadyToggle();
    void SendMapSelection(uint8_t mapType);

private:
    sf::Sprite m_background_sprite;
    sf::RectangleShape m_dark_overlay;
    gui::Container m_gui_container;
    sf::Text m_title_text;
	sf::Text m_instruction_text;
    std::vector<sf::Text> m_stat_texts;

    std::unique_ptr<GameServer> m_server;
    sf::TcpSocket m_socket;
	bool m_is_host;
    bool m_connected;
    uint8_t m_player_id;
    //used for showing on screen when in loby
    struct PlayerLobbyData
    {
		uint8_t tank_type;
		bool is_ready;
    };

    std::map<uint8_t, PlayerLobbyData> m_players;
	uint8_t m_selected_map;
    const sf::Time m_keepalive_interval = sf::seconds(1.f);
    sf::Clock m_keepalive_clock;
};

