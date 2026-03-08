#pragma once
#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class LevelSelectState : public State
{
public:
	LevelSelectState(StateStack& stack, Context context, bool isHost);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	void SendMapSelection(uint8_t mapType);

private:
	sf::Sprite m_background_sprite;
	sf::RectangleShape m_overlay;
	gui::Container m_gui_container;
	// pointer to socket that is owned by tank select state, but needs to be accessed here to send map selection to server
	sf::TcpSocket* m_socket;
	bool m_is_host;
};

