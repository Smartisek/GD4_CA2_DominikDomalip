//#include "game_over_state.hpp"
//#include "utility.hpp"
//#include "player.hpp"
//#include "resource_holder.hpp"
//#include <SFML/Graphics/RectangleShape.hpp>
//#include <SFML/Graphics/RenderWindow.hpp>
//#include <SFML/Graphics/View.hpp>
//#include "button.hpp"
//
//GameOverState::GameOverState(StateStack& stack, Context context)
//	: State(stack, context), m_game_over_text(context.fonts->Get(FontID::kMain)), m_gui_container()
//{
//	sf::Vector2f windowSize(context.window->getSize());
//
//	if (*context.winnerIndex == 1)
//	{
//		m_game_over_text.setString("Player 1 Wins!");
//	}
//	else if (*context.winnerIndex == 2)
//	{
//		m_game_over_text.setString("Player 2 Wins!");
//	}
//	else
//	{
//		m_game_over_text.setString("Draw!");
//	}
//
//	m_game_over_text.setCharacterSize(70);
//	Utility::CentreOrigin(m_game_over_text);
//	m_game_over_text.setPosition(sf::Vector2f({ 0.5f * windowSize.x }, { 0.4f * windowSize.y }));
//
//	//back to menu button
//	auto returnButton = std::make_shared<gui::Button>(context);
//	returnButton->SetText("Back to Menu");
//	returnButton->setPosition(sf::Vector2f({ 0.5f * windowSize.x - 100.f }, { 0.6f * windowSize.y }));
//
//	returnButton->SetCallback([this]()
//		{
//			RequestStackClear();
//			RequestStackPush(StateID::kMenu);
//		});
//	m_gui_container.Pack(returnButton);
//}
//
//void GameOverState::Draw()
//{
//	sf::RenderWindow& window = *GetContext().window;
//	window.setView(window.getDefaultView());
//
//	sf::RectangleShape backgroundShape;
//	backgroundShape.setFillColor(sf::Color(0, 0, 0, 150));
//	backgroundShape.setSize(window.getView().getSize());
//
//	window.draw(backgroundShape);
//	window.draw(m_game_over_text);
//	window.draw(m_gui_container);
//
//}
//
//bool GameOverState::Update(sf::Time dt)
//{
//	return false;
//}
//
//bool GameOverState::HandleEvent(const sf::Event& event)
//{
//	m_gui_container.HandleEvent(event);
//	return false;
//}