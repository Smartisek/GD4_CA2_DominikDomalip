#pragma once
#include "command_queue.hpp"
#include <SFML/Window/Event.hpp>
#include "action.hpp"
#include <map>
#include "command.hpp"
#include "key_binding.hpp"
#include "mission_status.hpp"
#include <SFML/Network/TcpSocket.hpp>


class Player
{
public:
	Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);

	//network specific 
	void HandleRealtimeNetworkInput(CommandQueue& commands);
	void HandleNetworkEvent(Action action, CommandQueue& commands);
	void HandleNetworkRealtimeChange(Action action, bool actionEnabled);
	void DisableAllRealtimeActions(bool enable);
	bool IsLocal() const;

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

private:
	void InitialiseActions();

private:
	const KeyBinding* m_key_binding;
	std::map<Action, Command> m_action_binding;
	std::map<Action, bool> m_action_proxies;

	uint8_t m_identifier;
	sf::TcpSocket* m_socket;

	MissionStatus m_current_mission_status;
	ReceiverCategories m_targetCategory;

};

