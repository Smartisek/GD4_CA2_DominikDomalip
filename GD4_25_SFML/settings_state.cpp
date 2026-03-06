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
    for (std::size_t x = 0; x < 2; ++x)
    {
        // Use the Actions defined in your Action.hpp
        AddButtonLabel(static_cast<int>(Action::kMoveUp), x, 0, "Move Up", context);
        AddButtonLabel(static_cast<int>(Action::kMoveDown), x, 1, "Move Down", context);
        AddButtonLabel(static_cast<int>(Action::kMoveLeft), x, 2, "Move Left", context);
        AddButtonLabel(static_cast<int>(Action::kMoveRight), x, 3, "Move Right", context);
        AddButtonLabel(static_cast<int>(Action::kBulletFire), x, 4, "Fire", context);
        AddButtonLabel(static_cast<int>(Action::kSprint), x, 5, "Sprint", context);
    }
    UpdateLabels();

    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y - 100.f));
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
	const int actionCount = static_cast<int>(Action::kActionCount);

    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
    for (std::size_t i = 0; i < 2 * actionCount; ++i)
    {
        if (m_binding_buttons[i]->IsActive())
        {
            is_key_binding = true;
            const auto* key_event = event.getIf<sf::Event::KeyReleased>();

            if (key_event)
            {
                //first half its player 1 
                sf::Keyboard::Scancode pressed_key = key_event->scancode;
                if (i < actionCount)
                {
                    GetContext().keys1->AssignKey(static_cast<Action>(i), pressed_key);
                }
                else
                {
                    GetContext().keys2->AssignKey(static_cast<Action>(i - actionCount), pressed_key);
                }
                m_binding_buttons[i]->Deactivate();
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
    const int actionCount = static_cast<int>(Action::kActionCount);

    for (std::size_t i = 0; i < actionCount; ++i)
    {
        auto action = static_cast<Action>(i);
        sf::Keyboard::Scancode key1 = GetContext().keys1->GetAssignedKey(action);
        sf::Keyboard::Scancode key2 = GetContext().keys2->GetAssignedKey(action);

        m_binding_labels[i]->SetText(Utility::toString(key1));
        m_binding_labels[i + actionCount]->SetText(Utility::toString(key2));
    }
}

void SettingsState::AddButtonLabel(std::size_t index, std::size_t x, std::size_t y, const std::string& text, Context context)
{
    const int actionCount = static_cast<int>(Action::kActionCount);
    std::size_t index = index + (actionCount * x);
    float posX = 400.f * x + 80.f;
    float posY = 60.f * y + 250.f;

    m_binding_buttons[index] = std::make_shared<gui::Button>(context);
    m_binding_buttons[index]->setPosition(sf::Vector2f(posX, posY));
    m_binding_buttons[index]->SetText(text + (x == 0 ? " (P1)" : " (P2)"));
    m_binding_buttons[index]->SetToggle(true);

    m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts);
    m_binding_labels[index]->setPosition(sf::Vector2f(posX + 220.f, posY + 15.f));

    m_gui_container.Pack(m_binding_buttons[index]);
    m_gui_container.Pack(m_binding_labels[index]);
}