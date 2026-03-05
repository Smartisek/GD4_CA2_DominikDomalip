#include "Entity.hpp"
#include "command_queue.hpp"

Entity::Entity(int hitpoints, float maxStamina, float drainRate, float rechargeRate, float sprintMultiplier) 
	:m_hitpoints(hitpoints)
	, m_max_stamina(maxStamina)
	, m_stamina(maxStamina)
	, m_is_sprinting(false)
	, m_stamina_drain_rate(drainRate)
	, m_stamina_recharge_rate(rechargeRate)
	, m_stamina_multiplier(sprintMultiplier)
{
}

void Entity::SetVelocity(sf::Vector2f velocity)
{
	m_velocity = velocity;
}

void Entity::SetVelocity(float vx, float vy)
{
	m_velocity.x = vx;
	m_velocity.y = vy;
}

sf::Vector2f Entity::GetVelocity() const
{
	return m_velocity;
}

void Entity::Accelerate(sf::Vector2f velocity)
{
	m_velocity += velocity;
}

void Entity::Accelerate(float vx, float vy)
{
	m_velocity.x += vx;
	m_velocity.y += vy;
}

int Entity::GetHitPoints() const
{
	return m_hitpoints;
}

void Entity::Repair(int points)
{
	assert(points > 0);
	m_hitpoints += points;
}

void Entity::Damage(int points)
{
	assert(points > 0);
	m_hitpoints -= points;
}

void Entity::Destroy()
{
	m_hitpoints = 0;
}

bool Entity::IsDestroyed() const
{
	return m_hitpoints <= 0;
}

void Entity::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	float multiplier = 1.0f;

	// Use the member multiplier we stored earlier
	// Only apply if the flag is true and we have stamina
	if (m_is_sprinting && m_stamina > 0.f)
	{
		multiplier = m_stamina_multiplier;
	}

	move(m_velocity * multiplier * dt.asSeconds());

	m_is_sprinting = false;
}

void Entity::Sprint()
{
	if (m_stamina > 0.f)
	{
		m_is_sprinting = true;
	}
}

bool Entity::IsSprinting() const
{
	return m_is_sprinting;
}

float Entity::GetStaminaRatio() const
{
	return m_stamina / m_max_stamina;
}

void Entity::UpdateStamina(sf::Time dt)
{
	bool isMoving = (m_velocity.x > 0.3f || m_velocity.y > 0.3f);
	if (m_is_sprinting && isMoving && m_stamina > 0.f)
	{
		m_stamina -= m_stamina_drain_rate * dt.asSeconds();
	}
	else
	{
		m_stamina += m_stamina_recharge_rate * dt.asSeconds();
	}

	m_stamina = std::clamp(m_stamina, 0.f, m_max_stamina);
}