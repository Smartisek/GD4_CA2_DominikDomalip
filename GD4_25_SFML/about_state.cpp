#include "about_state.hpp"
#include "utility.hpp"
#include "resource_holder.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include "button.hpp"

AboutState::AboutState(StateStack& stack, Context context)
    : State(stack, context)
    , m_text(context.fonts->Get(FontID::kMain))
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    sf::Vector2f viewSize = context.window->getView().getSize();
    sf::Vector2u textSize = m_background_sprite.getTexture().getSize();
    m_background_sprite.setScale({ viewSize.x / textSize.x, viewSize.y / textSize.y });
    //darken the background a little
    m_overlay.setSize(viewSize);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 220));
    m_text.setString(
        "ARMORED ASSAULT\n"
        "Tactical Local Multiplayer Mayhem\n\n"

        "--- THE BATTLE ---\n"
        "Engage in high-stakes tank combat across 7 unique environments,\n"
        "from scorching deserts to treacherous underground bunkers.\n"
        "Choose from 4 specialized tank types, each with unique stats.\n\n"

        "--- CONTROLS ---\n"
        "Player 1: WASD (Move) | L-SHIFT (Sprint) | SPACE (Fire)\n"
        "Player 2: ARROWS (Move) | R-SHIFT (Sprint) | ENTER (Fire)\n\n"

        "--- SURVIVAL TIPS ---\n"
        "- Automated turrets guard the center; dodge their fire to survive!\n"
        "- Watch your STAMINA bar; sprinting drains power quickly.\n"
        "- Scavenge battlefields for Health and Ammo refills to stay in the fight.\n\n"

        "--- OBJECTIVE ---\n"
        "Outmaneuver and outgun your opponent. The last tank standing wins!"
    );
    m_text.setCharacterSize(20);
    m_text.setLineSpacing(1.2f);
    Utility::CentreOrigin(m_text);
    m_text.setPosition(context.window->getView().getSize() / 2.f);

    auto backButton = std::make_shared<gui::Button>(context);
    backButton->setPosition(sf::Vector2f(viewSize.x / 2.f - 100.f, viewSize.y / 2.f + 280.f));
    backButton->SetText("Back");
    backButton->SetCallback([this]() {
        RequestStackPop();
        });
    m_gui_container.Pack(backButton);
}

void AboutState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_overlay);
    window.draw(m_text);
    window.draw(m_gui_container);
}

bool AboutState::Update(sf::Time dt)
{
    return true;
}

bool AboutState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return false;
}