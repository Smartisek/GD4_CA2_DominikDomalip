#include "menu_state.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "menu_options.hpp"
#include "button.hpp"

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    sf::Vector2f windowSize = context.window->getView().getSize();

    // getting size of texture
    sf::Vector2u textureSize = context.textures->Get(TextureID::kTitleScreen).getSize();

    //scale with the windows size 
    m_background_sprite.setScale({
        windowSize.x / textureSize.x,
        windowSize.y / textureSize.y
        });

    // HOST GAME
    auto host_button = std::make_shared<gui::Button>(context);
    host_button->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y/2 + 320));
    host_button->SetText("Host");
    host_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kHostGame);
        });

    // JOIN GAME
    auto join_button = std::make_shared<gui::Button>(context);
    join_button->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y / 2 + 370));
    join_button->SetText("Join");
    join_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kJoinGame);
        });

    // ABOUT
    auto aboutButton = std::make_shared<gui::Button>(context);
    aboutButton->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y / 2 + 420));
    aboutButton->SetText("About");
    aboutButton->SetCallback([this]() {
        RequestStackPush(StateID::kAbout);
        });

    // SETTINGS 
    auto settings_button = std::make_shared<gui::Button>(context);
    settings_button->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y / 2 + 470));
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
        {
            RequestStackPush(StateID::kSettings);
        });

    // EXIT
    auto exit_button = std::make_shared<gui::Button>(context);
    exit_button->setPosition(sf::Vector2f(windowSize.x / 2.f - 100.f, windowSize.y / 2 + 520));
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
        {
            RequestStackPop();
        });

    m_gui_container.Pack(host_button);
	m_gui_container.Pack(join_button);
    m_gui_container.Pack(aboutButton);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

    context.music->Play(MusicThemes::kMenuTheme);
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

bool MenuState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}