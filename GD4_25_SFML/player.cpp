#include "player.hpp"
#include "tank.hpp"
#include "network_protocol.hpp"
#include <SFML/Network/Packet.hpp>"

struct TankMover
{
    TankMover(float vx, float vy, int identifier) 
        : velocityMultiplier(vx, vy)
        , tank_id(identifier)
    {}
    
    void operator()(Tank& tank, sf::Time dt) const
    {
        if (tank.GetIdentifier() == tank_id)
        {
            float speedScale = 50.0f;
            tank.Accelerate(velocityMultiplier * tank.GetSpeed() * speedScale * dt.asSeconds());
        }
    }

    sf::Vector2f velocityMultiplier;
    int tank_id;
};

struct TankFireTrigger
{
    TankFireTrigger(int identifier) : tank_id(identifier) {}
    void operator() (Tank& tank, sf::Time) const
    {
        if (tank.GetIdentifier() == tank_id)
            tank.Fire();
    }
    int tank_id;
};

struct TankSprintTrigger
{
    TankSprintTrigger(int identifier) : tank_id(identifier) {}
    void operator() (Tank& tank, sf::Time) const
    {
        if (tank.GetIdentifier() == tank_id)
            tank.Sprint();
    }
	int tank_id;
};

Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding)
    : m_key_binding(binding)
    , m_identifier(identifier)
	, m_socket(socket)
{
    InitialiseActions();

    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerTank);
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    // one time events check such as firing
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(key_pressed->scancode, action) && !IsRealtimeAction(action))
        {
            if (m_socket)
            {
                // send the change from client to server
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerEvent);
                packet << m_identifier;
                packet << static_cast<uint8_t>(action);
                m_socket->send(packet);

            }
            else
            {
                // network disconnected so local event
                command_queue.Push(m_action_binding[action]);
            }
        }
    }

    // realtime keys like wasd press and release
    struct KeyStatus
    {
        sf::Keyboard::Scancode code;
        bool isPresed;
    };

    std::optional<KeyStatus> keyData;
    if (const auto* press = event.getIf<sf::Event::KeyPressed>())
    {
        keyData = { press->scancode, true };
    }
    else if (const auto* release = event.getIf<sf::Event::KeyReleased>())
    {
        keyData = { release->scancode, false };
    }

    if (keyData && m_socket)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(keyData->code, action) && IsRealtimeAction(action))
        {
            //send from client to server
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << keyData->isPresed;
            m_socket->send(packet);
        }
    }
}

bool Player::IsLocal() const
{
    // No key binding means this player is remote
    return m_key_binding != nullptr;
}

void Player::DisableAllRealtimeActions(bool enable)
{
    for (auto& action : m_action_proxies)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
        packet << m_identifier;
        packet << static_cast<uint8_t>(action.first);
        packet << enable;
        m_socket->send(packet);
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
   // local realtime input
    if ((m_socket && IsLocal()) || !m_socket)
    {
        if (!m_key_binding) return;

        std::vector<Action> activeActions;
        
        for (int i = 0; i < static_cast<int>(Action::kActionCount); ++i)
        {
            Action action = static_cast<Action>(i);

            // check if action is realtime for example moving is realtime 
            if (IsRealtimeAction(action))
            {
				//check the boundkeys from settings for example wasd and check if they are pressed
                sf::Keyboard::Scancode boundKey = m_key_binding->GetAssignedKey(action);

                // check if that key is being pressed 
                if (sf::Keyboard::isKeyPressed(boundKey))
                {
                    activeActions.push_back(action);
                }
            }
        }
        // psuh active actions to the command queue 
        for (Action action : activeActions)
        {
            command_queue.Push(m_action_binding[action]);
        }
    }
}

void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    // this is oponents tanks moving on players screen
    if (m_socket && !IsLocal())
    {
        for (auto pair : m_action_proxies)
        {
            if (pair.second && IsRealtimeAction(pair.first))
            {
                commands.Push(m_action_binding[pair.first]);
            }
        }
    }
}

void Player::HandleNetworkEvent(Action action, CommandQueue& commnands)
{
    commnands.Push(m_action_binding[action]);
}

void Player::HandleNetworkRealtimeChange(Action action, bool actionEnabled)
{
    m_action_proxies[action] = actionEnabled;
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
    // Passing in identifiers which is the id of tanks to know which to move
    const float kMoveAmount = 1.0f;
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Tank>(TankMover(-kMoveAmount, 0.f, m_identifier));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Tank>(TankMover(kMoveAmount, 0.f, m_identifier));
    m_action_binding[Action::kMoveUp].action = DerivedAction<Tank>(TankMover(0.f, -kMoveAmount, m_identifier));
    m_action_binding[Action::kMoveDown].action = DerivedAction<Tank>(TankMover(0.f, kMoveAmount, m_identifier));
    m_action_binding[Action::kBulletFire].action = DerivedAction<Tank>(TankFireTrigger(m_identifier));
    m_action_binding[Action::kSprint].action = DerivedAction<Tank>(TankSprintTrigger(m_identifier));
}
