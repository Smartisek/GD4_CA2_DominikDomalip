#pragma once
#include "scene_node.hpp"
#include "resource_holder.hpp"
#include <SFML/Graphics/Text.hpp>
#include "resource_identifiers.hpp"
#include "popup_type.hpp"

class PopupText : public SceneNode
{
public:
	PopupText(PopupType type, const std::string& text, const FontHolder& fonts);
	void SetColor(sf::Color color);
	void SetText(const std::string& text);

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
private:
	sf::Time m_lifetime;
	sf::Text m_text;
	PopupType m_type;
};

