#include "tank.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "projectile.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include <iostream>
#include <cmath>
#include "sound_node.hpp"
#include "network_node.hpp"

namespace
{
	const std::vector<TankData> Table = InitializeTankData();

	const std::array<sf::Color, 8> kPlayerColors =
	{
		sf::Color(255, 255, 255), // white
		sf::Color(255,  99,  71), // tomato
		sf::Color(50, 205,  50), // lime green
		sf::Color(30, 144, 255), // dodger blue
		sf::Color(255, 215,   0), // gold
		sf::Color(186,  85, 211), // medium orchid
		sf::Color(255, 140,   0), // dark orange
		sf::Color(64, 224, 208)  // turquoise
	};
}

Tank::Tank(int identifier, TankType type, const TextureHolder& textures, const FontHolder& fonts)
	:Entity(Table[static_cast<int>(type)].m_hitpoints,
		Table[static_cast<int>(type)].m_max_stamina,
		Table[static_cast<int>(type)].m_drain_rate,
		Table[static_cast<int>(type)].m_recharge_rate,
		Table[static_cast<int>(type)].m_sprint_multiplier
	)
	, m_type(type)
	, m_identifier(identifier) //the id for tank
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
	, m_outline_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
	, m_category(ReceiverCategories::kPlayerTank)
	, m_is_firing(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_fire_rate(1)
	, m_current_ammo(Table[static_cast<int>(type)].m_ammo_amount)
	, m_collision_cooldown(sf::Time::Zero)
	, m_max_hitpoints(Table[static_cast<int>(type)].m_hitpoints)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
	, m_show_fire_animation(false)
	, m_fire_animation(textures.Get(TextureID::kTankFireAnim))
	, m_ammo_icon(textures.Get(TextureID::kBulletUI))
	, m_ammo_text(fonts.Get(FontID::kMain))
	, m_anim_timer(0.f)
	, m_missile_ammo(0)
	, m_next_shot_missile(false)
	, m_textures(textures)
	, m_outline_scale(1.1f)
	, m_local_label(fonts.Get(FontID::kMain))
	, m_is_local(false)
{

	m_explosion.SetFrameSize(sf::Vector2i(256,256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));

	m_fire_animation.SetFrameSize(sf::Vector2i(256, 256));
	m_fire_animation.SetNumFrames(4);
	m_fire_animation.SetDuration(sf::seconds(0.2f));
	m_fire_animation.setPosition(sf::Vector2f(0.f, -120.f));
	

	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);
	Utility::CentreOrigin(m_fire_animation);
	m_outline_sprite = m_sprite;
	Utility::CentreOrigin(m_outline_sprite);
	m_outline_sprite.setScale(sf::Vector2f(m_outline_scale, m_outline_scale));
	UpdatePlayerColor();

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateBullet(node, textures);
		};

	//set up tanks health bar 
	m_health_bar_background.setSize(sf::Vector2f(150.f, 15.f));
	m_health_bar_background.setFillColor(sf::Color(50, 50, 50, 200));

	m_health_bar_foreground.setSize(sf::Vector2f(150.f, 15.f));
	m_health_bar_foreground.setFillColor(sf::Color(178, 34, 34));

	sf::Vector2f barPos = { 0.f, 150.f }; // Adjust -50.f based on tank size
	m_health_bar_background.setOrigin(m_health_bar_background.getSize() / 2.f);
	m_health_bar_foreground.setOrigin(m_health_bar_foreground.getSize() / 2.f);

	m_health_bar_background.setPosition(barPos);
	m_health_bar_foreground.setPosition(barPos);

	//set up stamina bar 
	m_stamina_bar_background.setSize(sf::Vector2f(150.f, 8.f));
	m_stamina_bar_background.setFillColor(sf::Color(50, 50, 50, 200));
	m_stamina_bar_background.setOrigin(m_stamina_bar_background.getSize() / 2.f);

	
	m_stamina_bar_foreground.setSize(sf::Vector2f(150.f, 8.f));
	m_stamina_bar_foreground.setFillColor(sf::Color::Cyan); 
	m_stamina_bar_foreground.setOrigin(m_stamina_bar_foreground.getSize() / 2.f);

	
	sf::Vector2f staminaPos = { 0.f, 170.f };
	m_stamina_bar_background.setPosition(staminaPos);
	m_stamina_bar_foreground.setPosition(staminaPos);

	//Ammo showup ui
	m_ammo_icon.setScale(sf::Vector2f(1.f, 1.f));
	m_ammo_icon.setPosition(sf::Vector2f(-60.f, 185.f));

	m_ammo_text.setCharacterSize(30);
	m_ammo_text.setFillColor(sf::Color::Black);
	m_ammo_text.setString("x " + std::to_string(m_current_ammo));
	m_ammo_text.setPosition({ -10.f, 185.f });

	m_local_label.setString("YOUR TANK");
	m_local_label.setCharacterSize(18);
	m_local_label.setFillColor(sf::Color::Black);
	m_local_label.setPosition({ 0.f, 235.f });
	Utility::CentreOrigin(m_local_label);
}

