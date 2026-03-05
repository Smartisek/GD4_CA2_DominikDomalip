#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "tank.hpp"
#include "projectile.hpp"
#include "sound_node.hpp"
#include "particle_node.hpp"
#include "particle_type.hpp"
#include "pickup.hpp"
#include "data_tables.hpp"
#include "obstacle.hpp"
#include "turret.hpp"
#include "popup_text.hpp"


World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, MapType mapType, TankType p1Type, TankType p2Type)
	: m_target(output_target) //m_window(window)
	, m_camera(output_target.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(4000.f, 4000.f))
	, m_spawn_position(m_world_bounds.size.x / 2.f, m_world_bounds.size.y / 2.f)
	, m_player_tank(nullptr)
	, m_player2_tank(nullptr)
	, m_sounds(sounds)
	, m_pickup_countdown(sf::seconds(10.f))
	, m_active_pickups(0)
	, m_current_map(mapType)
	, m_p1_type(p1Type)
	, m_p2_type(p2Type)
	, m_win_delay(sf::Time::Zero)
{
	m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y }); //might not need after implementing shaders??? *** CHECK LATER***
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);
}

void World::Update(sf::Time dt)
{

	ApplyFriction(dt);
	DestroyEntitiesOutsideView();
	GuideMissile();
	UpdateSounds();

	Command turretCommand;
	turretCommand.category = static_cast<int>(ReceiverCategories::kEnemy);
	turretCommand.action = DerivedAction<Turret>([this](Turret& turret, sf::Time)
		{
			// Check if tanks exist to avoid crashing
			if (m_player_tank && m_player2_tank)
			{
				turret.UpdateTarget(m_player_tank->getPosition(), m_player2_tank->getPosition());
			}
		});
	m_command_queue.Push(turretCommand);


	// 1. Process Input Commands
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}

	HandleCollisions();
	m_pickup_countdown -= dt;
	if (m_pickup_countdown <= sf::Time::Zero && m_active_pickups < 5)
	{
		SpawnRandomPickup();
		//randomly choose another coundown between 5 and 10 might change this later after testing 
		float randomSeconds = 5.0f + (std::rand() % 500) / 100.f;
		m_pickup_countdown = sf::seconds(randomSeconds);
	}

	m_scene_graph.RemoveWrecks();
	// 2. Update Scene Graph (Animations, Movement)
	m_scene_graph.Update(dt, m_command_queue);
	CheckOutOfBounds();
	UpdateView(dt);
}



void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scene_graph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_target);
	}
	else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scene_graph);
	}

	//m_window.setView(m_camera);
	//m_window.draw(m_scene_graph);
}


CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}


void World::LoadTextures()
{
	//m_textures.Load(TextureID::kLandscape, "Media/Textures/Background.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/Backgrounds.png");
	m_textures.Load(TextureID::kTankBody, "Media/Textures/Hull1.png");
	m_textures.Load(TextureID::kTankBody2, "Media/Textures/Hull2.png");
	m_textures.Load(TextureID::kTankBody3, "Media/Textures/Hull3.png");
	m_textures.Load(TextureID::kTankBody4, "Media/Textures/Hull4.png");
	m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kTankFireAnim, "Media/Textures/BulletFire.png");
	m_textures.Load(TextureID::kGrenade, "Media/Textures/Granade_Shell.png");
	m_textures.Load(TextureID::kHealthRefill, "Media/Textures/HealthRefill.png");
	m_textures.Load(TextureID::kBulletRefill, "Media/Textures/FireRate.png");
	m_textures.Load(TextureID::kBulletUI, "Media/Textures/FireSpread.png");
	m_textures.Load(TextureID::kWall, "Media/Textures/Brickwall.png");
	m_textures.Load(TextureID::kTurret, "Media/Textures/Turret.png");
	m_textures.Load(TextureID::kTurretPlasma, "Media/Textures/Plasma.png");
	m_textures.Load(TextureID::kMissile, "Media/Textures/Missile.png");
	m_textures.Load(TextureID::kMissileRefill, "Media/Textures/MissileRefill.png");
}

