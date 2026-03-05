#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"
#include "music_player.hpp"
#include "sound_player.hpp"
#include "map_type.hpp"
#include "tank_type.hpp"

class Application
{
public:
	Application();
	void Run();

private:
	void ProcessInput();
	void Update(sf::Time dt);
	void Render();
	void RegisterStates();

private:
	sf::RenderWindow m_window;
	Player m_player{ ReceiverCategories::kPlayer1Tank };
	Player m_player2{ ReceiverCategories::kPlayer2Tank };

	TextureHolder m_textures;
	FontHolder m_fonts;

	MapType m_current_map;
	StateStack m_stack;

	MusicPlayer m_music;
	SoundPlayer m_sound;

	TankType m_p1_tank_choice;
	TankType m_p2_tank_choice;

	int m_winner_index;
};

