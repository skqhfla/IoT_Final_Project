# Zombie Evade
---
## Game Description
This is a multiplayer zombie game built with Boost.Asio as the underlying networking API. 
- The server manages the positions of all objects during the game. Based on the client's requests (keyboard input), the server updates the positions of the objects. 
- The positions of the zombies are updated every second. 
- The updated positions are broadcasted to each player, and the client program displays them on the graphical interface.
---
## Game Play
You will select a character.
- Characters can be chosen using the number keys on the keyboard.

Once all players have joined, they can move their characters using W, A, S, D.
They must avoid randomly moving zombies.
- The speed of the zombies gradually increases over time.
Players who come into contact with a zombie will turn into a zombie.
Players who come into contact with a player-turned-zombie will also turn into a zombie.
If there is only one player remaining who is not a zombie, the game ends.
---
## Dependency

[Boost.Asio](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio.html)
- Installation command: ```$ sudo apt-get install libboost-all-dev```

[gtkmm](https://gtkmm.org/en/index.html)
- Installation command: ```$ sudo apt-get install libgtkmm-3.0-dev```
---
## Command Sequence: Steps to run the program.

### Server
```$ ./src/game_server <port>```
  
### Client
```$ ./src/game_client <ip address> <port>```
