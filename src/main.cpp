#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <utilapiset.h>
#include <winsock2.h>
#include <windows.h>

#include <conio.h>
#include <enet/enet.h>
#include <iostream>
#include <string>

#include "map.h"
#include "net.h"


#define BLACK         "\033[30m"
#define RED           "\033[31m"
#define GREEN         "\033[32m"
#define YELLOW        "\033[33m"
#define BLUE          "\033[34m"
#define MAGENTA       "\033[35m"
#define CYAN          "\033[36m"
#define WHITE         "\033[37m"

#define DARK_GRAY     "\033[90m"
#define LIGHT_RED     "\033[91m"
#define LIGHT_GREEN   "\033[92m"
#define LIGHT_YELLOW  "\033[93m"
#define LIGHT_BLUE    "\033[94m"
#define LIGHT_MAGENTA "\033[95m"
#define LIGHT_CYAN    "\033[96m"
#define GLOWING_WHITE "\033[97m"

#define RESET_COL         "\033[0m"
#define CLEAR         "\033[2J"
#define RESET_CURSOR_POS  "\033[H"
#define SHOW_CURSOR "\033[?25h"
#define HIDE_CURSOR "\033[?25l"

#define FRAME_DELAY 16


enum Game{
    PLAYING,
    MENU
};


std::tuple<int, int> init(){
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    int rows, columns;
  
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top;
    columns = csbi.srWindow.Right - csbi.srWindow.Left;

    columns = 30;
    rows = 20;
    return std::make_tuple(columns, rows);
}


inline const std::string& color_from_number(char v) {
    static const std::string colors[9] = {
        "\033[38;5;240m",
        "\033[38;5;19m",
        "\033[38;5;28m",
        "\033[38;5;124m",
        "\033[38;5;21m",
        "\033[38;5;196m",
        "\033[38;5;202m",
        "\033[38;5;171m",
        "\033[38;5;142m"
    };
    return colors[v - '0'];
}


void game_loop(bool &running, bool is_server, Server &server, Client &client, Map &map, Input &input){
    if (is_server) {
        // is both CLIENT and SERVER
        server_update(&server, map); // handle inputs from all clients

        enet_host_flush(server.host);

        if (map.over){
            std::cout << CLEAR << RESET_CURSOR_POS << SHOW_CURSOR << RESET_COL << "\033[?25l" << "Game over! Do you want to replay? (y/n)\n>> ";
            char option;
            std::cin >> option;

            if (option == 'y' || option == 'Y'){
                map = Map(map.w, map.h, map.start_bombs);
            } else {
                std::cout << "Exitting application...";
                running = false;
                return;
            }
        }
    }
    
    client_update(&client);

    client_send_input_to_server(&client, input);
    enet_host_flush(client.host);

    // -------- INPUT --------
    input.reset();
    running = handle_input_client(input);

    // -------- BOUNDARY --------
    if (input.x < 0) input.x = 0;
    if (input.y < 0) input.y = 0;
    if (input.x >= map.w) input.x = map.w - 1;
    if (input.y >= map.h) input.y = map.h - 1;

    // -------- DRAW/RENDER --------
    if (!client.recv_map) {
        return;
    }
    
    std::string frame;
    frame += CLEAR;
    frame.reserve(client.recv_map->w * map.h + client.recv_map->h);

    for (int i = 0; i < client.recv_map->h; i++){
        std::string line;
        std::string last_color = RESET_COL;

        for (int j = 0; j < client.recv_map->w; j++){
            Tile &t = client.recv_map->get(j, i);
            std::string color = WHITE;
            char display;

            if (t.revealed){
                if (t.kind == Tile::BOMB){
                        color = RED;
                        display = '!';
                } else {
                    color = color_from_number(t.value);
                    display = t.value;
                }
            }
            else if (t.flagged){
                color = YELLOW;
                display = 'F';
            }
            else{
                color = WHITE;
                display = '#';
            }
            
            for (int p = 0; p < MAX_PLAYERS; p++){ // pseudo code for rendering players
                if (client.players.initialized[p] == false) {
                    continue;
                }
                
                int px = client.players.positions[p].x;
                int py = client.players.positions[p].y;

                if (py == i && px == j) {
                    color = CYAN;
                    display = 'X';
                }
            }

            if (color != last_color){
                line += color;
                last_color = color;
            }

            line += display;
            line += ' '; // optional for better looks :)
        }

        if (i == 1){
            line += RESET_COL;
            line += "  Bombs: ";
            line += std::to_string(map.bombs);
        }

        line += RESET_COL;
        frame += line + '\n';
    }

    std::cout << RESET_CURSOR_POS << frame << std::flush;

    if (map.bombs == 0) map.check_win();
}

