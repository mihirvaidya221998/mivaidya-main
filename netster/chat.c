#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>

int connection = 0;

// Server function
void chat_server(char *iface, long port, int use_udp) {
    struct addrinfo hints;
    struct addrinfo *result;

    char str_port[256];
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    char received_message[256];
    char response_message[256];
    char host_buffer[256];
    char service_buffer[256];
    socklen_t client_addr_len;

    memset(received_message, 0, sizeof(received_message));

    // Set up hints for getaddrinfo
    hints.ai_family = AF_INET;

    if (use_udp == 1) {
        hints.ai_socktype = SOCK_DGRAM;
    } else {
        hints.ai_socktype = SOCK_STREAM;
    }

    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    sprintf(str_port, "%ld", port);
    int getaddr_result = getaddrinfo(iface, str_port, &hints, &result); // Creating a getaddrinfo

    if (getaddr_result != 0) {
        exit(EXIT_FAILURE);
    }

    // Create a socket
    server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (server_socket < 0) {
        perror("Socket error");
        exit(1);
    }

    // Set up server address and bind
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_result = bind(server_socket, result->ai_addr, result->ai_addrlen);

    if (bind_result < 0) {
        perror("Binding error");
        exit(1);
    }

    client_addr_len = result->ai_addrlen;

    if (use_udp == 1) {
        for (;;) {
            memset(response_message, 0, sizeof(response_message));
            memset(received_message, 0, sizeof(received_message));
            recvfrom(server_socket, received_message, sizeof(received_message), 0, (struct sockaddr *)&client_address, &client_addr_len);
            getnameinfo((struct sockaddr *)&client_address, sizeof(client_address), host_buffer, sizeof(host_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
            printf("got message from ('%s', %s)\n", host_buffer, service_buffer);

            if (strcmp(received_message, "hello\n") == 0) {
                strcpy(response_message, "world\n");
                sendto(server_socket, response_message, strlen(response_message), 0, (struct sockaddr *)&client_address, client_addr_len);
            } else if (strcmp(received_message, "goodbye\n") == 0) {
                strcpy(response_message, "farewell\n");
                sendto(server_socket, response_message, strlen(response_message), 0, (struct sockaddr *)&client_address, client_addr_len);
            } else if (strcmp(received_message, "exit\n") == 0) {
                strcpy(response_message, "ok\n");
                sendto(server_socket, response_message, strlen(response_message), 0, (struct sockaddr *)&client_address, client_addr_len);
                exit(0);
            } else {
                sendto(server_socket, received_message, sizeof(received_message), 0, (struct sockaddr *)&client_address, client_addr_len);
            }
        }
    } else {
        socklen_t client_len = sizeof(client_address);
        listen(server_socket, 10);

        while ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len)) > 0) {
            getnameinfo((struct sockaddr *)&client_address, sizeof(client_address), host_buffer, sizeof(host_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
            printf("connection %d from ('%s', %s)\n", connection++, host_buffer, service_buffer);
            int child_pid, n;

            if ((child_pid = fork()) == 0) {
                close(server_socket);
                memset(received_message, 0, sizeof(received_message));
                memset(response_message, 0, sizeof(response_message));

                while ((n = recv(client_socket, received_message, sizeof(received_message), 0)) > 0) {
                    getnameinfo((struct sockaddr *)&client_address, sizeof(client_address), host_buffer, sizeof(host_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
                    printf("got message from ('%s', %s)\n", host_buffer, service_buffer);

                    if (strcmp(received_message, "hello\n") == 0) {
                        strcpy(response_message, "world\n");
                        send(client_socket, response_message, strlen(response_message), 0);
                    } else if (strcmp(received_message, "goodbye\n") == 0) {
                        strcpy(response_message, "farewell\n");
                        send(client_socket, response_message, strlen(response_message), 0);
                    } else if (strcmp(received_message, "exit\n") == 0) {
                        strcpy(response_message, "ok\n");
                        send(client_socket, response_message, strlen(response_message), 0);
                        kill(child_pid, SIGINT);
                    } else {
                        send(client_socket, received_message, strlen(received_message), 0);
                    }

                    memset(received_message, 0, sizeof(received_message));
                    memset(response_message, 0, sizeof(response_message));
                }

                close(client_socket);
                exit(0);
            }
        }
    }
}

// Client function
void chat_client(char *host, long port, int use_udp) {
    struct addrinfo hints;
    struct addrinfo *result;
    int client_socket, bytes_sent;
    struct sockaddr_in server_address;
    char user_input[256];
    char received_message[256];
    char str_port[256];
    int getaddr_result;
    socklen_t server_addr_len;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;

    if (use_udp == 1) {
        hints.ai_socktype = SOCK_DGRAM;
    } else {
        hints.ai_socktype = SOCK_STREAM;
    }

    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    sprintf(str_port, "%ld", port);
    getaddr_result = getaddrinfo(host, str_port, &hints, &result);

    if (getaddr_result != 0) {
        exit(EXIT_FAILURE);
    }

    client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (client_socket < 0) {
        perror("[-]Socket error");
        exit(1);
    }

    if (use_udp == 1) {
        for (;;) {
            memset(user_input, 0, sizeof(user_input));
            memset(received_message, 0, sizeof(received_message));
            server_address = *((struct sockaddr_in *)result->ai_addr);
            server_addr_len = result->ai_addrlen;

            if (strlen(fgets(user_input, 255, stdin)) == 0) {
                perror("Error!");
            }

            bytes_sent = sendto(client_socket, user_input, sizeof(user_input), 0, (struct sockaddr *)&server_address, server_addr_len);
            recvfrom(client_socket, received_message, sizeof(received_message), 0, (struct sockaddr *)&server_address, &server_addr_len);

            if (strcmp(received_message, "farewell\n") == 0 || strcmp(received_message, "ok\n") == 0) {
                exit(0);
            }

            memset(user_input, 0, sizeof(user_input));
        }
    } else {
        if (connect(client_socket, result->ai_addr, result->ai_addrlen) < 0) {
            perror("Connect Error\n");
        }

        for (;;) {
            memset(user_input, 0, sizeof(user_input));
            memset(received_message, 0, sizeof(received_message));

            if (strlen(fgets(user_input, 255, stdin)) == 0) {
                perror("Error!");
            }

            bytes_sent = send(client_socket, user_input, strlen(user_input), 0);

            if (bytes_sent < 0) {
                perror("\nClient Error: Writing to Server");
            }

            memset(user_input, 0, sizeof(user_input));
            int bytes_received = recv(client_socket, received_message, sizeof(received_message), 0);

            if (bytes_received < 0) {
                perror("\nClient Error: Reading from Server");
            }
            printf("%s", received_message);

            if (strcmp(received_message, "farewell\n") == 0 || strcmp(received_message, "ok\n") == 0) {
                exit(0);
            }
        }
    }
    freeaddrinfo(result);
}