void World::BuildScene()
{
	// getting data from data table for the current map
	const std::vector<MapData> MapTable = InitializeMapData();
	const MapData& currentMapData = MapTable[static_cast<int>(m_current_map)];

	//Initialise the different layers
	for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kLowerGround)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scene_graph.AttachChild(std::move(layer));
	}

	sf::IntRect mapRect = MapTable[static_cast<int>(m_current_map)].m_texture_rect;
	sf::Texture& texture = m_textures.Get(TextureID::kLandscape);
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, mapRect));

	// stretch the sprite to fit the battlefield
	// We calculate how much we need to grow the map to fill the m_world_bounds still needs fix
	float scaleX = m_world_bounds.size.x / static_cast<float>(mapRect.size.x);
	float scaleY = m_world_bounds.size.y / static_cast<float>(mapRect.size.y);
	background_sprite->setScale({ scaleX, scaleY });
	background_sprite->setPosition({ 0.f, 0.f });
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Obstacles spawnign
	for (const sf::Vector2f& pos : currentMapData.m_obstacle_positions) {
		std::unique_ptr<Obstacle> obstacle(new Obstacle(m_textures, 150));
		obstacle->setScale(sf::Vector2f{ 0.1f, 0.2f });
		obstacle->setPosition(pos);
		m_scene_layers[static_cast<int>(SceneLayers::kUpperGround)]->AttachChild(std::move(obstacle));
	}

	//turret setting
	std::unique_ptr<Turret> t1(new Turret(TurretType::kStandard, m_textures));
	t1->setPosition(m_spawn_position);
	t1->setScale({ 0.6f, 0.6f });
	m_scene_layers[static_cast<int>(SceneLayers::kUpperGround)]->AttachChild(std::move(t1));

	// spawning for players on oposite sides 
	const float margin = 300.f;
	sf::Vector2f player1Spawn(m_world_bounds.position.x + margin, m_world_bounds.size.y / 2.f);
	sf::Vector2f player2Spawn(m_world_bounds.position.x + m_world_bounds.size.x - margin, m_world_bounds.size.y / 2.f);

	//adding tank player 1 
	std::unique_ptr<Tank> playerTank(new Tank(m_p1_type, m_textures, m_fonts, ReceiverCategories::kPlayer1Tank));
	m_player_tank = playerTank.get();
	m_player_tank->setScale(sf::Vector2f(0.5f, 0.5f));
	m_player_tank->setPosition(player1Spawn);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperGround)]->AttachChild(std::move(playerTank));

	//addding player tank 2 
	std::unique_ptr<Tank> player2Tank(new Tank(m_p2_type, m_textures, m_fonts, ReceiverCategories::kPlayer2Tank));
	m_player2_tank = player2Tank.get();
	m_player2_tank->setScale(sf::Vector2f(0.5f, 0.5f));
	m_player2_tank->setPosition(player2Spawn);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperGround)]->AttachChild(std::move(player2Tank));

	//particle nodes 
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerGround)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerGround)]->AttachChild(std::move(propellantNode));

	std::unique_ptr<ParticleNode> traceNode(new ParticleNode(ParticleType::kTurretTrace, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerGround)]->AttachChild(std::move(traceNode));

	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scene_graph.AttachChild(std::move(soundNode));
}

void World::DestroyEntitiesOutsideView()
{
	Command command;

	command.category = static_cast<int>(ReceiverCategories::kPlayer1Projectile) | static_cast<int>(ReceiverCategories::kPlayer2Projectile) |
		static_cast<int>(ReceiverCategories::kEnemyProjectile);
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
		{
			//Does the object intersect with the battlefield
			if (GetBattleFieldBounds().findIntersection(e.GetBoundingRect()) == std::nullopt)
			{
				e.Destroy();

			}
		});
	m_command_queue.Push(command);

}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area off screen where the enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;
	return bounds;
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());;
}

//Helper function to check if the scene nodes mathch the given categories
bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if ((static_cast<int>(type1) & category1) && (static_cast<int>(type2) & category2))
	{
		return true;
	}
	else if ((static_cast<int>(type1) & category2) && (static_cast<int>(type2) & category1))
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}

}

