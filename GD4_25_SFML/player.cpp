#include "player.hpp"
#include "tank.hpp"

struct TankMover
{
    TankMover(float vx, float vy) : velocityMultiplier(vx, vy) {}
    void operator()(Tank& tank, sf::Time dt) const
    {
        //this should make the tank move regardless of fps
        float speedScale = 50.0f;
        tank.Accelerate(velocityMultiplier * tank.GetSpeed() * speedScale * dt.asSeconds());
    }

    sf::Vector2f velocityMultiplier;
};

Player::Player(ReceiverCategories targetCategory)
    : m_targetCategory(targetCategory)
{
    m_key_binding[sf::Keyboard::Scancode::A] = Action::kMoveLeft;
    m_key_binding[sf::Keyboard::Scancode::D] = Action::kMoveRight;
    m_key_binding[sf::Keyboard::Scancode::W] = Action::kMoveUp;
    m_key_binding[sf::Keyboard::Scancode::S] = Action::kMoveDown;
    m_key_binding[sf::Keyboard::Scancode::Space] = Action::kBulletFire;
    m_key_binding[sf::Keyboard::Scancode::LShift] = Action::kSprint;

    if (m_targetCategory == ReceiverCategories::kPlayer2Tank)
    {
        m_key_binding.clear();
        m_key_binding[sf::Keyboard::Scancode::Left] = Action::kMoveLeft;
        m_key_binding[sf::Keyboard::Scancode::Right] = Action::kMoveRight;
        m_key_binding[sf::Keyboard::Scancode::Up] = Action::kMoveUp;
        m_key_binding[sf::Keyboard::Scancode::Down] = Action::kMoveDown;
        m_key_binding[sf::Keyboard::Scancode::Enter] = Action::kBulletFire;
		m_key_binding[sf::Keyboard::Scancode::RShift] = Action::kSprint;
    }

    InitialiseActions();

    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(m_targetCategory);
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        auto found = m_key_binding.find(key_pressed->scancode);
        if (found != m_key_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    for (auto pair : m_key_binding)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }
}

void Player::AssignKey(Action action, sf::Keyboard::Scancode key)
{
    //Remove keys that are currently bound to the action
    for (auto itr = m_key_binding.begin(); itr != m_key_binding.end();)
    {
        if (itr->second == action)
        {
            m_key_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
    m_key_binding[key] = action;
}

sf::Keyboard::Scancode Player::GetAssignedKey(Action action) const
{
    for (auto pair : m_key_binding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Scancode::Unknown;
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::InitialiseActions()
{
    const float kMoveAmount = 1.0f;
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Tank>(TankMover(-kMoveAmount, 0.f));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Tank>(TankMover(kMoveAmount, 0.f));
    m_action_binding[Action::kMoveUp].action = DerivedAction<Tank>(TankMover(0.f, -kMoveAmount));
    m_action_binding[Action::kMoveDown].action = DerivedAction<Tank>(TankMover(0.f, kMoveAmount));
    m_action_binding[Action::kBulletFire].action = DerivedAction<Tank>([](Tank& t, sf::Time)
        {
            t.Fire();
        });
    m_action_binding[Action::kSprint].action = DerivedAction<Tank>([](Tank& t, sf::Time) {
        t.Sprint();
        });
}

bool Player::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft:
    case Action::kMoveRight:
    case Action::kMoveUp:
    case Action::kMoveDown:
    case Action::kBulletFire:
	case Action::kSprint:
        return true;
    default:
        return false;
    }
}
