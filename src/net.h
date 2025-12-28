#pragma once

#include <enet/enet.h>
#include <stdio.h>

#include <string>

#include "map.h"

enum Channel {
    MESSAGE,
    MAP,
    LAST,
};

typedef struct { 
    ENetAddress addr;
    ENetHost *host;
    // ENetPeers *peer[];
} Server;

typedef struct { 
    ENetAddress addr;
    ENetHost *host; 
    ENetPeer *peer;
    int connected;
} Client;

inline Server server_init() {
    Server server = {0};

    if (enet_initialize() != 0) {
        printf("Enet failed to initialize");
        return {};
    }

    server.addr.host = ENET_HOST_ANY;
    server.addr.port = 7777;

    // up to 32 clients, CH_LAST channels, 0 means any amount of incoming/outgoing bandwidth
    server.host = enet_host_create(&server.addr, 32, Channel::LAST, 0, 0);

    if (!server.host) {
        printf("error while creating server host\n");
        return {};
    }

    printf("server started\n");
    
    return server;
}

inline void server_update(Server *server) {
    ENetEvent event;

    std::string name = "client";
    
    while (enet_host_service(server->host, &event, 0 /*non-blocking*/) > 0) { 
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("a new client connected from %x:%u.\n", 
                   event.peer->address.host, 
                   event.peer->address.port);
            event.peer->data = name.data();
        }

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID == Channel::MESSAGE) {
                printf("'%s' %s\n",
                    event.packet->data,
                    (char*)event.peer->data
                );
            }

            enet_packet_destroy(event.packet);
        }

        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("%s disconnected.\n", (char*)event.peer->data);
            event.peer->data = NULL;
        }
    }
}


Client client_init() {
    Client client = {0};
    
    if (enet_initialize() != 0) {
        printf("Enet failed to initialize\n");
        return client;
    }

    enet_address_set_host(&client.addr, "localhost");
    client.addr.port = 7777;

    // allow only 1 outgoing connection, CH_LAST channels, any bandwidth
    client.host = enet_host_create(NULL, 1, Channel::LAST, 0, 0);
    if (!client.host) {
        printf("error while creating client host\n");
        return client;
    }

    client.peer = enet_host_connect(client.host, &client.addr, Channel::LAST, 0);
    if (!client.peer) {
        printf("error while connecting to server\n");
        return client;
    }

    printf("client trying to connect\n");

    return client;
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
            // event.channelID
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
    
    enet_deinitialize();
}

void client_send_map_to_server(Client *client, Map &map) {
    enet_peer_send(client->peer, Channel::MAP, enet_packet_create(
        &map,
        sizeof(Map),
        ENET_PACKET_FLAG_RELIABLE
    ));

    enet_host_flush(client->host);
}

void server_broadcast_map(Server *server, Map &map) {
    enet_host_broadcast(server->host, Channel::MAP, enet_packet_create(
        &map,
        sizeof(Map),
        ENET_PACKET_FLAG_RELIABLE
    ));

    enet_host_flush(server->host);
}
