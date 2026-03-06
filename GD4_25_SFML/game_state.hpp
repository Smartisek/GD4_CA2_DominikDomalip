#pragma once
#include "State.hpp"
#include "world.hpp"
#include "player.hpp"

class GameState : public State
{
public:
	GameState(StateStack& stack, Context context);
	virtual bool Update(sf::Time delta_time) override;
	virtual void Draw() override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	World m_world;
	Player m_player;
	sf::Time m_game_over_delay; //since i got animation for tank explosion, i need a delay before going to game over state 
	bool m_is_game_over_triggered;
};

