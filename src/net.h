#pragma once

#include <enet/enet.h>
#include <stdio.h>
#include <conio.h>

#include <string>

#include "map.h"

enum Channel {
    MESSAGE,
    CLIENT_INPUT,
    PLAYERS,
    MAP,
    LAST,
};

struct Input {
    bool flagging;
    bool interacting;
    int x; int y;
    
    Input(int new_x,  int new_y){
        flagging = false;
        interacting = false;
        x = new_x;
        y = new_y;
    }

    void reset(){
        flagging = false;
        interacting = false;
    }
};

struct v2 { int x; int y;};

#define MAX_PLAYERS 4

struct Players {
    bool initialized[MAX_PLAYERS];
    v2 positions[MAX_PLAYERS];
};

typedef struct { 
    ENetAddress addr;
    ENetHost *host;

    int num_connected;
    Players players;
    // ENetPeers *peer[];
} Server;

typedef struct { 
    ENetAddress addr;
    ENetHost *host; 
    ENetPeer *peer;
    int connected;

    Map *recv_map;

    Players players;
} Client;

inline Server server_init(int port) {
    Server server = {};

    server.addr.host = ENET_HOST_ANY;
    server.addr.port = port;

    // CH_LAST channels, 0 means any amount of incoming/outgoing bandwidth
    server.host = enet_host_create(&server.addr, MAX_PLAYERS, Channel::LAST, 0, 0);

    if (!server.host) {
        printf("error while creating server host\n");
        return {};
    }

    printf("server started\n");
    
    return server;
}

inline void handle_input_server(Input& input, Map &map){
    if (input.flagging){
        Tile &t = map.get(input.x, input.y);
        if (t.flagged){
            map.bombs += 1;
        } else{
            map.bombs -= 1;
        }
        t.flagged ^= 1; // flag on / flag off
    }
    if (input.interacting){ // use tile
        if (!map.get(input.x, input.y).flagged){
            use_tile(map, input.x, input.y);
        }
    }
}


inline bool handle_input_client(Input &input){
    if (_kbhit()) { // if key pressed...
        int c = _getch();

        if (c == 0 || c == 224) { // get special characters
            int key = _getch();
            
            if (key == 72) input.y--;
            if (key == 80) input.y++;
            if (key == 75) input.x--;
            if (key == 77) input.x++;
        } else { // do normal keyboard handling
            if (c == 'w' || c == 'W') input.y--;
            if (c == 's' || c == 'S') input.y++;
            if (c == 'a' || c == 'A') input.x--;
            if (c == 'd' || c == 'D') input.x++;
            if (c == 'q') {
                std::cout << "\033[?25h";
                return false;
            }
            if (c == 'f'){
                input.flagging = true;
            }
            if (c == ' '){ // use tile
                input.interacting = true;
            }
        }
    }

    return true;
}


// void server_broadcast_map(Server *server, Map &map) {
//     enet_host_broadcast(server->host, Channel::MAP, enet_packet_create(
//         &map,
//         sizeof(Map),
//         ENET_PACKET_FLAG_RELIABLE
//     ));
// }

inline void server_broadcast_map(Server *server, Map &map) {
    // Calculate total size: 2 ints (w, h) + all tiles
    size_t total_size = sizeof(int) * 2 + sizeof(Tile) * map.w * map.h;
    uint8_t* buffer = new uint8_t[total_size];
    
    // Serialize dimensions
    int* dims = (int*)buffer;
    dims[0] = map.w;
    dims[1] = map.h;
    
    // Serialize tile data
    Tile* tiles = (Tile*)(buffer + sizeof(int) * 2);
    for (int x = 0; x < map.w; x++) {
        for (int y = 0; y < map.h; y++) {
            tiles[y * map.w + x] = map.data[x][y];
        }
    }
    
    enet_host_broadcast(server->host, Channel::MAP, enet_packet_create(
        buffer,
        total_size,
        ENET_PACKET_FLAG_RELIABLE
    ));
    
    delete[] buffer;
}

