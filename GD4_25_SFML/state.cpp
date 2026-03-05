#include "state.hpp"
#include "statestack.hpp"

State::State(StateStack& stack, Context context) : m_stack(&stack), m_context(context)
{
}

State::~State()
{
}

State::Context::Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, Player& player, Player& player2, MusicPlayer& music, SoundPlayer& sound, MapType& currentMap, TankType& p1Tank, TankType& p2Tank, int& winnerIndex)
	: window(&window)
	, textures(&textures)
	, fonts(&fonts)
	, player(&player)
	, player2(&player2)
	, music(&music)
	, sound(&sound)
	, currentMap(&currentMap)
	, p1Tank(&p1Tank)
	, p2Tank(&p2Tank)
	, winnerIndex(&winnerIndex)
{
}

void State::RequestStackPush(StateID state_id)
{
	m_stack->PushState(state_id);
}

void State::RequestStackPop()
{
	m_stack->PopState();
}

void State::RequestStackClear()
{
	m_stack->ClearStack();
}

State::Context State::GetContext() const
{
	return m_context;
}