void World::HandleCollisions() {
	std::set<SceneNode::Pair> collisionPairs;
	m_scene_graph.CheckSceneCollision(m_scene_graph, collisionPairs);

	for (SceneNode::Pair pair : collisionPairs)
	{
		// 1. player 1 hit by player 2's projectile
		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Tank, ReceiverCategories::kPlayer2Projectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& bullet = static_cast<Projectile&>(*pair.second);

			tank.Damage(bullet.GetDamage());
			bullet.Destroy();

			// spawn popup
			float damage = bullet.GetDamage();
			CreatePopup(tank.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)damage));

			//after applying the damage check if it was fatal 
			if (tank.IsDestroyed())
			{
				tank.PlayLocalSound(m_command_queue, SoundEffect::kExplosionDestroy);
			}
			else
			{
				tank.PlayLocalSound(m_command_queue, SoundEffect::kExplosion1);
				std::cout << "Player 1 hit by Player 2's projectile! Tank HP: " << tank.GetHitPoints() << "\n";
			}
		}

		// 2. player 2 hit by player 1's projectile
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Tank, ReceiverCategories::kPlayer1Projectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& bullet = static_cast<Projectile&>(*pair.second);

			tank.Damage(bullet.GetDamage());
			bullet.Destroy();

			float damage = bullet.GetDamage();
			CreatePopup(tank.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)damage));

			//same as for tank 2 check if fatal bllow
			if (tank.IsDestroyed())
			{
				tank.PlayLocalSound(m_command_queue, SoundEffect::kExplosionDestroy);
			}
			else
			{
				tank.PlayLocalSound(m_command_queue, SoundEffect::kExplosion1);
				std::cout << "Player 2 hit by Player 1's projectile! Tank HP: " << tank.GetHitPoints() << "\n";
			}
		}

		// 3. tank vs tank (body collision)
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer1Tank, ReceiverCategories::kPlayer2Tank))
		{
			auto& p1 = static_cast<Tank&>(*pair.first);
			auto& p2 = static_cast<Tank&>(*pair.second);

			HandleTankCollision(p1, p2);
		}

		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Tank, ReceiverCategories::kPickup))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			pickup.Apply(tank);
			pickup.Destroy();

			CreatePopup(tank.GetWorldPosition(), pickup.GetPopupType(), pickup.GetPopupText());

			tank.PlayLocalSound(m_command_queue, SoundEffect::kPickup);
			m_active_pickups--;
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Tank, ReceiverCategories::kPickup))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			pickup.Apply(tank);
			pickup.Destroy();

			CreatePopup(tank.GetWorldPosition(), pickup.GetPopupType(), pickup.GetPopupText());

			tank.PlayLocalSound(m_command_queue, SoundEffect::kPickup);
			m_active_pickups--;
		}

		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Tank, ReceiverCategories::kObstacle) || MatchesCategories(pair, ReceiverCategories::kPlayer2Tank, ReceiverCategories::kObstacle))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& obstacle = static_cast<Obstacle&>(*pair.second);
			//getting simple collision
			sf::Vector2f tankPos = tank.getPosition();
			sf::Vector2f obsPos = obstacle.getPosition();
			sf::Vector2f diff = tankPos - obsPos;
			//similar push like when tanks ram each other
			float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
			if (len > 0)
			{
				diff /= len;
			}
			tank.move(diff * 5.f);
			tank.SetVelocity(sf::Vector2f(0.f, 0.f));
		}
		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Projectile, ReceiverCategories::kObstacle) || MatchesCategories(pair, ReceiverCategories::kPlayer2Projectile, ReceiverCategories::kObstacle))
		{
			auto& bullet = static_cast<Projectile&>(*pair.first);
			auto& obstacle = static_cast<Obstacle&>(*pair.second);

			obstacle.Damage(bullet.GetDamage()); // Obstacle takes damage
			bullet.Destroy();

			float damage = bullet.GetDamage();
			CreatePopup(obstacle.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)damage));
			 
			obstacle.PlayLocalSound(m_command_queue, SoundEffect::kWall);
		}

		//turret bullet and tanks
		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Tank, ReceiverCategories::kEnemyProjectile) ||
			MatchesCategories(pair, ReceiverCategories::kPlayer2Tank, ReceiverCategories::kEnemyProjectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& bullet = static_cast<Projectile&>(*pair.second);

			tank.Damage(bullet.GetDamage());
			bullet.Destroy();

			float damage = bullet.GetDamage();
			CreatePopup(tank.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)damage));

			tank.PlayLocalSound(m_command_queue, SoundEffect::kExplosion1);
		}
		
		if (MatchesCategories(pair, ReceiverCategories::kPlayer1Projectile, ReceiverCategories::kEnemy) ||
			MatchesCategories(pair, ReceiverCategories::kPlayer2Projectile, ReceiverCategories::kEnemy))
		{
			auto& bullet = static_cast<Projectile&>(*pair.first);
			auto& turret = static_cast<Turret&>(*pair.second);

			turret.Damage(bullet.GetDamage());
			bullet.Destroy();

			float damage = bullet.GetDamage();
			CreatePopup(turret.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)damage));

			turret.PlayLocalSound(m_command_queue, SoundEffect::kExplosion1);

			if (turret.IsDestroyed())
			{
				turret.PlayLocalSound(m_command_queue, SoundEffect::kExplosionDestroy);
			}
		}

		if (MatchesCategories(pair, ReceiverCategories::kEnemyProjectile, ReceiverCategories::kObstacle))
		{
			auto& bullet = static_cast<Projectile&>(*pair.first);
			auto& obstacle = static_cast<Obstacle&>(*pair.second);

			obstacle.Damage(bullet.GetDamage());
			bullet.Destroy();
			if (obstacle.IsDestroyed())
			{
				obstacle.PlayLocalSound(m_command_queue, SoundEffect::kExplosionDestroy);
			}
			else
			{
				obstacle.PlayLocalSound(m_command_queue, SoundEffect::kExplosion1);
			}
		}
	}

	
}