void Tank::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	//turret should get drawn by the scenegraph

	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}


	if (!IsDestroyed())
	{
		target.draw(m_health_bar_background, states);
		target.draw(m_health_bar_foreground, states);

		target.draw(m_stamina_bar_background, states);
		target.draw(m_stamina_bar_foreground, states);

		target.draw(m_ammo_icon, states);
		target.draw(m_ammo_text, states);

		if (m_is_local)
		{
			target.draw(m_local_label, states);
		}

		// outline behind the tank
		target.draw(m_outline_sprite, states);
		// main stprite
		target.draw(m_sprite, states);

		if (m_show_fire_animation)
		{
			
			target.draw(m_fire_animation, states);
		}
	}
	

}

void Tank::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	//Logic for the tank rotation 
	sf::Vector2f velocity = GetVelocity();

	Entity::UpdateStamina(dt);
	UpdateHealthBar();
	UpdateStaminaBar();

	//*****this was for local multiplayer now not needed because server handles it, if no commented out the tank would be jittering****
	//if (std::abs(velocity.x) > 1.0f || std::abs(velocity.y) > 1.0f) //check if we are moving 
	//{

	//	//atan2 function takes in y and x coordinates and gives angle of connecting line (0,0) to that point 
	//	// https://www.w3schools.com/cpp/ref_math_atan2.asp
	//	float radians = std::atan2(velocity.y, velocity.x); 
	//	float degrees = Utility::ToDegrees(radians);
	//	//move to that angle
	//	setRotation(sf::degrees(degrees + 90.f));
	//}

	UpdateMovementAnimation(dt);

	if (IsDestroyed())
	{
		m_explosion.Update(dt);
		if (!m_explosion_began)
		{
			sf::Vector2f position = GetWorldPosition();

			Command command;
			command.category = static_cast<int>(ReceiverCategories::kNetwork);
			command.action = DerivedAction<NetworkNode>([position](NetworkNode& node, sf::Time)
				{
					// This should tell multiplayer game state that tank exploded at that position
					node.NotifyGameAction(GameActions::kTankExplode, position);
				});
			commands.Push(command);
			m_explosion_began = true;
		}
		return;
	}

	if (m_show_fire_animation)
	{
		m_fire_animation.Update(dt);
		if (m_fire_animation.IsFinished())
		{
			m_show_fire_animation = false;
		}
	}

	if (!IsDestroyed())
	{
		UpdateUI();
	}

	Entity::UpdateCurrent(dt, commands);
	//Check if bullets or missiles were fired
	ReduceCollisionCooldown(dt);
	CheckProjectileLaunch(dt, commands);
}

