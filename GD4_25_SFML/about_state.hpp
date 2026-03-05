#pragma once
#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

class AboutState : public State
{
public:
	AboutState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	sf::Text  m_text;
	sf::Sprite m_background_sprite;
	sf::RectangleShape m_overlay;
	gui::Container m_gui_container;
};

