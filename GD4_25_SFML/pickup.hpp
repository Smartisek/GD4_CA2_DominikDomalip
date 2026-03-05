#pragma once
#include "entity.hpp"
#include "command.hpp"
#include "resource_identifiers.hpp"
#include "pickup_type.hpp"
#include "popup_type.hpp"

class Tank; // Forward declaration

class Pickup : public Entity
{
public:
	Pickup(PickupType type, const TextureHolder& textures);

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;

	void Apply(Tank& player) const;
	PopupType GetPopupType() const;
	std::string GetPopupText() const;

protected:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	PickupType m_type;
	sf::Sprite m_sprite;
};