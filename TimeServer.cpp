#include <iostream>
#include <string.h>
#include <ctime>

#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#endif

#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif

#pragma comment(lib, "ws2_32.lib")



using namespace std;

int main()
{
    time_t timer;
    time(&timer);

    char time_msg[26];
    ctime_s(time_msg, 26, &timer);
    printf("Local time is: %s", time_msg);

    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to init");
        return 1;
    }

    printf("Configuring local address...\n");

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if (socket_listen == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d", WSAGetLastError());
        return 1;
    }

    int option = 0;
    if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&option, sizeof(option))) {
        fprintf(stderr, "setsockopt() failed: %d", WSAGetLastError());
        return 1;
    }

    printf("Binding socket to local address...\n");

    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed: %d", WSAGetLastError());
        return 1;
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");

    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed: %d", WSAGetLastError());
        return 1;
    }

    printf("Waiting for connection...\n");

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);

    if (socket_client == INVALID_SOCKET) {
        fprintf(stderr, "accept() failed: %d", WSAGetLastError());
        return 1;
    }

    printf("Client connected...\n");
    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("%s\n", address_buffer);

    printf("Reading request...\n");
    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);
    printf("Received %d bytes\n", bytes_received);
    printf("%.*s\n", bytes_received, request);

    printf("Sending response...");
    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";
    int bytes_sent = send(socket_client, response, strlen(response), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
    printf("Sent %d of %d bytes", bytes_sent, strlen(time_msg));

    printf("Closing connection...\n");
    closesocket(socket_client);

    printf("Closing listening socket...\n");
    closesocket(socket_listen);

    WSACleanup();

    printf("Finished.\n");

    return 0;

}