void World::HandleTankCollision(Tank& tank1, Tank& tank2)
{
	sf::Vector2f tank1Pos = tank1.GetWorldPosition();
	sf::Vector2f tank2Pos = tank2.GetWorldPosition();
	//this vector gives us arrow pointing from player2 to player1 
	sf::Vector2f diff = tank1Pos - tank2Pos;

	float distSq = diff.x * diff.x + diff.y * diff.y; //pythagoras distance squared 
	float dist = std::sqrt(distSq); //the acutal distance 

	if (dist > 0.1f) //preventing division by zero
	{
		sf::Vector2f normal = diff / dist; // normalize by making length 1 making it direciton

		//fixing bug with the collision into each other bugging with commands
		float pushForce = 10.0f;
		tank1.move(normal * pushForce); //move tank1 away along the normal so from tank2
		tank2.move(-normal * pushForce); //move tank2 oposite way 

		sf::Vector2f v1 = tank1.GetVelocity();
		sf::Vector2f v2 = tank2.GetVelocity();

		//-normal for tank1 because normal points away from tank2, we want speed towards tank2
		float tank1SpeedTowards = v1.x * -normal.x + v1.y * -normal.y;
		//normal because normal points towards tank 1 
		float tank2SpeedTowards = v2.x * normal.x + v2.y * normal.y;

		float damageThreshold = 10.0f;

		//logic for who is ramming who to be able to apply damage to the "rammed one"
		if (tank1SpeedTowards > tank2SpeedTowards + damageThreshold)
		{
			if (tank2.CanBeDamaged())
			{
				tank2.PlayLocalSound(m_command_queue, SoundEffect::kTankCollision);
				tank2.Damage(10);
				CreatePopup(tank2.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)10));
				tank2.ResetCollisionCooldown();
				tank2.Accelerate(-normal *( pushForce + 200.0f));

			}
		}
		else if (tank2SpeedTowards > tank1SpeedTowards + damageThreshold)
		{
			if (tank1.CanBeDamaged())
			{
				tank1.PlayLocalSound(m_command_queue, SoundEffect::kTankCollision);
				CreatePopup(tank1.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)10));
				tank1.Damage(10);

				tank1.ResetCollisionCooldown();
				tank1.Accelerate(normal * (pushForce + 200.0f));
			}
		}
		else if (tank1SpeedTowards > 0 && tank2SpeedTowards > 0)
		{
			// head-on collision
			if (tank1.CanBeDamaged()) { tank1.Damage(10); tank1.ResetCollisionCooldown(); tank1.move(normal * (pushForce + 50.0f));
			tank1.PlayLocalSound(m_command_queue, SoundEffect::kTankCollision);
			CreatePopup(tank1.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)10));
			}
			if (tank2.CanBeDamaged()) { tank2.Damage(10); tank2.ResetCollisionCooldown(); tank2.move(-normal * (pushForce + 50.0f));
			tank2.PlayLocalSound(m_command_queue, SoundEffect::kTankCollision);
			CreatePopup(tank2.GetWorldPosition(), PopupType::kDamage, "-" + std::to_string((int)10));
			}
		}

		//stopping from fighting the push
		float dot1 = v1.x * normal.x + v1.y * normal.y;
		float dot2 = v2.x * normal.x + v2.y * normal.y;

		if (dot1 < 0.f) tank1.SetVelocity(v1 - normal * dot1);
		if (dot2 > 0.f) tank2.SetVelocity(v2 - normal * dot2);
	}
}

