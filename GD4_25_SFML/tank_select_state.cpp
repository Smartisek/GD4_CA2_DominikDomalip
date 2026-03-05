#include "tank_select_state.hpp"
#include "data_tables.hpp"
#include "button.hpp"
#include "utility.hpp"

TankSelectState::TankSelectState(StateStack& stack, Context context)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
	, m_gui_container()
	, m_is_player_1_selecting(true)
	, m_title_text(context.fonts->Get(FontID::kMain))
{
	//background setup
    sf::Vector2f viewSize = context.window->getView().getSize();
    sf::Vector2u textSize = m_background_sprite.getTexture().getSize();
    m_background_sprite.setScale(sf::Vector2f(viewSize.x / textSize.x, viewSize.y / textSize.y));

    m_dark_overlay.setSize(viewSize);
    m_dark_overlay.setFillColor(sf::Color(0, 0, 0, 250));

    //title
    m_title_text.setFont(context.fonts->Get(FontID::kMain));
    m_title_text.setString("Player 1: Select Tank");
    m_title_text.setCharacterSize(50);
    Utility::CentreOrigin(m_title_text);
    m_title_text.setPosition(sf::Vector2f(viewSize.x / 2.f + 100.f, viewSize.y / 2.f));
    
    //for getting the data 
    const std::vector<TankData> tankData = InitializeTankData();

    // Define layout variables
    float startX = viewSize.x * 0.25f; 
    float startY = 150.f;   
    float gapY = 280.f;                

    for (int i = 0; i < static_cast<int>(TankType::kTankCount); ++i)
    {
        TankType type = static_cast<TankType>(i);
        auto button = std::make_shared<gui::Button>(context);

        // using the custom button I made for background selection
        button->SetCustomIcon(context.textures->Get(tankData[i].m_texture), tankData[i].m_texture_rect);

        // button->setScale(0.8f, 0.8f); 
        float currentY = startY + (i * gapY);
        button->setPosition(sf::Vector2f(startX, currentY));

        button->SetCallback([this, type, context]()
            {
                if (m_is_player_1_selecting)
                {
                    *context.p1Tank = type;
                    m_is_player_1_selecting = false;
                    UpdateLabels();
                }
                else
                {
                    *context.p2Tank = type;
                    RequestStackPop();
                    RequestStackPush(StateID::kLevelSelect);
                }
            });

        m_gui_container.Pack(button);
        //stats logic text
        sf::Text statText(context.fonts->Get(FontID::kMain));
        statText.setCharacterSize(20);
        statText.setFillColor(sf::Color::White);

        // format fot printign the stats
        std::stringstream ss;
        ss << "HP:      " << tankData[i].m_hitpoints << "\n"
            << "Speed:   " << static_cast<int>(tankData[i].m_speed) << "\n"
            << "Ammo:    " << tankData[i].m_ammo_amount << "\n"
            << "Stamina: " << static_cast<int>(tankData[i].m_max_stamina);

        statText.setString(ss.str());

        statText.setPosition(sf::Vector2f(startX + 200.f, currentY + 20.f ));

        //populate the vector texts
        m_stat_texts.push_back(statText);
    }
}

void TankSelectState::UpdateLabels()
{
    if (m_is_player_1_selecting)
        m_title_text.setString("Player 1: Select Tank");
    else
        m_title_text.setString("Player 2: Select Tank");

    Utility::CentreOrigin(m_title_text);
}

void TankSelectState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_dark_overlay);
    window.draw(m_title_text);
    window.draw(m_gui_container);
    for (const auto& text : m_stat_texts)
    {
        window.draw(text);
    }
}

bool TankSelectState::Update(sf::Time)
{
    return true;
}

bool TankSelectState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return false;
}