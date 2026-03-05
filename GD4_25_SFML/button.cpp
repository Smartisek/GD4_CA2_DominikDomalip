#include "button.hpp"
#include "fontID.hpp"
#include "utility.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "texture_id.hpp"

gui::Button::Button(State::Context context)
    : m_sprite(context.textures->Get(TextureID::kButtons))
    , m_text(context.fonts->Get(FontID::kMain), "", 16)
    , m_is_toggle(false)
    , m_is_custom(false)
    , m_sounds(*context.sound)
{
    ChangeTexture(ButtonType::kNormal);
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_text.setPosition(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
}

void gui::Button::SetCallback(Callback callback)
{
    m_callback = std::move(callback);
}

void gui::Button::SetToggle(bool flag)
{
    m_is_toggle = flag;
}

void gui::Button::SetText(const std::string& text)
{
    m_text.setString(text);
    Utility::CentreOrigin(m_text);
}

bool gui::Button::IsSelectable() const
{
    return true;
}

void gui::Button::Select()
{
    Component::Select();

    if (m_is_custom)
    {
        m_sprite.setColor(sf::Color::White);
    }
    else
    {
        ChangeTexture(ButtonType::kSelected);
    }
	m_sounds.Play(SoundEffect::kButtonSelect);
}

void gui::Button::Deselect()
{
    Component::Deselect();

    if (m_is_custom)
    {
        m_sprite.setColor(sf::Color(255, 255, 255, 150)); // dimm
    }
    else
    {
        ChangeTexture(ButtonType::kNormal);
    }
}

void gui::Button::Activate()
{ 
    Component::Activate();
    if (m_is_toggle && !m_is_custom)
    {
        ChangeTexture(ButtonType::kPressed);
    }
    if (m_callback)
    {
        m_callback();
    }
    if (!m_is_toggle)
    {
        Deactivate();
    }
    m_sounds.Play(SoundEffect::kButton);
}

void gui::Button::SetCustomIcon(const sf::Texture& texture, sf::IntRect textureRect)
{
    m_is_custom = true;
    m_sprite.setTexture(texture);
    m_sprite.setTextureRect(textureRect);

    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_text.setPosition(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
    //start deselected 
    m_sprite.setColor(sf::Color(255, 255, 255, 150));
}

void gui::Button::Deactivate()
{
    Component::Deactivate();
    if (m_is_toggle && !m_is_custom)
    {
        if (IsSelected())
        {
            ChangeTexture(ButtonType::kSelected);
        }
        else
        {
            ChangeTexture(ButtonType::kNormal);
        }
    }

    if (m_is_custom && IsSelected())
    {
        m_sprite.setColor(sf::Color::White);
    }
}

void gui::Button::HandleEvent(const sf::Event& event)
{
}

void gui::Button::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(m_sprite, states);
    target.draw(m_text, states);
}

void gui::Button::ChangeTexture(ButtonType buttonType)
{
    sf::IntRect textureRect({ 0, 50 * static_cast<int>(buttonType) }, { 200, 50 });
    m_sprite.setTextureRect(textureRect);
}