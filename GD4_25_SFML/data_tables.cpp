#include "data_tables.hpp"
#include "projectile_type.hpp"
#include "tank_type.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "particle_type.hpp"
#include "constants.hpp"
#include "map_type.hpp"
#include "turret_type.hpp"

std::vector<TankData> InitializeTankData()
{
	std::vector<TankData> data(static_cast<int>(TankType::kTankCount));
	data[static_cast<int>(TankType::kTank1)].m_hitpoints = 150;
	data[static_cast<int>(TankType::kTank1)].m_speed = 70.f;
	data[static_cast<int>(TankType::kTank1)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(TankType::kTank1)].m_texture = TextureID::kTankBody;
	data[static_cast<int>(TankType::kTank1)].m_texture_rect = sf::IntRect({ 0, 0 }, { 159, 249 });
	data[static_cast<int>(TankType::kTank1)].m_max_stamina = 120.f;
	data[static_cast<int>(TankType::kTank1)].m_sprint_multiplier = 1.5f;
	data[static_cast<int>(TankType::kTank1)].m_drain_rate = 40.f;
	data[static_cast<int>(TankType::kTank1)].m_recharge_rate = 20.f;
	data[static_cast<int>(TankType::kTank1)].m_ammo_amount = 11;

	data[static_cast<int>(TankType::kTank2)].m_hitpoints = 130;
	data[static_cast<int>(TankType::kTank2)].m_speed = 85.f;
	data[static_cast<int>(TankType::kTank2)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(TankType::kTank2)].m_texture = TextureID::kTankBody2;
	data[static_cast<int>(TankType::kTank2)].m_texture_rect = sf::IntRect({ 0, 0 }, { 166, 247 });
	data[static_cast<int>(TankType::kTank2)].m_max_stamina = 95.f;
	data[static_cast<int>(TankType::kTank2)].m_sprint_multiplier = 1.5f;
	data[static_cast<int>(TankType::kTank2)].m_drain_rate = 40.f;
	data[static_cast<int>(TankType::kTank2)].m_recharge_rate = 20.f;
	data[static_cast<int>(TankType::kTank2)].m_ammo_amount = 12;

	data[static_cast<int>(TankType::kTank3)].m_hitpoints = 85;
	data[static_cast<int>(TankType::kTank3)].m_speed = 100.f;
	data[static_cast<int>(TankType::kTank3)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(TankType::kTank3)].m_texture = TextureID::kTankBody3;
	data[static_cast<int>(TankType::kTank3)].m_texture_rect = sf::IntRect({ 0, 0 }, { 131, 247 });
	data[static_cast<int>(TankType::kTank3)].m_max_stamina = 130.f;
	data[static_cast<int>(TankType::kTank3)].m_sprint_multiplier = 1.5f;
	data[static_cast<int>(TankType::kTank3)].m_drain_rate = 40.f;
	data[static_cast<int>(TankType::kTank3)].m_recharge_rate = 20.f;
	data[static_cast<int>(TankType::kTank3)].m_ammo_amount = 15;

	data[static_cast<int>(TankType::kTank4)].m_hitpoints = 130;
	data[static_cast<int>(TankType::kTank4)].m_speed = 70.f;
	data[static_cast<int>(TankType::kTank4)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(TankType::kTank4)].m_texture = TextureID::kTankBody4;
	data[static_cast<int>(TankType::kTank4)].m_texture_rect = sf::IntRect({ 0, 0 }, { 114, 240 });
	data[static_cast<int>(TankType::kTank4)].m_max_stamina = 100.f;
	data[static_cast<int>(TankType::kTank4)].m_sprint_multiplier = 1.5f;
	data[static_cast<int>(TankType::kTank4)].m_drain_rate = 30.f;
	data[static_cast<int>(TankType::kTank4)].m_recharge_rate = 20.f;
	data[static_cast<int>(TankType::kTank4)].m_ammo_amount = 15;
	return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
	std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));
	data[static_cast<int>(ProjectileType::kTank1Bullet)].m_damage = 20;
	data[static_cast<int>(ProjectileType::kTank1Bullet)].m_speed = 1000;
	data[static_cast<int>(ProjectileType::kTank1Bullet)].m_texture = TextureID::kBullet;


	data[static_cast<int>(ProjectileType::kTank2Bullet)].m_damage = 30;
	data[static_cast<int>(ProjectileType::kTank2Bullet)].m_speed = 1000;
	data[static_cast<int>(ProjectileType::kTank2Bullet)].m_texture = TextureID::kGrenade;

	data[static_cast<int>(ProjectileType::kTank3Bullet)].m_damage = 20;
	data[static_cast<int>(ProjectileType::kTank3Bullet)].m_speed = 1000;
	data[static_cast<int>(ProjectileType::kTank3Bullet)].m_texture = TextureID::kBullet;

	data[static_cast<int>(ProjectileType::kTank4Bullet)].m_damage = 30;
	data[static_cast<int>(ProjectileType::kTank4Bullet)].m_speed = 1000;
	data[static_cast<int>(ProjectileType::kTank4Bullet)].m_texture = TextureID::kGrenade;

	data[static_cast<int>(ProjectileType::kTurretBullet)].m_damage = 40;
	data[static_cast<int>(ProjectileType::kTurretBullet)].m_speed = 800;
	data[static_cast<int>(ProjectileType::kTurretBullet)].m_texture = TextureID::kTurretPlasma;

	data[static_cast<int>(ProjectileType::kMissile)].m_damage = 60;
	data[static_cast<int>(ProjectileType::kMissile)].m_speed = 1000;
	data[static_cast<int>(ProjectileType::kMissile)].m_texture = TextureID::kMissile;

	return data;
}

