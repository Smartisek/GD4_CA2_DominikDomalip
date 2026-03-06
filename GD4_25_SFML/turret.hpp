#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include "turret_type.hpp"
#include "projectile_type.hpp"
#include "animation.hpp"


class Turret : public Entity
{
public:
	Turret(TurretType type, const TextureHolder& textures);

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);
	void UpdateTarget(sf::Vector2f targetPosition);
	virtual bool IsMarkedForRemoval() const override;
private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	void CreateProjectile(SceneNode& node, const TextureHolder& textures) const;

private:
	TurretType m_type;
	sf::Sprite m_sprite;

	float m_range;
	sf::Time m_fire_interval;
	ProjectileType m_bullet_type;

	Command m_fire_command;
	sf::Vector2f m_target_pos; //position for nearest player
	sf::Time m_fire_countdown;
	bool m_has_targets; //to check if anyone is close

	Animation m_fire_animation;
	bool m_show_fire_animation;

	Animation m_explosion;
	bool m_show_explosion;
	bool m_explosion_began;
};

