#include "obstacle.hpp"
#include "utility.hpp"
#include "receiver_categories.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include "sound_node.hpp"

Obstacle::Obstacle(const TextureHolder& textures, int hitpoints)
	: Entity(hitpoints, 0, 0, 0, 0)
	, m_sprite(textures.Get(TextureID::kWall))
{
	Utility::CentreOrigin(m_sprite);
}

unsigned int Obstacle::GetCategory() const
{
	return static_cast<int>(ReceiverCategories::kObstacle);
}

sf::FloatRect Obstacle::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Obstacle::IsDestroyed() const
{
	return GetHitPoints() <= 0;
}

void Obstacle::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}

void Obstacle::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});
	commands.Push(command);
}