std::vector<ParticleData> InitializeParticleData()
{
	std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

	data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
	data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.5f);

	data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
	data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(2.5f);

	data[static_cast<int>(ParticleType::kTurretTrace)].m_color = sf::Color(173, 216, 230); 
	data[static_cast<int>(ParticleType::kTurretTrace)].m_lifetime = sf::seconds(0.3f);

	return data;
}

std::vector<PickupData> InitializePickupData()
{
	std::vector<PickupData> data(static_cast<int>(PickupType::kTypeCount));
	data[static_cast<int>(PickupType::kHealthRefill)].m_texture = TextureID::kHealthRefill;
	data[static_cast<int>(PickupType::kHealthRefill)].m_popup_type = PopupType::kHealth;
	data[static_cast<int>(PickupType::kHealthRefill)].m_popup_text = "+30 HP";
	data[static_cast<int>(PickupType::kHealthRefill)].m_action = [](Tank& t) {
		t.Repair(30);
		};

	data[static_cast<int>(PickupType::kBulletRefill)].m_texture = TextureID::kBulletRefill;
	data[static_cast<int>(PickupType::kBulletRefill)].m_popup_type = PopupType::kAmmo;
	data[static_cast<int>(PickupType::kBulletRefill)].m_popup_text = "+5 Ammo";
	data[static_cast<int>(PickupType::kBulletRefill)].m_action = [](Tank& t) {
		t.Reload(5);
		};

	data[static_cast<int>(PickupType::kMissile)].m_texture = TextureID::kMissileRefill;
	data[static_cast<int>(PickupType::kMissile)].m_popup_type = PopupType::kAmmo;
	data[static_cast<int>(PickupType::kMissile)].m_popup_text = "+MISSILE!";
	data[static_cast<int>(PickupType::kMissile)].m_action = [](Tank& t)
		{
			t.CollectMissile(1);
		};


	return data;
}

