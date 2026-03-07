#include "game_state.hpp"
#include "mission_status.hpp"
#include "map_type.hpp"
#include "world.hpp"
#include <iostream>

GameState::GameState(StateStack& stack, Context context) 
	: State(stack, context)
	, m_world(*context.window, *context.fonts, *context.sound, false)
	, m_player(nullptr, 1, context.keys1)
{
	m_world.AddTank(1, TankType::kTank1);
	context.music->Play(MusicThemes::kGameTheme);
}

void GameState::Draw()
{
	m_world.Draw();
}

bool GameState::Update(sf::Time dt)
{

	m_world.Update(dt);

	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleRealTimeInput(commands);

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

	//Escape should bring up the pause menu
	const auto* keypress = event.getIf<sf::Event::KeyPressed>();
	if(keypress && keypress->scancode == sf::Keyboard::Scancode::Escape)
	{
		RequestStackPush(StateID::kPause);
	}
	return true;
}


