#pragma once
#include "scene_node.hpp"

class Entity : public SceneNode
{
public:
	Entity(int hitpoints, float maxStamina, float drainRate, float rechargeRate, float sprintMultiplier);
	void SetVelocity(sf::Vector2f velocity);
	void SetVelocity(float vx, float vy);
	sf::Vector2f GetVelocity() const;
	void Accelerate(sf::Vector2f velocity);
	void Accelerate(float vx, float vy);

	int GetHitPoints() const;
	virtual void Repair(int points);
	void Damage(int points);
	void Destroy();
	virtual bool IsDestroyed() const override;

	void Sprint();
	bool IsSprinting() const;
	float GetStaminaRatio() const;

protected:
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	void UpdateStamina(sf::Time dt);
	int m_hitpoints;

private:
	sf::Vector2f m_velocity;

	float m_stamina;
	float m_max_stamina;
	bool m_is_sprinting;
	float m_stamina_drain_rate;
	float m_stamina_recharge_rate;
	float m_stamina_multiplier;
};

