#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include <SFML/Graphics/Sprite.hpp>

class Obstacle : public Entity
{

public:
	Obstacle(const TextureHolder& textures, int hitpoints);

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;
	virtual bool IsDestroyed() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	sf::Sprite m_sprite;
};

