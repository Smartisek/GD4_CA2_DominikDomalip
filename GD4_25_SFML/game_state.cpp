#include "game_state.hpp"
#include "mission_status.hpp"
#include "map_type.hpp"
#include "world.hpp"
#include <iostream>

GameState::GameState(StateStack& stack, Context context) : State(stack, context), m_world(*context.window, *context.fonts, *context.sound, *context.currentMap, *context.p1Tank, *context.p2Tank), m_player(*context.player), m_player2(*context.player2), m_game_over_delay(sf::Time::Zero), m_is_game_over_triggered(false)
{
	context.music->Play(MusicThemes::kGameTheme);
}

void GameState::Draw()
{
	m_world.Draw();
}

bool GameState::Update(sf::Time dt)
{

	m_world.Update(dt);

	if (m_world.HasPlayer1Won() || m_world.HasPlayer2Won())
	{
		m_game_over_delay += dt;
		//wait 1 sec before switching 
		if (!m_is_game_over_triggered && m_game_over_delay > sf::seconds(1.0f))
		{

			if (m_world.HasPlayer1Won())
				*GetContext().winnerIndex = 1;
			else
				*GetContext().winnerIndex = 2;

			std::cout << "Game over now!" << std::endl;
			RequestStackPush(StateID::kGameOver);
			m_is_game_over_triggered = true;
		}
	}

		CommandQueue& commands = m_world.GetCommandQueue();
		m_player.HandleRealTimeInput(commands);
		m_player2.HandleRealTimeInput(commands);
	return true;
}

bool GameState::HandleEvent(const sf::Event& event)
{
	if (m_is_game_over_triggered)
	{
		return true; //no need for handling events
	}

	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleEvent(event, commands);
	m_player2.HandleEvent(event, commands); 

	//Escape should bring up the pause menu
	const auto* keypress = event.getIf<sf::Event::KeyPressed>();
	if(keypress && keypress->scancode == sf::Keyboard::Scancode::Escape)
	{
		RequestStackPush(StateID::kPause);
	}
	return true;
}


