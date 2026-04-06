#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "command_queue.hpp"
#include "tank.hpp"
#include "sound_player.hpp"
#include "bloom_effect.hpp"
#include "map_type.hpp"
#include <algorithm>
#include "popup_type.hpp"
#include "network_node.hpp"
#include "pickup.hpp"

class World
{
public:
	explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked = false);

	void Update(sf::Time dt);
	void Draw();
	CommandQueue& GetCommandQueue();

	// multiplayer related functions 
	Tank* AddTank(uint8_t identifier, TankType type);
	void RemoveTank(uint8_t identifier);
	Tank* GetTank(uint8_t identifier) const;
	bool HasAlivePlayer() const;
	void SetCurrentMap(MapType map);
	void InitializeScene();

	bool PollGameAction(GameActions::Action& out);
	//spawning pickups might not be ideal as public 
	void CreatePickup(uint8_t type, sf::Vector2f position, uint16_t id);
	void RemovePickup(uint16_t id);
	void CreatePopup(sf::Vector2f position, PopupType type, const std::string& text);

private:
	void LoadTextures();
	void BuildScene();
	void DestroyEntitiesOutsideView();
	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;
	void UpdateSounds();
	void HandleCollisions();
	void HandleTankCollision(Tank& tank1, Tank& tank2);
	void ApplyFriction(sf::Time dt);
	void UpdateView(sf::Time dt);
	void SpawnRandomPickup();
	void CheckOutOfBounds();
	void GuideMissile();
private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	//sf::RenderWindow& m_window;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;

	SceneNode m_scene_graph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	CommandQueue m_command_queue;

	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;

	SoundPlayer& m_sounds;

	BloomEffect m_bloom_effect;
	sf::Time m_pickup_countdown;
	int m_active_pickups;
	MapType m_current_map;
	sf::Time m_win_delay;

	std::vector<Tank*> m_player_tanks;
	NetworkNode* m_network_node;
	bool m_networked_world;

	std::map<uint16_t, Pickup*> m_network_pickups;
};

