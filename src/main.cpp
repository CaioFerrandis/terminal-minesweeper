#include <cstdio>
#include <cstdlib>
#include <winsock2.h>
#include <windows.h>

#include <enet/enet.h>
#include <conio.h>
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

#define RESET         "\033[0m"
// im sorry i like making my code feel like its from the 1980s alr



enum Key {
    UP = 72,
    DOWN = 80,
    LEFT = 75,
    RIGHT = 77
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

    return std::make_tuple(columns, rows);
}


void flood_fill(Map &map, int x, int y){
    Tile &t = map.get(x, y);
    if (t.value != '0') return; // make sure its a 0
    t.revealed = true;

    for (int dx = -1; dx <= 1; dx++){
        for (int dy = -1; dy <= 1; dy++){
            if (dx == 0 && dy == 0) continue; // skips itself
            int nx = x + dx;
            int ny = y + dy;

            // is it inside the map?
            if (nx < 0 || ny < 0 || nx >= map.w || ny >= map.h) continue;

            Tile &neighbor = map.get(nx, ny);
            if (neighbor.revealed) continue;

            neighbor.revealed = true;
            if (neighbor.value == '0'){ // do the flood
                flood_fill(map, nx, ny);
            }
        }
    }
}


void use_tile(Map &map, int x, int y){
    Tile &tile = map.get(x, y);
    tile.revealed = true;

    if (tile.kind == Tile::BOMB){
        // exit(0);
    } else if (tile.value == '0'){
        flood_fill(map, x, y);
    }
}


bool handle_input(int &x, int &y, Map &map){
    if (_kbhit()) { // if key pressed...
        int c = _getch();

        if (c == 0 || c == 224) { // get special characters
            int key = _getch();
            if (key == UP)    y--;
            if (key == DOWN)  y++;
            if (key == LEFT)  x--;
            if (key == RIGHT) x++;
        } else { // do normal keyboard handling
            if (c == 'q') {
                std::cout << "\033[?25h";
                return false;
            }
            if (c == ' '){
                use_tile(map, x, y);
            }
        }
    }

    return true;
}


int main(int argc, char* argv[]) {
    auto [WIDTH, HEIGHT] = init();
    
    Client client;
    Server server;
    bool is_server = false;
    
    if (argc > 1) {
        if (strcmp(argv[1], "server") == 0){
            server = server_init();
            client = client_init();
            is_server = true;
        } else if (strcmp(argv[1], "client") == 0){
            client = client_init();
        }
    }
    
    int x = 5;
    int y = 5;

    std::cout << "\033[?25l"; // hide cursor

    Map map( WIDTH, HEIGHT, WIDTH*HEIGHT/10);

    bool running = true;
    while (running) {
        if (is_server) {
            server_update(&server);
            server_broadcast_map(&server, map);
        }

        // -------- INPUT --------
        running = handle_input(x, y, map);

        // -------- BOUNDARY --------
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= map.w) x = map.w - 1;
        if (y >= map.h) y = map.h - 1;

        // -------- DRAW/RENDER --------
        std::string frame;
        frame.reserve(map.w * map.h + map.h);

        for (int i = 0; i < map.h; i++){
            std::string line;
            std::string last_color = RESET;

            for (int j = 0; j < map.w; j++){
                Tile &t = map.get(j, i);
                std::string color = WHITE;
                char display;

                if (i == y && j == x) {
                    color = CYAN;
                    display = 'X';
                }
                else if (t.revealed){
                    if (t.kind == Tile::BOMB){
                         color = RED;
                         display = '!';
                    } else {
                        color = GREEN;
                        display = t.value;
                    }
                } else{
                    color = WHITE;
                    display = '#';
                }

                if (color != last_color){
                    line += color;
                    last_color = color;
                }

                line += display;
            }

            line += RESET;
            frame += line + '\n';
        }

        std::cout << "\033[H" << frame << std::flush;

        // -------- FRAME --------
        Sleep(16);
    }
    
    client_cleanup(&client);
    enet_deinitialize();
    
    return 0;
}
