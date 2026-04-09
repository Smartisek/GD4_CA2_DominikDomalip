#include "application.hpp"
#include "constants.hpp"
#include "fontid.hpp"
#include "game_state.hpp"
#include "menu_state.hpp"
#include "title_state.hpp"
#include "pause_state.hpp"
#include "settings_state.hpp"
#include "level_select_state.hpp"
#include "map_type.hpp"
#include "tank_select_state.hpp"
#include "game_over_state.hpp"
#include "about_state.hpp"
#include "multiplayer_game_state.hpp"


Application::Application() 
	: m_window(sf::VideoMode::getDesktopMode(), "Armored Assault", sf::Style::Close)
	, m_textures()
	, m_fonts()
	, m_music()
	, m_sound()
	, m_keys1(1) // 1 means Player 1 defaults
	, m_keys2(2) // 2 means Player 2 defaults
	, m_multiplayer_host(false)
	, m_socket()
	, m_stack(State::Context(m_window, m_textures, m_fonts, m_music, m_sound, m_keys1, m_keys2, m_socket, m_server, m_local_id, m_game_over_message))
{
	m_window.setKeyRepeatEnabled(false);
	m_window.setVerticalSyncEnabled(true);
	m_fonts.Load(FontID::kMain, "Media/Fonts/Sansation.ttf");
	m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
	m_textures.Load(TextureID::kButtons, "Media/Textures/Buttons.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/Backgrounds.png");
	m_textures.Load(TextureID::kTankBody, "Media/Textures/Hull1.png");
	m_textures.Load(TextureID::kTankBody2, "Media/Textures/Hull2.png");
	m_textures.Load(TextureID::kTankBody3, "Media/Textures/Hull3.png");
	m_textures.Load(TextureID::kTankBody4, "Media/Textures/Hull4.png");
	RegisterStates();
	m_stack.PushState(StateID::kTitle);
	
}

void Application::Run()
{
	sf::Clock clock;
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	sf::Time time_since_last_update = sf::Time::Zero;
	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInput();
			Update(sf::seconds(kTimePerFrame));

			if (m_stack.IsEmpty())
			{
				m_window.close();
			}
		}
		Render();
	}
}

void Application::ProcessInput()
{
	while (const std::optional event = m_window.pollEvent())
	{
		m_stack.HandleEvent(*event);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}

	}
}

void Application::Update(sf::Time dt)
{
	m_stack.Update(dt);
}

void Application::Render()
{
	m_window.clear();
	m_stack.Draw();
	m_window.display();
}

void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	//lobby states 
	m_stack.RegisterState<TankSelectState>(StateID::kHostGame, true);
	m_stack.RegisterState<TankSelectState>(StateID::kJoinGame, false);

	m_stack.RegisterState<GameState>(StateID::kGame);
	m_stack.RegisterState<AboutState>(StateID::kAbout);
	m_stack.RegisterState<PauseState>(StateID::kPause);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
	m_stack.RegisterState<TankSelectState>(StateID::kTankSelect, true);
	m_stack.RegisterState<LevelSelectState>(StateID::kLevelSelect, true);
	//multiplayer states 
	m_stack.RegisterState<MultiplayerGameState>(StateID::kMultiplayerHost, true); 
	m_stack.RegisterState<MultiplayerGameState>(StateID::kMultiplayerJoin, false); 
	m_stack.RegisterState<GameOverState>(StateID::kGameOver);
}