std::vector<MapData> InitializeMapData()
{
	std::vector<MapData> data(static_cast<int>(MapType::kTypeCount));
	
	std::vector<sf::Vector2f> standardLayout = {
		{1000.f, 1000.f}, {3000.f, 1000.f}, // top positon
		{1000.f, 3000.f}, {3000.f, 3000.f}, // bottom position
		{2000.f, 1200.f}, {2000.f, 2800.f}, // center vertical
		{1200.f, 2000.f}, {2800.f, 2000.f}  // center horizontal
	};

	data[static_cast<int>(MapType::kDesert)].m_texture_rect = sf::IntRect({ 355, 39 }, { 156, 116 }); 
	data[static_cast<int>(MapType::kDesert)].m_theme_icon = sf::IntRect({ 377, 11 }, { 111, 18 });
	data[static_cast<int>(MapType::kDesert)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kGrass)].m_texture_rect = sf::IntRect({ 10, 39 }, { 156, 116 });
	data[static_cast<int>(MapType::kGrass)].m_theme_icon = sf::IntRect({ 32, 11 }, { 111, 18 });
	data[static_cast<int>(MapType::kGrass)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kSavana)].m_texture_rect = sf::IntRect({ 526, 39 }, { 156, 116 });
	data[static_cast<int>(MapType::kSavana)].m_theme_icon = sf::IntRect({ 548, 11 }, { 111, 18 });
	data[static_cast<int>(MapType::kSavana)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kMountains)].m_texture_rect = sf::IntRect({ 10, 203 }, { 156, 116 });
	data[static_cast<int>(MapType::kMountains)].m_theme_icon = sf::IntRect({ 32, 174 }, { 111, 18 });
	data[static_cast<int>(MapType::kMountains)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kRedRock)].m_texture_rect = sf::IntRect({ 182, 203 }, { 156, 116 });
	data[static_cast<int>(MapType::kRedRock)].m_theme_icon = sf::IntRect({ 204, 174 }, { 111, 18 });
	data[static_cast<int>(MapType::kRedRock)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kUnderground)].m_texture_rect = sf::IntRect({ 526, 203 }, { 156, 116 });
	data[static_cast<int>(MapType::kUnderground)].m_theme_icon = sf::IntRect({ 548, 174 }, { 111, 18 });
	data[static_cast<int>(MapType::kUnderground)].m_obstacle_positions = standardLayout;

	data[static_cast<int>(MapType::kBeach)].m_texture_rect = sf::IntRect({ 10, 367 }, { 156, 116 });
	data[static_cast<int>(MapType::kBeach)].m_theme_icon = sf::IntRect({ 32, 339 }, { 111, 18 });
	data[static_cast<int>(MapType::kBeach)].m_obstacle_positions = standardLayout;

	return data;
}
 

std::vector<TurretData> InitializeTurretData()
{
	std::vector<TurretData> data(static_cast<int>(TurretType::kCount));

	data[static_cast<int>(TurretType::kStandard)].m_hitpoints = 120;
	data[static_cast<int>(TurretType::kStandard)].m_range = 700.f;
	data[static_cast<int>(TurretType::kStandard)].m_fire_interval = sf::seconds(2.0f);
	data[static_cast<int>(TurretType::kStandard)].m_texture = TextureID::kTurret;
	data[static_cast<int>(TurretType::kStandard)].m_bullet_type = ProjectileType::kTurretBullet;

	return data;
}

std::vector<PopupData> InitializePopupData()
{
	std::vector<PopupData> data(static_cast<int>(PopupType::kTypeCount));

	data[static_cast<int>(PopupType::kDamage)].m_color = sf::Color::Red;
	data[static_cast<int>(PopupType::kDamage)].m_lifetime = sf::seconds(0.7f);
	data[static_cast<int>(PopupType::kDamage)].m_speed = 120.f;

	data[static_cast<int>(PopupType::kHealth)].m_color = sf::Color::Green;
	data[static_cast<int>(PopupType::kHealth)].m_lifetime = sf::seconds(1.2f);
	data[static_cast<int>(PopupType::kHealth)].m_speed = 60.f;

	data[static_cast<int>(PopupType::kAmmo)].m_color = sf::Color::Cyan;
	data[static_cast<int>(PopupType::kAmmo)].m_lifetime = sf::seconds(1.0f);
	data[static_cast<int>(PopupType::kAmmo)].m_speed = 80.f;

	return data;
}