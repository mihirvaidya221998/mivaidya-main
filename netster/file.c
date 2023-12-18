#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>


void file_server(char* iface, long port, int use_udp, FILE* fp) {
    struct addrinfo hints;
    struct addrinfo *result;

    hints.ai_family = AF_INET;
    if (use_udp == 1) {
        hints.ai_socktype = SOCK_DGRAM;
    } else {
        hints.ai_socktype = SOCK_STREAM;
    }
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    char port_str[256];
    int getaddr_result;
    int socket_fd;

    sprintf(port_str, "%ld", port);
    getaddr_result = getaddrinfo(iface, port_str, &hints, &result);

    if (getaddr_result != 0) {
        perror("getaddrinfo error");
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (socket_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_result = bind(socket_fd, result->ai_addr, result->ai_addrlen);

    if (bind_result < 0) {
        perror("Binding error");
        exit(1);
    }

    char temp_buf1[256], temp_buf2[256];
    int n;

    if (use_udp == 1) {
        memset(temp_buf1, 0, 256);
        unsigned int length = sizeof(client_addr);

         while (1) {
            n = recvfrom(socket_fd, (char *)temp_buf1, 256, MSG_WAITALL, (struct sockaddr *)&client_addr, &length);

            if (n < 0) {
                perror("Error receiving data\n");
                fclose(fp);
                close(socket_fd);
                exit(1);
             }

             if (temp_buf1[0] == '\0') {
                 // End of file reached
                 break;
             }

             fwrite(temp_buf1, sizeof(char), n, fp);
             memset(temp_buf1, 0, 256);
         }
         fclose(fp);
    } else {
        socklen_t sockaddr_len = sizeof(server_addr);
        int incoming;
        int i, bytes;
        listen(socket_fd, 10);

        while (!feof(fp)) {
            incoming = accept(socket_fd, (struct sockaddr *) & server_addr, &sockaddr_len);

            if (incoming == -1) {
                continue;
            }
            
            bytes = 0;

            if (fp != NULL) {
                while ((i = recv(incoming, temp_buf2, sizeof(temp_buf2), 0)) > 0) {
                    bytes += 1;
                    fwrite(temp_buf2, 1, i, fp);

                    printf("Bytes received: %d\n", bytes);
                    if (i < 0) {
                        fclose(fp);
                    }
                }
            } else {
                perror("File Error");
            }
            close(incoming);
            exit(0);
        }
    }
    close(socket_fd);
}


void file_client(char* host, long port, int use_udp, FILE* fp) {
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    int n;
    hints.ai_family = AF_INET;

    if (use_udp == 1) {
        hints.ai_socktype = SOCK_DGRAM;
    } else {
        hints.ai_socktype = SOCK_STREAM;
    }
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    char port_str[1024];
    int getaddr_result;
    int socket_fd;

    sprintf(port_str, "%ld", port);
    getaddr_result = getaddrinfo(host, port_str, &hints, &result);

    if (getaddr_result != 0) {
        perror("getaddrinfo error");
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (socket_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    struct sockaddr_in addr;
    char temp_buf1[1024], temp_buf2[1024];
    int elements_read;

    memset(temp_buf1, 0, sizeof(temp_buf1));
    memset(temp_buf2, 0, sizeof(temp_buf2));

    if (use_udp == 1) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = result->ai_addr->sa_family;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (file_size == 0) {
            // Send an empty file message
            strcpy(temp_buf1, "EMPTY_FILE");
            sendto(socket_fd, (const char *)temp_buf1, 256, 0, (const struct sockaddr *)&addr, sizeof(addr));
            return;
        }

        while ((elements_read = fread(temp_buf1, sizeof(char), sizeof(temp_buf1), fp)) > 0) {
            n = sendto(socket_fd, (const char *)temp_buf1, elements_read, 0, (const struct sockaddr *)&addr, sizeof(addr));

            if (n < 0) {
                perror("Sending File Failed");
                exit(1);
            }
            memset(temp_buf1, 0, sizeof(temp_buf1));
        }

        strcpy(temp_buf1, "");
        sendto(socket_fd, (const char *)temp_buf1, sizeof(temp_buf1), 0, (const struct sockaddr *)&addr, sizeof(addr));
    } else {
        if (connect(socket_fd, result->ai_addr, result->ai_addrlen) < 0) {
            perror("Connect Error\n");
        }

        if (fp == NULL) {
            exit(1);
        }

        while ((elements_read = fread(temp_buf1, 1, sizeof(temp_buf1), fp)) > 0) {
            send(socket_fd, temp_buf1, elements_read, 0);
            memset(temp_buf1, 0, sizeof(temp_buf1));
        }
        printf("Data Transfer Complete\n");
        close(socket_fd);
    }
}