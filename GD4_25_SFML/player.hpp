#pragma once
#include "command_queue.hpp"
#include <SFML/Window/Event.hpp>
#include "action.hpp"
#include <map>
#include "command.hpp"
#include "mission_status.hpp"


class Player
{
public:
	Player(ReceiverCategories targetCategory = ReceiverCategories::kPlayer1Tank);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);

	void AssignKey(Action action, sf::Keyboard::Scancode key);
	sf::Keyboard::Scancode GetAssignedKey(Action action) const;
	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

private:
	void InitialiseActions();
	static bool IsRealTimeAction(Action action);

private:
	std::map<sf::Keyboard::Scancode, Action> m_key_binding;
	std::map<Action, Command> m_action_binding;
	MissionStatus m_current_mission_status;
	ReceiverCategories m_targetCategory;

};

