#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Invalid arguments - %s <host> <port>\n", argv[0]);
    return -1;
  }

  char* host = argv[1]; // Keep this variable name as it is

  // Initialize hints and results structures
  struct addrinfo hints, *results, *rp;
  memset(&hints, 0, sizeof(struct addrinfo));
  // This will allow both IPv4 and IPv6
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; // TCP SOCKET
  hints.ai_protocol = IPPROTO_TCP; // TCP protocol

  int status = getaddrinfo(host, argv[2], &hints, &results);
  if (status != 0) {
    fprintf(stderr, "There is a getaddrinfo error: %s\n", gai_strerror(status));
    return -1;
  }

  // Print IPv6 addresses first
  for (rp = results; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET6) { // IPv6
      char ipv6_address_str[INET6_ADDRSTRLEN];
      struct sockaddr_in6* ipv6_addr = (struct sockaddr_in6*)rp->ai_addr;
      if (inet_ntop(AF_INET6, &(ipv6_addr->sin6_addr), ipv6_address_str, sizeof(ipv6_address_str)) != NULL) {
        printf("IPv6 %s\n", ipv6_address_str);
      }
    }
  }

  // Print IPv4 addresses
  for (rp = results; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET) { // IPv4
      char ipv4_address_str[INET_ADDRSTRLEN];
      struct sockaddr_in* ipv4_addr = (struct sockaddr_in*)rp->ai_addr;
      if (inet_ntop(AF_INET, &(ipv4_addr->sin_addr), ipv4_address_str, sizeof(ipv4_address_str)) != NULL) {
        printf("IPv4 %s\n", ipv4_address_str);
      }
    }
  }

  freeaddrinfo(results);
  return 0;
}