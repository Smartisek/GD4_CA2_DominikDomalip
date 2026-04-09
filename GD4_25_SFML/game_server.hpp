#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <thread>
#include <cstdint>
#include <map>
#include "projectile_type.hpp"


class GameServer
{
public:
	explicit GameServer(sf::Vector2f battlefieldSize);
	~GameServer();

	void NotifyPlayerSpawn(uint8_t tank_identifier);
	void NotifyPlayerRealtimeChange(uint8_t tank_identifier, uint8_t action_identifier, bool action_enabled);
	void NotifyPlayerEvent(uint8_t tank_identifier, int8_t action);

private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		std::vector<uint8_t> m_tank_identifiers; //which tank ID belongs to this connection
		bool m_ready;
		bool m_timed_out;
	};

	// Server's authoritative record of connected players
	struct TankInfo
	{
		// data from lobby (selection menu)
		uint8_t m_tank_type;
		bool m_is_ready;

		// data from game
		sf::Vector2f m_position;
		float m_rotation;
		uint8_t m_hitpoints;
		uint8_t m_current_ammo;
		uint8_t m_missile_ammo;
		float stamina;
		std::map<uint8_t, bool> m_real_time_actions;
		uint8_t m_map_vote;
		sf::Vector2f m_velocity;
		uint8_t m_collision_cooldown;
	};

	struct PickupInfo
	{
		uint8_t m_type;
		sf::Vector2f m_position;
		uint16_t m_pikcup_identifier;
	};

	struct ProjectileInfo
	{
		ProjectileType m_type;
		sf::Vector2f m_position;
		sf::Vector2f m_velocity;
		uint8_t m_owner_id;
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;

	// Server Loop
	void SetListening(bool enable);
	void ExecutionThread();
	void Tick(); //physics and game logic update in here 
	sf::Time Now() const;

	// Network handling 
	void HandleIncomingConnections();
	void HandleDisconnections();
	void HandleIncomingPackets();
	void HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);

	// Broadcast packets to clients 
	void BroadcastMessage(const std::string& message);
	void SendToAll(sf::Packet& packet);
	void UpdateClientState();
	void InformWorldState(sf::TcpSocket& socket);

	// Logic for lobby tank and map selection
	void CheckIfAllReady();
	void BroadcastLobbyUpdate();
	void CheckIfMapVotingDone();

	//pikcups functions
	void SpawnPickup();
	void HandlePickupCollisions();

	//projectile logics 
	void SpawnProjectile(uint8_t tank_identifier);
	void UpdateProjectiles(float dt);

	//tank to tank collision
	void HandleTankCollisions(float dt);

	void UpdateTurret(float dt);

private:
	//networking variables 
	sf::Clock m_clock;
	sf::TcpListener m_listener_socket;
	bool m_listening_state;
	sf::Time m_client_timeout;
	bool m_waiting_thread_end;

	// Game and lobby state
	std::size_t m_max_connected_players;
	std::size_t m_connected_players;
	bool m_game_started;
	uint8_t m_selected_map;

	// Entity data
	sf::Vector2f m_battlefield_size;
	uint8_t m_tank_identifier_counter;

	std::map<uint8_t, TankInfo> m_tank_info;
	std::vector<PeerPtr> m_peers;
	std::thread m_thread;

	//pickups
	std::vector<PickupInfo> m_active_pickups;
	uint16_t m_pickup_id_counter = 0;
	sf::Time m_pickup_spawn_timer;

	//projectiles and turret 
	std::vector<ProjectileInfo> m_projectiles;
	sf::Vector2f m_turret_position;
	sf::Time m_turret_fire_cooldown;
	float m_turret_rotation;

};