void menu_loop(Game& state, bool &running, bool &is_server, Client& client, Server& server) {
    int option = 0;
    int old_option = option;
    std::string options[3] = { // i could use code to align the texts but come on
        "Host Game",
        "Join Game",
        "Quit"
    };

    int option_count = std::size(options);

    size_t max_len = 0;
    for (int i = 0; i < option_count; i++) {
        max_len = std::max(max_len, options[i].length());
    }
    
    bool selected = false;
    std::cout << CLEAR << RESET_COL;
    while (!selected){
        std::cout << RESET_CURSOR_POS;

        std::cout << "Welcome to " << GLOWING_WHITE << "MineSweeper!" << RESET_COL << "\n\n\n";

        for (size_t i = 0; i < std::size(options); i++){
            size_t pad = max_len - options[i].length();
            size_t left = pad / 2;
            size_t right = pad - left;

            std::string left_pad(left, ' ');
            std::string right_pad(right, ' ');

            if (option == (int)i) {
                std::cout << GLOWING_WHITE
                        << ">> "
                        << left_pad
                        << options[i]
                        << right_pad
                        << " <<\n"
                        << RESET_COL;
            } else { // print empty space to mimic clear
                std::cout << "   "
                        << left_pad
                        << options[i]
                        << right_pad
                        << "   \n";
            }
        }
        std::cout << std::flush;

        if (_kbhit()){
            int k = _getch();
            if (k == 0 || k == 224){
                int key = _getch();
                if (key == 72)
                    option--;
                if (key == 80)
                    option++;
            } else{
                if (k == 'w' || k == 'W') option--;
                if (k == 's' || k == 'S') option++;
                if (k == ' ' || k == 13){ // space or enter
                    selected = true;
                    Beep(2000, 100);
                }
                if (k == 27){
                    running = false;
                    return;
                }
            }

            if (option < 0) option = 2;
            else if (option > 2) option = 0;

            if (option != old_option){
                Beep(600, 80);

                old_option = option;
            }
        }

        if (selected){
            std::cout << SHOW_CURSOR << CLEAR;
            if (option == 0) {
                std::string input;
                std::cout << "insert port to host the server:\n" << SHOW_CURSOR;
                std::cin >> input;

                if (input == "q" || input == "Q"){
                    selected = false;
                    std::cout << HIDE_CURSOR << CLEAR;
                    continue;
                }

                int port = std::stoi(input);

                server = server_init(port);

                client_connect(&client, "localhost", port);

                is_server = true;
            } else if (option == 1) {
                std::cout << "Write the server's address:\n>> ";
                std::string addr;
                std::cin >> addr;

                if (addr == "q" || addr == "Q"){
                    selected = false;
                    std::cout << HIDE_CURSOR << CLEAR;
                    continue;
                }
                
                std::cout << "Now the port:\n>>";
                std::string input;
                std::cin >> input;

                if (input == "q" || input == "Q"){
                    selected = false;
                    std::cout << HIDE_CURSOR << CLEAR;
                    continue;
                }

                int port = stoi(input);
                
                client_connect(&client, addr.data(), port);
            } else if (option == 2){
                running = false;
            }
            state = Game::PLAYING;
            std::cout << HIDE_CURSOR;
        }

        Sleep(FRAME_DELAY);
    }
}


int main() {
    auto [WIDTH, HEIGHT] = init();
    
    Client client;
    Server server;
    bool is_server = false;

    if (enet_initialize() != 0) {
        printf("ENet failed to initialize\n");
        return 1;
    }

    std::cout << HIDE_CURSOR;
    
    client = client_init();

    Map map( WIDTH, HEIGHT, WIDTH*HEIGHT/10);
    Input input(5, 5);

    Game game = Game::MENU;

    bool running = true;
    while (running) {
        if (game == Game::MENU){
            menu_loop(game, running, is_server, client, server);
        } else if (game == Game::PLAYING){
            game_loop(running, is_server, server, client, map, input);
        }

        Sleep(FRAME_DELAY);
    }
    std::cout << SHOW_CURSOR;
    client_cleanup(&client);
    enet_deinitialize();
    
    return 0;
}
