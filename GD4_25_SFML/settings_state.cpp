#include "settings_state.hpp"
#include "Utility.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    sf::Vector2f windowSize = context.window->getView().getSize();

    // getting size of texture
    sf::Vector2u textureSize = context.textures->Get(TextureID::kTitleScreen).getSize();

    //scale with the windows size 
    m_background_sprite.setScale({
        windowSize.x / textureSize.x,
        windowSize.y / textureSize.y
        });
    AddButtonLabel(Action::kMoveUp, windowSize.y/2, "Move Up", context);
    AddButtonLabel(Action::kMoveDown, windowSize.y / 2 + 50, "Move Down", context);
    AddButtonLabel(Action::kMoveRight, windowSize.y / 2 +100, "Move Right", context);
    AddButtonLabel(Action::kMoveLeft, windowSize.y / 2 +150, "Move Left", context);
    AddButtonLabel(Action::kBulletFire, windowSize.y / 2 + 200, "Fire", context);
    AddButtonLabel(Action::kSprint, windowSize.y / 2 + 250, "Sprint", context);

    UpdateLabels();

    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(sf::Vector2f(80.f, windowSize.y/2 + 300));
    back_button->SetText("Back");
    back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));
    m_gui_container.Pack(back_button);
}

void SettingsState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool SettingsState::Update(sf::Time dt)
{
    return true;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
    bool is_key_binding = false;

    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
    for (std::size_t action = 0; action < static_cast<int>(Action::kActionCount); ++action)
    {
        if (m_binding_buttons[action]->IsActive())
        {
            is_key_binding = true;
            const auto* key_released = event.getIf<sf::Event::KeyReleased>();
            if (key_released)
            {
                GetContext().player->AssignKey(static_cast<Action>(action), key_released->scancode);
                m_binding_buttons[action]->Deactivate();
            }
            break;
        }
    }

    if (is_key_binding)
    {
        UpdateLabels();
    }
    else
    {
        m_gui_container.HandleEvent(event);
    }
    return false;
}

void SettingsState::UpdateLabels()
{
    Player& player = *GetContext().player;
    for (std::size_t i = 0; i < static_cast<int>(Action::kActionCount); ++i)
    {
        sf::Keyboard::Scancode key = player.GetAssignedKey(static_cast<Action>(i));
        m_binding_labels[i]->SetText(Utility::toString(key));
    }
}

void SettingsState::AddButtonLabel(Action action, float y, const std::string& text, Context context)
{
    m_binding_buttons[static_cast<int>(action)] = std::make_shared<gui::Button>(context);
    m_binding_buttons[static_cast<int>(action)]->setPosition(sf::Vector2f(80.f, y));
    m_binding_buttons[static_cast<int>(action)]->SetText(text);
    m_binding_buttons[static_cast<int>(action)]->SetToggle(true);

    m_binding_labels[static_cast<int>(action)] = std::make_shared<gui::Label>("", *context.fonts);
    m_binding_labels[static_cast<int>(action)]->setPosition(sf::Vector2f(300.f, y + 15.f));

    m_gui_container.Pack(m_binding_buttons[static_cast<int>(action)]);
    m_gui_container.Pack(m_binding_labels[static_cast<int>(action)]);
}