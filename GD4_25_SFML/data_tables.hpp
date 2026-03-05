#pragma once
#include "texture_id.hpp"
#include <SFML/System/Time.hpp>
#include <functional>
#include <SFML/Graphics/Color.hpp>
#include "animation.hpp"
#include "pickup_type.hpp"
#include "tank.hpp"
#include "projectile_type.hpp"
#include "popup_type.hpp"

struct Direction
{
	Direction(float angle, float distance)
		: m_angle(angle), m_distance(distance) {
	}
	float m_angle;
	float m_distance;
};


struct TankData
{
	int m_hitpoints;
	float m_speed;
	TextureID m_texture;
	sf::IntRect m_texture_rect;
	sf::Time m_fire_interval;
	std::vector<Direction> m_directions;
	float m_max_stamina;
	float m_sprint_multiplier;
	float m_drain_rate;
	float m_recharge_rate;
	int m_ammo_amount;
};

struct ProjectileData
{
	int m_damage;
	float m_speed;
	TextureID m_texture;

};

struct ParticleData
{
	sf::Color m_color;
	sf::Time m_lifetime;
};

struct PickupData
{
	TextureID m_texture;
	std::function<void(Tank&)> m_action;
	PopupType m_popup_type;
	std::string m_popup_text;
};

struct MapData
{
	sf::IntRect m_texture_rect;
	sf::IntRect m_theme_icon;
	std::vector<sf::Vector2f> m_obstacle_positions;
};

struct TurretData
{
	int m_hitpoints;
	float m_range;
	sf::Time m_fire_interval;
	TextureID m_texture;
	ProjectileType m_bullet_type;
};

struct PopupData
{
	sf::Color	m_color;
	sf::Time	m_lifetime;
	float		m_speed;
};

std::vector<ProjectileData> InitializeProjectileData();
std::vector<TankData> InitializeTankData();
std::vector<ParticleData> InitializeParticleData();
std::vector<PickupData> InitializePickupData();
std::vector<MapData> InitializeMapData();
std::vector<TurretData> InitializeTurretData();
std::vector<PopupData> InitializePopupData();




