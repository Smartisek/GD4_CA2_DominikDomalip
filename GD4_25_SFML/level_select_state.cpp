#include "level_select_state.hpp"

#include "utility.hpp"
#include "data_tables.hpp"
#include "button.hpp"

LevelSelectState::LevelSelectState(StateStack& stack, Context context)
	: State(stack, context)
	, m_gui_container()
	, m_background_sprite(context.textures->Get(TextureID::kTitleScreen))

{
	sf::Vector2f viewSize = context.window->getView().getSize();
	sf::Vector2u textSize = m_background_sprite.getTexture().getSize();
	m_background_sprite.setScale({ viewSize.x / textSize.x, viewSize.y / textSize.y });
	//darken the background a little
	m_overlay.setSize(viewSize);
	m_overlay.setFillColor(sf::Color(0, 0, 0, 250));

	const std::vector<MapData> mapData = InitializeMapData();
	float startY = 150.f;


    for (int i = 0; i < static_cast<int>(MapType::kTypeCount); ++i)
    {
        MapType type = static_cast<MapType>(i);

        // make abutton
        auto button = std::make_shared<gui::Button>(context);

        //use the custom fuction and add the i texture to that button 
        button->SetCustomIcon(context.textures->Get(TextureID::kLandscape), mapData[i].m_theme_icon);

        //position and scales 
        button->setPosition(sf::Vector2f(viewSize.x / 2.f - 333.f, startY));
        button->setScale({ 6.f, 6.f });

        // saving selection and starting game state
        button->SetCallback([this, type, context]()
            {
                *context.currentMap = type; 
                RequestStackPop();
                RequestStackPush(StateID::kGame);
            });

        m_gui_container.Pack(button);
        //button y spacing 
        startY += 150.f; 
    }

    //back button into the settinhs 
    auto backButton = std::make_shared<gui::Button>(context);
    backButton->setPosition(sf::Vector2f(viewSize.x / 2.f - 100.f, startY ));
    backButton->SetText("Back");
    backButton->SetCallback([this]() {
        RequestStackPop();
        RequestStackPush(StateID::kMenu);
        });
    m_gui_container.Pack(backButton);
}

void LevelSelectState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    window.draw(m_background_sprite);
    window.draw(m_overlay);
    window.draw(m_gui_container);
}

bool LevelSelectState::Update(sf::Time)
{
    return true;
}

bool LevelSelectState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);

    return false;
}