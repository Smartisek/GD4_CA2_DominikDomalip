#include "popup_text.hpp"
#include "utility.hpp"
#include <SFML/Graphics.hpp>
#include "data_tables.hpp"

namespace
{
	const std::vector<PopupData> Table = InitializePopupData();
}

PopupText::PopupText(PopupType type, const std::string& text, const FontHolder& fonts) 
	: m_lifetime(Table[static_cast<int>(type)].m_lifetime)
	, m_text(fonts.Get(FontID::kMain))
	, m_type(type)
{
	m_text.setFillColor(Table[static_cast<int>(type)].m_color);
	m_text.setCharacterSize(30);
	m_text.setString(text);
	Utility::CentreOrigin(m_text);
}

void PopupText::SetColor(sf::Color color)
{
	m_text.setFillColor(color);
}

void PopupText::SetText(const std::string& text)
{
	m_text.setString(text);
	Utility::CentreOrigin(m_text);
}

void PopupText::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_text, states);
}

void PopupText::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	m_lifetime -= dt;

	if (m_lifetime <= sf::Time::Zero)
	{
		//if lifetime is up just destroy and return 
		Destroy();
		return;
	}
	//move up at a speed of 50 pixels per second
	float speed = Table[static_cast<int>(m_type)].m_speed;
	move(sf::Vector2f(0.f, speed * dt.asSeconds()));

	//fade logic
	sf::Color color = m_text.getFillColor();
	float max_life = Table[static_cast<int>(m_type)].m_lifetime.asSeconds();
	// alpha on percentage of lifetime left
	float alphaValue = std::max(0.f, 255.f * (m_lifetime.asSeconds() / max_life));
	color.a = static_cast<std::uint8_t>(alphaValue);

}