unsigned int Tank::GetCategory() const
{
	return static_cast<unsigned int>(m_category);
}

void Tank::Fire()
{
	if (m_fire_countdown <= sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

void Tank::CreateBullet(SceneNode& node, const TextureHolder& textures) const
{
	//checks for the projectile type and owner 
	ProjectileType projType;
	if (m_next_shot_missile)
	{
		projType = ProjectileType::kMissile;
	}
	else
	{
		switch (m_type)
		{
			// I currently only have two types of projectiles, later can CHANGE HERE BULLETS SPRITES
		case TankType::kTank1: projType = ProjectileType::kTank1Bullet; break;
		case TankType::kTank2: projType = ProjectileType::kTank2Bullet; break;
		case TankType::kTank3: projType = ProjectileType::kTank3Bullet; break;
		case TankType::kTank4: projType = ProjectileType::kTank4Bullet; break;
		default: projType = ProjectileType::kTank1Bullet; break;
		}
	}

	ReceiverCategories owner = ReceiverCategories::kPlayerProjectile;
	

	std::unique_ptr<Projectile> bullet(new Projectile(projType, textures, owner, m_identifier));

	if (projType == ProjectileType::kMissile)
	{
		bullet->setScale(sf::Vector2f(1.2f, 1.2f));
	}
	else
	{
		bullet->setScale(sf::Vector2f(0.7f, 0.7f));
	}
	// decide the rotation for bullet
	sf::Angle rotation = getRotation();

	//conversions 
	float radiansRotation = rotation.asRadians();
	sf::Vector2f direction(std::sin(radiansRotation), -std::cos(radiansRotation));
	// positions rotation and speed 
	bullet->setPosition(GetWorldPosition() + direction * 100.f); 
	bullet->setRotation(rotation);                            
	bullet->SetVelocity(direction * bullet->GetMaxSpeed());

	node.AttachChild(std::move(bullet));
}

void Tank::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
	}

	SoundEffect soundEffect;

	if (m_is_firing)
	{
		//its time to fire
		if (m_fire_countdown <= sf::Time::Zero)
		{
			bool canFireMissile = m_missile_ammo > 0;
			bool canFireBullet = m_current_ammo > 0;

			if (canFireBullet || canFireMissile)
			{
				// give priority to missile if we have ammo 
				bool fireMissile = canFireMissile;

				if (fireMissile)
				{
					soundEffect = SoundEffect::kMissile;
					m_missile_ammo--;
					m_next_shot_missile = true;
				}
				else
				{
					soundEffect = SoundEffect::kTankBulletFire;
					m_current_ammo--;
					m_next_shot_missile = false;
				}
				commands.Push(m_fire_command); 
				PlayLocalSound(commands, soundEffect);

				m_show_fire_animation = true;
				m_fire_animation.Restart();
				m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
			}
		}
		m_is_firing = false;
	}
}

