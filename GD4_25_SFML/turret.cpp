#include "turret.hpp"
#include "utility.hpp"
#include "projectile.hpp"
#include "receiver_categories.hpp"
#include <cmath>
#include "data_tables.hpp"
#include <iostream>
#include "sound_node.hpp"

namespace
{
	const std::vector<TurretData> Table = InitializeTurretData();
}

Turret::Turret(TurretType type, const TextureHolder& textures)
	: Entity(Table[static_cast<int>(type)].m_hitpoints, 0,0,0,0)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
	, m_range(Table[static_cast<int>(type)].m_range)
	, m_fire_interval(Table[static_cast<int>(type)].m_fire_interval)
	, m_bullet_type(Table[static_cast<int>(type)].m_bullet_type)
	, m_fire_countdown(sf::Time::Zero)
	, m_has_targets(false)
	, m_show_fire_animation(false)
	, m_fire_animation(textures.Get(TextureID::kTankFireAnim))
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
{
	m_fire_animation.SetFrameSize(sf::Vector2i(256, 256));
	m_fire_animation.setScale({ 2.f,2.f });
	m_fire_animation.SetNumFrames(4);
	m_fire_animation.SetDuration(sf::seconds(0.2f));

	m_explosion.SetFrameSize(sf::Vector2i(256, 256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));

	//offset for the animation
	m_fire_animation.setPosition(sf::Vector2f(0.f, -240.f));

	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_fire_animation);
	Utility::CentreOrigin(m_explosion);

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time)
		{
			this->CreateProjectile(node, textures);
		};
}

unsigned int Turret::GetCategory() const
{
	return static_cast<int>(ReceiverCategories::kEnemy);
}

sf::FloatRect Turret::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

void Turret::UpdateTarget(sf::Vector2f targetPosition)
{
	sf::Vector2f pos = getPosition();
	float distance = Utility::Length(targetPosition - pos);

	if (distance < m_range)
	{
		m_has_targets = true;
		m_target_pos = targetPosition;
	}
	else
	{
		m_has_targets = false;
	}
}

void Turret::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		m_explosion.Update(dt);
		if (!m_explosion_began)
		{
			m_explosion_began = true;
		}
		return;
	}

	if (m_has_targets)
	{
		sf::Vector2f direction = m_target_pos - getPosition();
		float angle = Utility::ToDegrees(std::atan2(direction.y, direction.x));

		setRotation(sf::degrees(angle+90.f));
	}

	if (m_show_fire_animation)
	{
		m_fire_animation.Update(dt);
		if (m_fire_animation.IsFinished())
		{
			m_show_fire_animation = false;
		}
	}

	CheckProjectileLaunch(dt, commands);
}

void Turret::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else if (!IsDestroyed())
	{
		target.draw(m_sprite, states);

		if (m_show_fire_animation)
		{
			target.draw(m_fire_animation, states);
		}
	}
}

void Turret::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
	}
	//if is ready and has target
	if (m_has_targets && m_fire_countdown <= sf::Time::Zero)
	{
		commands.Push(m_fire_command);
		m_fire_countdown = m_fire_interval;
		

		m_show_fire_animation = true;
		m_fire_animation.Restart();
		SoundEffect soundEffect = SoundEffect::kTurretFire;
		PlayLocalSound(commands, soundEffect);
	}
}

void Turret::CreateProjectile(SceneNode& node, const TextureHolder& textures) const
{
	float rotation = getRotation().asDegrees();
	double radians = Utility::toRadians(rotation - 90.f);

	// 1. Calculate the direction vector based on rotation
	sf::Vector2f direction(std::cos(radians), std::sin(radians));

	// 2. Use direction for the spawn offset
	sf::Vector2f offset = direction * 60.f;
	sf::Vector2f spawnPos = getPosition() + offset;

	std::unique_ptr<Projectile> projectile(new Projectile(m_bullet_type, textures, ReceiverCategories::kEnemyProjectile, -2));
	projectile->setPosition(spawnPos);
	projectile->setRotation(sf::degrees(rotation));

	// 3. APPLY VELOCITY (This makes the bullet move!)
	projectile->SetVelocity(direction * projectile->GetMaxSpeed());

	node.AttachChild(std::move(projectile));
	std::cout << "Bullet fired" << std::endl;
}

void Turret::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
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

bool Turret::IsMarkedForRemoval() const
{
	//same as tank class
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}