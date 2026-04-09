#pragma once

#include <SFML/System/Vector2.hpp>

// Server will be listening on this port 
// Greater than 49151 which is in dynamic ports range, no conflicts with well-known ports
const unsigned short SERVER_PORT = 50000; 

namespace Server
{
	enum class PacketType
	{
		kBroadcastMessage, //string message sent to all clients, can be shown on their screen
		kLobbyUpdate, //Server will tell clients who is in the lobby, their IDs and names, and if the game is starting
		kStartGame, //this will tell client to start the game and take the map type and the initial positions of tanks, so clients can create the map and tanks before the game starts
		kInitialState, //this will tell client map type, number of tanks (players), their IDs and positions
		kPlayerEvent, //when player makes an action, this takes sf::Int32 variables, tank ID and action ID from the action.hpp
		kPlayerRealtimeChange, //same as above but for realtime actions, such as holding down the key to move, instead of just pressing it once
		kPlayerConnect, //when player connects to a server from client, this will send the player ID and initial position to the server
		kPlayerDisconnect, //takes sf::Int32 tank ID when player disconnects so we are able to remove him
		kSpawnPickup, //takes sf::Int32 for pickup type in PickupType.hpp and the position
		kSpawnProjectile, //takes sf::Int32 for projectile type in ProjectileType.hpp and the position
		kTurretState, // turret rotations
		kUpdateClientState, // server will send this to clients every frame, it will contain all the information about tanks, projectiles and pickups on the map 
		kGameEvent, // this will be used to send game events such as player killed, player won, etc
		kMissionSuccess, //to give the game over screen to clients, takes the player ID of the winner
		kEntityDamage //notify clients that entity took damage 
	};
}

namespace Client
{
	enum class PacketType
	{
		kSelectTank, //when player selects a tank, this will send the tank type to the server
		kSelectMap, //when player selects a map, this will send the map type to the server
		kToggleReady, //when player toggles ready, this will send the ready state to the server
		kPlayerEvent, // send server what action player made
		kStateUpdate, // this will be sent to server every frame, it will contain the position and rotation of the player's tank and turret, so the server can update the game state and send it to other clients, takes sf::Int32 for player ID, position and rotation
		kGameEvent, // player killed etc
		kPlayerRealtimeChange, // same as above but for realtime actions, such as holding down the key to move
		kQuit,
		kKeepAlive // dummy packet to keep us connected while in lobby , as server will disconnect us if we don't send any packets for a while, even if we are still connected and waiting for other players to get ready
	};
}

namespace GameActions
{
	enum Type
	{
		kTankExplode,
		kTurretExplode,
		kObstacleDestroyed,
		kEntityDamaged, //tank, obstacle and turret are entities , so we can use this for all of them, just send the ID and the new HP
		kTankHealed,
		kAmmoRefilled,
		kMissileRefilled
	};

	struct Action
	{
		Action() = default;
		Action(Type type, sf::Vector2f position, float amount = 0.f)
			: type(type)
			, position(position)
			, amount(amount)
		{
		}

		Type type;
		sf::Vector2f position;
		float amount; //damage/healing amount or ammo refill amount, depending on the action type
	};
}