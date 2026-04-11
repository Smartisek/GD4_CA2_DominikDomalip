# *** GD4_CA1_DominikDomalip ***

## Armored Assault
![Title Image](./GD4_25_SFML/Media/Textures/TitleScreen.png)
- **Tactical Local Multiplayer Mayhem**
- Engage in high-stakes tank combat across 7 unique environments, from scorching deserts to treacherous underground bunkers. Choose from 4 specialized tank types, each with unique stats.
- **Controls**: Player1 WASD (Move) | L-SHIFT (Sprint) | SPACE (Fire)
                Player2 ARROWS (Move) | R-SHIFT (Sprint) | ENTER (Fire)
- **Tips**: 
        - Automated turret guards the center; dodge its fire to survive!
        - Watch your STAMINA bar; sprinting drains power quickly.
        - Scavenge battlefields for Health and Ammo refills to stay in the fight.
- **Objective**
Outmaneuver and outgun your opponent. The last tank standing wins!


## CA2 Multiplayer with TCP - DATA
- The main data transmission for synchronization happens inside the UpdateClientState function.

#### **Synchronization Packet Structure - Server to Client**
kUpdateClientState packet is being sent to every client at a rate of 60hz.

| Field    |      Data Type   |  Size (Bytes) |           Description             |
|----------|:----------------:|:-------------:|:----------------------------------:|
| PacketType |  uint8_t    |      1           |  Server::PacketType::kUpdateClientState   |
|Tank Count |    uint8_t      |       1     |      Number of tanks in the update                             |
| Tank ID | uint8_t    |       1      |     Unique identifier for the tank                              |
| Position X | float    |       4      |     X-coordinate of the tank                             |
| Position Y | float    |       4      |     Y-coordinate of the tank                             |
| Rotation | float    |       4      |     Rotation angle of the tank                            |
| Hitpoints | uint8_t    |       1      |     Current health                           |
| Current Ammo | uint8_t    |       1      |     Bullets ammount                           |
| Missile Ammo | uint8_t    |       1      |     Missiles amount                           |
| Stamina | float    |       4      |     Current stamina                           |

```
- Total Data per Player (tank): 20 Bytes
- Packet Header: 2 Bytes (Type + Count)
- Total Packet Size for N players: 2+ (20 x N) bytes
```

#### Example of print statement in UpdateClientState of the packet size being sent for two players:
```
[SERVER] Update packet size: 42 bytes
```

#### **Optimization Efforts in Development**
- I am using uint8_t whenever I can instead of regular int - *uint8_t -> 1 byte  VS  int -> 4 bytes* - for fields such as IDs, health or ammo, because they do not need the 32-bit precision of a 4-byte integer, it would be waisting. All those fields will never get above 255 range size so with int there would be 3 bytes per field, per player, every single tick wasted. 
- *By using uint8_t for 5 fields instead of int, I am saving 15 bytes per player. For 8 players at a 60Hz update rate, this is saves about 7.2KB/s of upload bandwidth.*
- Centralized Physics, the server is authorative, performing physics updates in Tick() and only broadcasting the final state, which prevents clients from sending excessive physics data back to server. 

#### **Bandwidth Requirements**
```
m_max_connected_players = 8 and update rate of 60Hz
```
1. #### **Server Upload (Outgoing Data)**
This is the total amount of data the server pushes out to all connected clients. Happening in *UpdateClientState()*
- Every 1/60th of a second, the server sends a single packet containing the status of all connected tanks (players).
- This same packet is received by all clietns connected, each of the 8 playes if max players connected.
##### Calculation brekdown:
```
- One packet size: 2 bytes (header) + (20 bytes per tank x 8 tanks) = 162 bytes
- Data per Player: 162 bytes x 60  times per second = 9,720 bytes/second
- Total for max 8 players: 9,720 bytes/second x 8 players = 77,760 bytes/sec = ~0.62 Mbps (0.62208)
```

2. #### **Server Downloading (Incoming Data)**
This is the total amount of data the server receives from all clients combined. This is being send by each client in the *MultiplayerGameState::Update* function.
- Each client sends its own local status (like position, rotation, etc.) back to the server so the server can maintain its authorative state.
- The server receive one packet from each of the players connected every 1/60th of a second.
##### Calculation brekdown:
```
- One client packet: ~22 bytes (header + local tank data)
- Data from one player: 22 bytes x 60 times per second = 1,320 bytes/second
- Total from all 8 players: 1,320 bytes/second x 8 players = 10,560 bytes/second = ~0.08 Mbps (0.08448)
```

The reason why upload is so much larger is because of the difference of the volume of information sent each packet. 
- Incoming (Client -> Server): Each player only tells the server about themselves, worth 1 tank of data
- Outgoing (Server -> Client): The server must tell every player about everyone else, worth 8 tanks of data 

#### **DKIT Network Estimation and PLayer Support**
I am not sure what is the exact speed for our network in college so I will work with lower such as 100 Mbps even though it should be probably higher. 
1. Current Capacity (8 players)
- Total bandwidth required: ~0.70 Mbps (upload + download combined)
- Network Utilization: On a 100 Mbps connection, 8 players utilize less than 1% of the available bandwidth
- **Conclusion:** The game should run flawlessly on the DKIT network with 8 players, leaving a huge room for other network traffic and maintaining low latency. 

2. Theoretical Scalling (32 players)
To demonstrate the efficiency of using uint8_t and centralized physics, we can estimate requirements for a much larger 32 player match:
- Server Outgoing: 60Hz x 32 players x (2 + 20 x 32) bytes = 9.86 Mbps
- Server Incoming: 60Hz x 32 players x 22 bytes = 0.34 Mbps
- Theorerical total bandwidth = 10.2 Mbps
```
This shows that network bandwidth should not be a bottle neck for this game. More concern should be on CPU of the machine hosting the game running server where it needs to process physics calculations for dozens of tanks simultaneously. 
```