inline void server_broadcast_players(Server* server) {
    enet_host_broadcast(server->host, Channel::PLAYERS, enet_packet_create(
        &server->players,
        sizeof(Players),
        ENET_PACKET_FLAG_RELIABLE
    ));
}

inline void server_update(Server *server, Map& map) {
    ENetEvent event;

    std::string name = "client";
    
    while (enet_host_service(server->host, &event, 0 /*non-blocking*/) > 0) { 
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("a new client connected from %x:%u.\n", 
                   event.peer->address.host, 
                   event.peer->address.port);
            int* player_id = new int(server->num_connected); // unique ID
            event.peer->data = player_id;

            server->players.initialized[*player_id] = true;
            server->num_connected++;
        }

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID == Channel::MESSAGE) {
                printf("'%s' %s\n",
                    event.packet->data,
                    (char*)event.peer->data
                );
            }

            if (event.channelID == Channel::CLIENT_INPUT) {
                Input *input = (Input*)event.packet->data;

                handle_input_server(*input, map);

                int player_id = *(int*)event.peer->data;
                v2 pos = {.x = input->x, .y = input->y};
                server->players.positions[player_id] = pos;
            }

            enet_packet_destroy(event.packet);
        }

        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            int player_id = *(int*)event.peer->data;
            printf("Player %d disconnected.\n", player_id);
            server->players.initialized[player_id] = false;

            delete (int*)event.peer->data;
            event.peer->data = NULL;
            
            server->num_connected--;
        }
    }

    server_broadcast_map(server, map);
    server_broadcast_players(server);
}


inline Client client_init() {
    Client client = {};

    client.host = enet_host_create(
        NULL,
        1,
        Channel::LAST,
        0, 0
    );
    if (!client.host) {
        printf("failed to create client's host");
    }

    return client;
}

inline void client_connect(Client *client, const char *addr, int port) {
    enet_address_set_host(&client->addr, addr);
    client->addr.port = port;

    client->peer = enet_host_connect(
        client->host,
        &client->addr,
        Channel::LAST,
        0
    );

    if (!client->peer) {
        printf("No available peers for connection\n");
        return;
    }

    printf("Client attempting to connect\n");
}



inline void client_update(Client *client) {
    ENetEvent event;

    while (enet_host_service(client->host, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("connected to server\n");
            // client->peer->data = "client data";
            client->connected = 1;
        }

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID == Channel::MAP) {
                // client->recv_map = (Map*)event.packet->data;

                uint8_t* buffer = event.packet->data;
                int* dims = (int*)buffer;
                int w = dims[0];
                int h = dims[1];
                
                // Recreate the map if needed
                if (!client->recv_map) {
                    client->recv_map = new Map(w, h, 0); // 0 bombs for deserialize
                }
                
                // Deserialize tiles
                Tile* tiles = (Tile*)(buffer + sizeof(int) * 2);
                for (int x = 0; x < w; x++) {
                    for (int y = 0; y < h; y++) {
                        client->recv_map->data[x][y] = tiles[y * w + x];
                    }
                }
            }

            if (event.channelID == Channel::PLAYERS) {
                client->players = *(Players*)event.packet->data;
            }
                        
            enet_packet_destroy(event.packet);
        }

        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("disconnected from server\n");
            client->connected = 0;
            client->peer->data = NULL;
        }
    }
}


inline void client_cleanup(Client *client) {
    if (client->peer && client->connected) {
        enet_peer_disconnect(client->peer, 0);
        
        // wait up to 200ms for the server to acknowledge the disconnect
        ENetEvent event;
        while (enet_host_service(client->host, &event, 200) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                printf("Disconnected from server.\n");
        }
    }
    
    if (client->host) {
        enet_host_destroy(client->host);
    }

    delete client->recv_map;
    client->recv_map = nullptr;
}

inline void client_send_input_to_server(Client *client, Input &input) {
    enet_peer_send(client->peer, Channel::CLIENT_INPUT, enet_packet_create(
        &input,
        sizeof(Input),
        ENET_PACKET_FLAG_RELIABLE
    ));
}
