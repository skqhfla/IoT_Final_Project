all:
	g++ game_client.cpp -o game_client -std=c++11 `pkg-config --cflags --libs gtkmm-3.0`
	g++ game_server.cpp -o game_server -std=c++11