sf::FloatRect Tank::GetBoundingRect() const
{
	 //transform local sprite bounds into world coordinates
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

// **FUNCTIONS FOR COLLISION COOLDOWN**
bool Tank::CanBeDamaged() const
{
	return m_collision_cooldown <= sf::Time::Zero;
}

void Tank::ResetCollisionCooldown()
{
	m_collision_cooldown = sf::seconds(1.0f);
}

void Tank::ReduceCollisionCooldown(sf::Time dt)
{
	// check if the cooldown is more than zero so active, we can reduce it being called in update
	if (m_collision_cooldown > sf::Time::Zero)
	{
		m_collision_cooldown -= dt;
		if (m_collision_cooldown < sf::Time::Zero)
			m_collision_cooldown = sf::Time::Zero;
	}
}

void Tank::UpdateHealthBar()
{
	//get the percebtage 
	float healthRatio = static_cast<float>(GetHitPoints()) / m_max_hitpoints;

	//apply the green health bar 
	m_health_bar_foreground.setSize(sf::Vector2f(150.f * healthRatio, 15.f));
}

float Tank::GetSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Tank::UpdateStaminaBar()
{
	float ratio = GetStaminaRatio();

	m_stamina_bar_foreground.setSize(sf::Vector2f(150.f * ratio, 8.f));
}

void Tank::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
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

bool Tank::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Tank::Repair(int points)
{
	//how much to heal
	int currentHealth = GetHitPoints();
	int missingHealth = m_max_hitpoints - currentHealth;

	int amountToHeal = std::min(points, missingHealth);
	if (amountToHeal > 0)
	{
		Entity::Repair(amountToHeal);
	}
}

int Tank::GetAmmoCount() const
{
	return m_current_ammo;
}

void Tank::Reload(int amount)
{
	int maxAmmo = Table[static_cast<int>(m_type)].m_ammo_amount;
	m_current_ammo = std::min(m_current_ammo + amount, maxAmmo);
}

void Tank::UpdateMovementAnimation(sf::Time dt)
{
	//get how fast the tank is going 
	float speed = std::sqrt(GetVelocity().x * GetVelocity().x + GetVelocity().y * GetVelocity().y);
	bool smoking = false;
	//movement check 
	if (speed > 10.f)
	{
		//high speed will have full intensity and lower speed like stopping will be tiny vibration
		float intensity = std::min(1.0f, speed / 200.f);
		//"vibration" faster or slower depending on multiplying number
		m_anim_timer += dt.asSeconds() * 25.f;

		// oscilation movement 
		float scale = 1.0f + std::sin(m_anim_timer) * 0.03f * intensity;

		m_sprite.setScale(sf::Vector2f(1.f, 1.f));
	}
	else
	{
		//resetting when stopped
		m_anim_timer = 0.f;
		m_sprite.setScale(sf::Vector2f(1.f, 1.f));
		m_outline_sprite.setScale(m_sprite.getScale() * m_outline_scale);
	}
}

void Tank::CollectMissile(unsigned int amount)
{
	m_missile_ammo += amount;
}

int Tank::GetMissileAmmo() const
{
	return m_missile_ammo;
}

void Tank::UpdateUI()
{
	if (m_missile_ammo > 0)
	{
		m_ammo_icon.setTexture(m_textures.Get(TextureID::kMissileRefill));
		m_ammo_text.setString("x " + std::to_string(m_missile_ammo));
		m_ammo_text.setFillColor(sf::Color::Red);	

	}
	else
	{
		m_ammo_icon.setTexture(m_textures.Get(TextureID::kBulletUI));
		m_ammo_text.setString("x " + std::to_string(m_current_ammo));
		m_ammo_text.setFillColor(sf::Color::Black);
	}
}

int Tank::GetIdentifier() const
{
	return m_identifier;
}

void Tank::SetIdentifier(int identifier)
{
	m_identifier = identifier;
	UpdatePlayerColor();
}

void Tank::SetHitpoints(int points)
{
	m_hitpoints = std::clamp(points, 0, m_max_hitpoints);
}

void Tank::SetAmmo(int amount)
{
	// Clamp ammo between 0 and max, same logic as Reload
	int maxAmmo = Table[static_cast<int>(m_type)].m_ammo_amount;
	m_current_ammo = std::clamp(amount, 0, maxAmmo);
}

void Tank::SetMissileAmmo(uint8_t ammo) 
{
	m_missile_ammo = ammo; 
}

void Tank::SetStamina(float stamina)
{
	m_stamina = std::clamp(stamina, 0.f, GetMaxStamina());
}

void Tank::UpdatePlayerColor()
{
	const std::size_t index = static_cast<std::size_t>(std::abs(m_identifier)) % kPlayerColors.size();
	m_outline_sprite.setColor(kPlayerColors[index]);
}

void Tank::SetLocalPlayer(bool isLocal)
{
	m_is_local = isLocal;
}