#### **Possible Improvements**
1. Stamina and Rotation
Stamina
- Currently stamina is tracked as float however we could switch to use uint8_t as well to not send 4bytes but only 1byte instead. Since stamina is a value between 0-100, we can easily map it to uint8_t (0-255).
- Before sending, we would multiply the stamina ratio by 255 and then on client side divide by 255 to get a float back. This will save 3 bytes per tank every update.
Rotation
- Rotation could also be changed to be mapped with uint8_t, since angles are 0-360 we can just map it to a single byte (0-255).
- uint8_t compressedRotation = static_cast<uint8_t>((rotation / 360.f) * 255.f);
- Another 3 bytes per tank every update saved.

2. Bit-Packing for Booleans
For booleans we should use bit-tracking instead of sending standalone booleans because, yes boolean is 1 byte, but if we pack many of them and send them that will be 1 byte for every boolean.
- Instead of sending multiple 1-byte booleans, we can use single uint8_t (which is also 1 byte) and assign each boolean to a specific bit slot using bitwise operators.
```
Bit 0: m_is_ready
Bit 1: isSprinting
Bit 2: isFiring
and so on... up to 8 states for use of 1 byte
```
This is not a big deal in my current game, this is just optimazation for future where we could save a lot of space. Since server updates 60Hz, these small savings can get significant over time if we add more gameplay features.
For example:
- Without Bit-Packing: if we have 8 booleans for 8 players, we are sending 64 bytes of boolean data every tick. At 60Hz this gets to 3,840 bytes per second.
- With Bit-Packing: We pack those same 8 booleans into 1 byte per player. Now we would be sending only 8 bytes per tick. At 60Hz this would only be 480 bytes per second.
- The result is badwidth for those states reduced by more than 85%.
==============================================================================================================================================================================================================================

### REFERENCES 
- Freesound. (2025). Session Beat by Lit1onion. [online] Available at: https://freesound.org/people/Lit1onion/sounds/784081/ [Accessed 17 Feb. 2026].
- Freesound. (2018). Mushroom Background Music by Sunsai. [online] Available at: https://freesound.org/people/Sunsai/sounds/415804/ [Accessed 17 Feb. 2026]
- freesound.org. (n.d.). Freesound - Tank Firing by qubodup. [online] Available at: https://freesound.org/people/qubodup/sounds/168707/ [Accessed 17 Feb. 2026]
- Freesound. (2016). Sizzling boom by AceOfSpadesProduc100. [online] Available at: https://freesound.org/people/AceOfSpadesProduc100/sounds/340960/ [Accessed 17 Feb. 2026]
- Freesound. (2020). button-selected.wav by StavSounds. [online] Available at: https://freesound.org/people/StavSounds/sounds/546079/ [Accessed 17 Feb. 2026]
- Freesound. (2020). Select, Granted 04.wav by LilMati. [online] Available at: https://freesound.org/people/LilMati/sounds/515823/ [Accessed 17 Feb. 2026]
- Freesound. (2026). Bullet_Impact_2 by toxicwafflezz. [online] Available at: https://freesound.org/people/toxicwafflezz/sounds/150838/ [Accessed 17 Feb. 2026] 
- Freesound. (2023). Car Impact Hit, Shatter, No Glass by PNMCarrieRailfan. [online] Available at: https://freesound.org/people/PNMCarrieRailfan/sounds/681527/ [Accessed 17 Feb. 2026]
- Freesound. (2020). Powerup 04.wav by LilMati. [online] Available at: https://freesound.org/people/LilMati/sounds/503520/ [Accessed 17 Feb. 2026]
- Freesound. (2025). Smashed/demolished brick wall crumbling/caving in by Squirrel_404. [online] Available at: https://freesound.org/people/Squirrel_404/sounds/829103/ [Accessed 17 Feb. 2026]
- The Spriters Resource. (2026). Box Backgrounds - Pokémon Black / White - DS / DSi. [online] Available at: https://www.spriters-resource.com/ds_dsi/pokemonblackwhite/asset/34025/ [Accessed 17 Feb. 2026]
- itch.io. (n.d.). Stones & Brick Textures by Pucci Games. [online] Available at: https://pucci-games.itch.io/stones-brick-textures [Accessed 17 Feb. 2026]
- Pngegg.com. (2026). Free download | Computer hardware, Gun Turret, hardware, machine png | PNGEgg. [online] Available at: https://www.pngegg.com/en/png-tovsr/download [Accessed 17 Feb. 2026]#
- Moreira, Artur, Jan Haller, and Henrik Vogelius Hansson. SFML Game Development. Packt Publishing, 2013
- Gemini. (n.d.). Google Gemini picture generation. [online] Available at: https://gemini.google.com/app?hl=en_GB.
- Clipart Library (2026). Illustration. [online] Clipart-library.com. Available at: https://clipart-library.com/clip-art/49-495275_bullets-clipart-sprite-bullets-sprite.htm [Accessed 20 Feb. 2026].
- Freesound. (2022). TF_movie_Optimus_inspired_laser_sound_effect_01_2022 by Artninja. [online] Available at: https://freesound.org/people/Artninja/sounds/784935/ [Accessed 20 Feb. 2026].
- Freesound. (2016). Launching 1 by AceOfSpadesProduc100. [online] Available at: https://freesound.org/people/AceOfSpadesProduc100/sounds/334268/ [Accessed 20 Feb. 2026].
