#pragma once
#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class LevelSelectState : public State
{
public:
	LevelSelectState(StateStack& stack, Context context);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	sf::Sprite m_background_sprite;
	sf::RectangleShape m_overlay;
	gui::Container m_gui_container;
};

