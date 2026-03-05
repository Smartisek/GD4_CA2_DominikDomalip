#pragma once
#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

class TankSelectState : public State
{
public:
    TankSelectState(StateStack& stack, Context context);

    virtual void Draw() override;
    virtual bool Update(sf::Time dt) override;
    virtual bool HandleEvent(const sf::Event& event) override;

private:
    void UpdateLabels();

private:
    sf::Sprite m_background_sprite;
    sf::RectangleShape m_dark_overlay;
    gui::Container m_gui_container;
    sf::Text m_title_text;

    bool m_is_player_1_selecting;
    std::vector<sf::Text> m_stat_texts;
};

