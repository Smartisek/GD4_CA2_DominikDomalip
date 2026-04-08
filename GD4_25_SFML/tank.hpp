#pragma once
#include "entity.hpp"
#include "text_node.hpp"
#include "sprite_node.hpp"
#include "resource_identifiers.hpp"
#include "tank_type.hpp"
#include "animation.hpp"
#include "emitter_node.hpp"
#include <array>

class Tank : public Entity
{
public:

	Tank(int identifier, TankType type, const TextureHolder& textures, const FontHolder& fonts);
	
	virtual unsigned int GetCategory() const override;

	//more functions will be added later
	void CreateBullet(SceneNode& node, const TextureHolder& textures) const;
	void Fire();
	float GetSpeed() const;

	// cooldown for bumping into other tanks
	bool CanBeDamaged() const;
	void ResetCollisionCooldown();
	void ReduceCollisionCooldown(sf::Time dt);
	sf::FloatRect GetBoundingRect() const;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	bool IsMarkedForRemoval() const override;

	//functions for pickup
	virtual void Repair(int points) override;
	void Reload(int amount);
	int GetAmmoCount() const;
	void CollectMissile(unsigned int amount);
	int GetMissileAmmo() const;

	//Network related 
	int GetIdentifier() const;
	void SetIdentifier(int identifier);
	void SetHitpoints(int points);
	void SetAmmo(int ammo);
	void SetMissileAmmo(uint8_t ammo);
	void SetStamina(float stamina);


private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	void UpdateHealthBar();
	void UpdateStaminaBar();
	void UpdateMovementAnimation(sf::Time dt);
	void UpdateUI();
	void UpdatePlayerColor();

private:
	TankType m_type;
	sf::Sprite m_sprite; //body for tank
	sf::Sprite m_outline_sprite; 
	float m_outline_scale;
	ReceiverCategories m_category; // what ccategory this tank belongs to
	const TextureHolder& m_textures;

	//fire control
	bool m_is_firing;
	sf::Time m_fire_countdown;
	unsigned int m_fire_rate;
	Command m_fire_command;
	int m_missile_ammo;
	bool m_next_shot_missile;

	sf::Time m_collision_cooldown;

	sf::RectangleShape m_health_bar_background;
	sf::RectangleShape m_health_bar_foreground;
	int m_max_hitpoints;

	sf::RectangleShape m_stamina_bar_background;
	sf::RectangleShape m_stamina_bar_foreground;

	sf::Sprite m_ammo_icon;
	sf::Text m_ammo_text;

	Animation m_explosion;
	bool m_show_explosion;
	bool m_explosion_began;

	Animation m_fire_animation;
	bool m_show_fire_animation;

	int m_current_ammo;
	float m_anim_timer;

	int m_identifier;
};

