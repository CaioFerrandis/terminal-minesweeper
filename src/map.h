#pragma once

#include <vector>
#include <cstdlib>
#include <iostream>
#include <random>


struct Tile {
    enum Kind {
        BOMB,
        WALL
    } kind;

    bool flagged;
    bool revealed;
    char value;
};

struct Map {
    int w;
    int h;
    std::vector<std::vector<Tile>> data;

    bool over;
    int start_bombs;
    int bombs;

    Map(int width, int height, int n_bombs) {
        over = false;
        w = width;
        h = height;
        start_bombs = n_bombs;
        bombs = start_bombs;

        data.resize(w);
        for (int i = 0; i < w; i++) {
            data[i].resize(h);

            for (int k = 0; k < h; k++) {
                data[i][k].revealed = false;
                data[i][k].value = '0';
                data[i][k].kind = Tile::WALL;
                data[i][k].flagged = false;
            }

        }

        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<int> x_dist(0, width-1);
        std::uniform_int_distribution<int> y_dist(0, height-1);
        
        for (int i = 0; i < n_bombs; i++){
            // choose random cell to become a bomb
            bool worked = false;
            int x, y;
            while (!worked){
                x = x_dist(rng);
                y = y_dist(rng);
                Tile &t = get(x, y);

                if (t.kind != Tile::BOMB){ // check that chosen cell is not already a bomb
                    t.kind = Tile::BOMB;
                    t.value = '0';
                    t.flagged = false;
                    worked = true;
                }
            }

            // get cells near current bomb and update them
            for (int dx = -1; dx <= 1; dx++){
                for (int dy = -1; dy <= 1; dy++){
                    if (dx == 0 && dy == 0) continue; // skips itself
                    int nx = x + dx;
                    int ny = y + dy;

                    // is it inside the map?
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;

                    Tile &neighbor = get(nx, ny);
                    if (neighbor.kind != Tile::BOMB){
                        if (neighbor.value >= '0' && neighbor.value < '9')
                            neighbor.value += 1;
                        else
                            neighbor.value = '1'; // ensure that tile's value is actually a number
                    }
                }
            }
        }
    }

    Tile& get(int x, int y){
        if (x < 0 || y < 0 || x >= w || y >= h){
            std::cerr << "Coordinate outside of map:\n    x: " << x << "/ w: " << w << "\n    y: " << y << "/ h: " << h <<"\n";
            std::exit(1);
        }

        return data[x][y];
    }

    void check_win(){
        int count = 0;

        for (int y = 0; y < h; y++){
            for (int x = 0; x < w; x++){
                Tile &t = get(x, y);
                if (t.flagged && t.kind == Tile::BOMB){
                    count += 1;
                }
            }
        }

        if (count == start_bombs) over = true;
    }
};

inline void flood_fill(Map &map, int x, int y){
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

 inline void use_tile(Map &map, int x, int y){
    Tile &tile = map.get(x, y);
    tile.revealed = true;

    if (tile.kind == Tile::BOMB){
        map.bombs -= 1;
        map.over = true;
    } else if (tile.value == '0'){
        flood_fill(map, x, y);
    }
}