#pragma once

#include <vector>
#include <cstdlib>
#include <iostream>
#include <random>


struct Tile {
    enum Kind {
        BOMB,
        WALL,
        EMPTY
    } kind;

    bool revealed;
    char value;
};

struct Map {
    int w;
    int h;
    std::vector<std::vector<Tile>> data;

    Map(int width, int height, int n_bombs) {
        w = width;
        h = height;

        data.resize(w);
        for (int i = 0; i < w; i++) {
            data[i].resize(h);

            for (int k = 0; k < h; k++) {
                data[i][k].revealed = false;
                data[i][k].value = '0';
                data[i][k].kind = Tile::WALL;
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
                    t.value = 'b';
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
};
