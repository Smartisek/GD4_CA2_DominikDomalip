#include "pickup.hpp"
#include "data_tables.hpp"
#include "utility.hpp"

namespace
{
	const std::vector<PickupData> Table = InitializePickupData();
}

Pickup::Pickup(PickupType type, const TextureHolder& textures)
	: Entity(1,1,1,1,1) // 1 HP (doesn't matter)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
{
	Utility::CentreOrigin(m_sprite);
}

unsigned int Pickup::GetCategory() const
{
	return static_cast<int>(ReceiverCategories::kPickup);
}

sf::FloatRect Pickup::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

void Pickup::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}

void Pickup::Apply(Tank& player) const
{
	Table[static_cast<int>(m_type)].m_action(player);
}

PopupType Pickup::GetPopupType() const {
	return Table[static_cast<int>(m_type)].m_popup_type;
}

std::string Pickup::GetPopupText() const {
	return Table[static_cast<int>(m_type)].m_popup_text;
}