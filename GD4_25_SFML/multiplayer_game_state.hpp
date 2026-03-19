#pragma once
#include "state.hpp"
#include "world.hpp"
#include "player.hpp"
#include "game_server.hpp"
#include "network_protocol.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Network/Packet.hpp>

class MultiplayerGameState : public State
{
public:
	MultiplayerGameState(StateStack& stack, Context context, bool is_host);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;
	virtual void OnActivate() override;
	virtual void OnDestroy() override;

private:
	void UpdateBroadcastMessage(sf::Time elapsed_time);
	void HandlePacket(uint8_t packetType, sf::Packet& packet);
	void DisableAllRealtimeActions(bool enable);

private:
	World m_world;
	sf::RenderWindow& m_window;
	TextureHolder& m_texture_holder;
	std::map<uint8_t, std::unique_ptr<Player>> m_players;
	std::vector<uint8_t> m_local_player_identifiers;

	bool m_is_host;
	bool m_has_focus;
	bool m_connected;
	bool m_active_state;
	bool m_game_started;
	bool m_scene_initialized;
	//uis
	sf::Time m_client_timeout;
	sf::Time m_time_since_last_packet;
	sf::Clock m_tick_clock;
	sf::Clock m_failed_connection_clock;
	//timings for network
	sf::Text m_broadcast_text;
	sf::Text m_failed_connection_text;
	std::vector<std::string> m_broadcasts;
	sf::Time m_broadcast_elapsed_time;
};