void World::ApplyFriction(sf::Time dt) {
	// value for the damp between 0.0 - 1.0, the higher means slide more and lower stops faster 
	float damping = 0.6f;

	//https://www.w3schools.com/cpp/ref_math_exp.asp
	//this should make the friction feel same for no matter what fps, 60 or 144 no matter
	float frictionFactor = std::exp(-damping * dt.asSeconds() * 10.0f);

	//lambda function to make the slide feel
	// if speed is 100 and frictionFactor is 0.9, the next frame the speed will be 90, then 81 and so on
	// making the slow down feel and allowing me to use smoother input instead of snappy one with the 
	// reseting velocity inside the update when we stop holding key
	auto dampenVelocity = [frictionFactor](Tank* tank) {
		if (tank) {
			sf::Vector2f velocity = tank->GetVelocity();
			velocity *= frictionFactor;

			// Stop the tank completely if it's moving extremely slow
			if (std::abs(velocity.x) < 1.0f && std::abs(velocity.y) < 1.0f) {
				velocity = sf::Vector2f(0.f, 0.f);
			}
			tank->SetVelocity(velocity);
		}
		};

	dampenVelocity(m_player_tank);
	dampenVelocity(m_player2_tank);

}


void World::UpdateSounds()
{
	sf::Vector2f listener_position;

	listener_position = m_camera.getCenter();

	m_sounds.SetListenerPosition(listener_position);

	m_sounds.RemoveStoppedSounds();
}

void World::SpawnRandomPickup()
{
	sf::FloatRect bounds = m_world_bounds;
	float margin = 50.f; //so it does noit spawn on the edge
	//get random x and y location and store it in vector 
	int x = std::rand() % static_cast<int>(bounds.size.x - 2 * margin);
	int y = std::rand() % static_cast<int>(bounds.size.y - 2 * margin);
	sf::Vector2f spawnPos(bounds.position.x + margin + x, bounds.position.y + margin + y);

	//choose random pickup to spawn
	int typeIndex = std::rand() % static_cast<int>(PickupType::kTypeCount);
	PickupType type = static_cast<PickupType>(typeIndex);

	//create the pickup at the choosen location 
	std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
	pickup->setPosition(spawnPos);
	//attahc it to lower ground so we drive over it 
	m_scene_layers[static_cast<int>(SceneLayers::kLowerGround)]->AttachChild(std::move(pickup));
	m_active_pickups++;
}

