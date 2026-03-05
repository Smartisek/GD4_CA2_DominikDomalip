#include "title_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"

TitleState::TitleState(StateStack& stack, Context context) : State(stack, context), m_show_text(true), m_text_effect_time(sf::Time::Zero), m_background_sprite(context.textures->Get(TextureID::kTitleScreen)), m_text(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f windowSize = context.window->getView().getSize();

    // getting size of texture
    sf::Vector2u textureSize = context.textures->Get(TextureID::kTitleScreen).getSize();

    //scale with the windows size 
    m_background_sprite.setScale({
        windowSize.x / textureSize.x,
        windowSize.y / textureSize.y
        });

    m_text.setString("Press any key to continue");
    Utility::CentreOrigin(m_text);
    m_text.setPosition({windowSize.x/2.f, windowSize.y/2.f + 500.f});
}

void TitleState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);

    if (m_show_text)
    {
        window.draw(m_text);
    }
}

bool TitleState::Update(sf::Time dt)
{
    m_text_effect_time += dt;
    if (m_text_effect_time >= sf::seconds(0.5))
    {
        m_show_text = !m_show_text;
        m_text_effect_time = sf::Time::Zero;
    }
    return true;
}

bool TitleState::HandleEvent(const sf::Event& event)
{
    const auto* key_released = event.getIf<sf::Event::KeyReleased>();
    if (key_released)
    {
        RequestStackPop();
        RequestStackPush(StateID::kMenu);

    }
    return true;
}