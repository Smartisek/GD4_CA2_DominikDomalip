#include "projectile.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "emitter_node.hpp"

namespace
{
    const std::vector<ProjectileData> Table = InitializeProjectileData();
}

Projectile::Projectile(ProjectileType type, const TextureHolder& textures, ReceiverCategories owner, uint8_t owner_id)
    : Entity(1,1,1,1,1), m_type(type), m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
    , m_owner(owner)
    , m_owner_id(owner_id)
{
    Utility::CentreOrigin(m_sprite);
    if (m_owner == ReceiverCategories::kEnemyProjectile)
    {
        std::unique_ptr<EmitterNode> trace(new EmitterNode(ParticleType::kTurretTrace));
        trace->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
        AttachChild(std::move(trace));

        std::unique_ptr<EmitterNode> smoke(new EmitterNode(ParticleType::kSmoke));
        smoke->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
        AttachChild(std::move(smoke));
    }
    else
    {
        std::unique_ptr<EmitterNode> smoke(new EmitterNode(ParticleType::kSmoke));
        smoke->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
        AttachChild(std::move(smoke));

        std::unique_ptr<EmitterNode> propellant(new EmitterNode(ParticleType::kPropellant));
        propellant->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
        AttachChild(std::move(propellant));
    }
}

void Projectile::GuideTowards(sf::Vector2f position)
{
    assert(IsGuided());
    m_target_direction = Utility::Normalise(position - GetWorldPosition());
}

bool Projectile::IsGuided() const
{
    return m_type == ProjectileType::kMissile;
}

unsigned int Projectile::GetCategory() const
{
    return static_cast<unsigned int>(m_owner);
}

sf::FloatRect Projectile::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

float Projectile::GetMaxSpeed() const
{
    return Table[static_cast<int>(m_type)].m_speed;
}

float Projectile::GetDamage() const
{
    return Table[static_cast<int>(m_type)].m_damage;
}

void Projectile::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    if (IsGuided())
    {
        //this will incrase the turn speed of the missile
        const float turn_speed = 5.f;
		sf::Vector2f current_dir = Utility::Normalise(GetVelocity());
        //get direction to target and add to current rotation normalise and multiply by the speed 
		sf::Vector2f new_dir = Utility::Normalise(current_dir + m_target_direction * turn_speed * dt.asSeconds());
		SetVelocity(new_dir * GetMaxSpeed());

        float angle = std::atan2(new_dir.y, new_dir.x);
		setRotation(sf::degrees(Utility::ToDegrees(angle) + 90.f));
    }
    Entity::UpdateCurrent(dt, commands);
}

void Projectile::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

uint8_t Projectile::GetOwnerId() const
{
    return m_owner_id;
}