void World::UpdateView(sf::Time dt)
{
	sf::Vector2f pos1 = m_player_tank->getPosition();
	sf::Vector2f pos2 = m_player2_tank->getPosition();
	//calculation for the center
	sf::Vector2f midpoint = (pos1 + pos2) / 2.f;
	//calculation for required size based on distance 
	float dx = std::abs(pos1.x - pos2.x);
	float dy = std::abs(pos1.y - pos2.y);

	//convert sizes to float immediately to avoid std::max error
	float winX = static_cast<float>(m_target.getSize().x);
	float winY = static_cast<float>(m_target.getSize().y);
	float windowAspect = winX / winY;

	float margin = 400.f;
	float requiredWidth = dx + margin;
	float requiredHeight = dy + margin;

	sf::Vector2f targetSize;

	// Use the aspect ratio to decide which dimension to scale by
	if (requiredWidth / requiredHeight > windowAspect) {
		targetSize.x = std::max(requiredWidth, winX);
		targetSize.y = targetSize.x / windowAspect;
	}
	else {
		targetSize.y = std::max(requiredHeight, winY);
		targetSize.x = targetSize.y * windowAspect;
	}
	//ensure the camera size never goes beyond the total of world dimensions
	targetSize.x = std::min(targetSize.x, m_world_bounds.size.x);
	targetSize.y = std::min(targetSize.y, m_world_bounds.size.y);
	//applying smoothing, move 5% of the way to the target every frame, kind of like nice animation
	float lerpFactor = 0.05f;
	m_camera.setCenter(m_camera.getCenter() + (midpoint - m_camera.getCenter()) * lerpFactor);
	m_camera.setSize(m_camera.getSize() + (targetSize - m_camera.getSize()) * lerpFactor);

	// bounds Clamping
	sf::Vector2f center = m_camera.getCenter();
	sf::Vector2f size = m_camera.getSize();


	if (size.x < m_world_bounds.size.x) {
		center.x = std::clamp(center.x, m_world_bounds.position.x + size.x / 2.f,
			m_world_bounds.position.x + m_world_bounds.size.x - size.x / 2.f);
	}
	else {
		center.x = m_world_bounds.position.x + m_world_bounds.size.x / 2.f;
	}

	if (size.y < m_world_bounds.size.y) {
		center.y = std::clamp(center.y, m_world_bounds.position.y + size.y / 2.f,
			m_world_bounds.position.y + m_world_bounds.size.y - size.y / 2.f);
	}
	else {
		center.y = m_world_bounds.position.y + m_world_bounds.size.y / 2.f;
	}
	m_camera.setCenter(center);
}

void World::CheckOutOfBounds()
{
	//to prevent overlapping at the edge
	const float margin = 40.f;

	auto clampTank = [this, margin](Tank* tank)
		{
			if (tank && !tank->IsDestroyed())
			{
				sf::Vector2f pos = tank->getPosition();

				// to keep the tank inside world boundaries
				pos.x = std::clamp(pos.x, m_world_bounds.position.x + margin, m_world_bounds.position.x + m_world_bounds.size.x - margin);
				pos.y = std::clamp(pos.y, m_world_bounds.position.y + margin, m_world_bounds.position.y + m_world_bounds.size.y - margin);

				tank->setPosition(pos);
			}
		};

	clampTank(m_player_tank);
	clampTank(m_player2_tank);
}

bool World::HasPlayer1Won() const
{
	return m_player2_tank->IsDestroyed();
}

bool World::HasPlayer2Won() const
{
	return m_player_tank->IsDestroyed();
}

void World::GuideMissile()
{
	Command guideCommand;
	// Target all player projectiles
	guideCommand.category = static_cast<int>(ReceiverCategories::kPlayer1Projectile) |
		static_cast<int>(ReceiverCategories::kPlayer2Projectile);
	//lambda function expression to guide the missile towards other tank if it has missile ammo 
	guideCommand.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time)
		{
			if (!missile.IsGuided()) return;
			// if the missile belongs to player 1, target player 2 and vice versa
			Tank* target = (missile.GetCategory() == static_cast<int>(ReceiverCategories::kPlayer1Projectile))
				? m_player2_tank : m_player_tank;

			if (target && !target->IsDestroyed())
			{
				//distance calculation of the missile to the target 
				sf::Vector2f diff = target->GetWorldPosition() - missile.GetWorldPosition();
				float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

				// guide only over this range when we are too close
				if (distance > 50.f)
				{
					missile.GuideTowards(target->GetWorldPosition());
				}
			}
		});

	m_command_queue.Push(guideCommand);
}

void World::CreatePopup(sf::Vector2f position, PopupType type, const std::string& text)
{
	std::unique_ptr<PopupText> popup(new PopupText(type, text, m_fonts));
	//spawn above the entity
	popup->setPosition(position + sf::Vector2f(0.f, -60.f));
	m_scene_layers[static_cast<int>(SceneLayers::kUpperGround)]->AttachChild(std::move(popup));
}