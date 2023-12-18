#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define TIMEOUT_MS 30
#define MAX_BUFFER_SIZE 1024

// Structure representing a packet of data
typedef struct data_packet {
  char content[MAX_BUFFER_SIZE];
} DataPacket;

// Structure representing a communication frame
typedef struct communication_frame {
  int frame_type;           // 0: ACK, 1: Data, 2: End of File
  int sequence_number;      // Sequence number of the frame
  int acknowledgment;       // Expected acknowledgment number
  int packet_size;          // Size of the data packet
  DataPacket packet;        // Data packet
} CommFrame;

void handle_error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

// Server implementation of Stop-and-Wait protocol
void stopandwait_server(char *iface, long port, FILE *fp) {
  int socket_fd;
  int sequence_id = 0;
  struct sockaddr_in server_address;
  struct addrinfo communication_hints, *communication_result;

  CommFrame received_frame, send_frame;

  memset(&communication_hints, 0, sizeof(struct addrinfo));
  memset(&server_address, 0, sizeof(server_address));

  // Communication hints for address resolution
  communication_hints.ai_family = AF_UNSPEC;
  communication_hints.ai_socktype = SOCK_DGRAM;
  communication_hints.ai_flags = AI_PASSIVE;
  communication_hints.ai_protocol = 0;

  char str_port[10];
  sprintf(str_port, "%ld", port);

  // Get address info for socket creation and binding
  if (getaddrinfo(NULL, str_port, &communication_hints, &communication_result) != 0) {
    handle_error("Get address info failed");
  }

  // Create socket
  socket_fd = socket(communication_result->ai_family, communication_result->ai_socktype, communication_result->ai_protocol);
  if (socket_fd < 0) {
    handle_error("Socket creation failed");
  }

  // Bind socket to the specified address and port
  if (bind(socket_fd, communication_result->ai_addr, communication_result->ai_addrlen) < 0) {
    handle_error("Binding failed");
  }

  socklen_t addr_size = sizeof(server_address);

  for (;;) {
    // Receive frame from the client
    int recv_size = recvfrom(socket_fd, &received_frame, sizeof(CommFrame), 0, (struct sockaddr *)&server_address, &addr_size);
    if (recv_size <= 0) {
      break;
    }

    // Process received frame
    if (recv_size > 0 && received_frame.frame_type == 1 && received_frame.sequence_number == sequence_id) {
      printf("Frame Received: %s\n", received_frame.packet.content);

      // Write data to the file
      fwrite(received_frame.packet.content, sizeof(unsigned char), received_frame.packet_size, fp);

      // Send acknowledgment back to the client
      send_frame.sequence_number = 0;
      send_frame.frame_type = 0;
      send_frame.acknowledgment = received_frame.sequence_number + 1;
      sendto(socket_fd, &send_frame, sizeof(send_frame), 0, (struct sockaddr *)&server_address, addr_size);
      printf("Acknowledgement Sent\n");
    }

    // Check for end of file frame
    if (received_frame.frame_type == 2) {
      printf("File done here as frame type received is 2\n");
      close(socket_fd);
      fflush(fp);
    }

    sequence_id++;
  }
}

// Client implementation of Stop-and-Wait protocol
void stopandwait_client(char *host, long port, FILE *fp) {
  struct timeval timeout = {0, TIMEOUT_MS * 1000};
  struct hostent *host_info;

  host_info = gethostbyname(host);
  if (host_info == NULL) {
    handle_error("Get host by name failed");
  }

  char *ip_address = inet_ntoa(*(struct in_addr *)host_info->h_addr_list[0]);

  struct addrinfo communication_hints, *communication_result;
  int socket_fd, data_read;
  char data_buffer[MAX_BUFFER_SIZE];

  CommFrame send_frame, received_frame;
  int acknowledgment_received = 1;
  int sequence_id = 0;

  printf("IP Address: %s\n", ip_address);
  memset(&communication_hints, 0, sizeof(struct addrinfo));

  // Communication hints for address resolution
  communication_hints.ai_family = AF_UNSPEC;
  communication_hints.ai_socktype = SOCK_DGRAM;

  char str_port[10];
  sprintf(str_port, "%ld", port);

  // Get address info for socket creation and connection
  if (getaddrinfo(host, str_port, &communication_hints, &communication_result) != 0) {
    handle_error("Get address info failed");
  }

  // Create socket
  socket_fd = socket(communication_result->ai_family, communication_result->ai_socktype, communication_result->ai_protocol);
  if (socket_fd < 0) {
    handle_error("Socket creation failed");
  }

  // Connect socket to the server
  if (connect(socket_fd, communication_result->ai_addr, communication_result->ai_addrlen) == -1) {
    handle_error("Socket connect failed");
  }

  for (;;) {
    // Resend frame if acknowledgment not received
    if (acknowledgment_received == 0) {
      printf("Acknowledgement not received. Sending again\n");
      send_frame.sequence_number = sequence_id;
      send_frame.frame_type = 1;
      send_frame.acknowledgment = 0;
      send_frame.packet_size = data_read;
      memcpy(send_frame.packet.content, data_buffer, data_read);
      sendto(socket_fd, &send_frame, sizeof(CommFrame), 0, communication_result->ai_addr, communication_result->ai_addrlen);
    }

    // Read data from the file
    data_read = fread(data_buffer, 1, sizeof(data_buffer), fp);
    if (data_read <= 0) {
      break;
    }

    // Send data frame to the server
    if (acknowledgment_received == 1) {
      send_frame.sequence_number = sequence_id;
      send_frame.frame_type = 1;
      send_frame.acknowledgment = 0;
      send_frame.packet_size = data_read;
      printf("Data from file\n%s", data_buffer);
      memcpy(send_frame.packet.content, data_buffer, data_read);
      sendto(socket_fd, &send_frame, sizeof(CommFrame), 0, communication_result->ai_addr, communication_result->ai_addrlen);
      printf("Frame Sent\n");
    }

    // Set timeout for receiving acknowledgment
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
      perror("setsockopt failed\n");
    }

    // Receive acknowledgment from the server
    int recv_size = recvfrom(socket_fd, &received_frame, sizeof(received_frame), 0, communication_result->ai_addr, &communication_result->ai_addrlen);

    // Process acknowledgment
    if (recv_size > 0 && received_frame.sequence_number == 0 && received_frame.acknowledgment == sequence_id + 1) {
      printf("\nAcknowledgement Received\n");
      acknowledgment_received = 1;
      sequence_id++;
    } else {
      acknowledgment_received = 0;
    }
  }

  // Send end of file frame to the server
  send_frame.sequence_number = sequence_id;
  send_frame.frame_type = 2;
  send_frame.acknowledgment = 0;
  memset(data_buffer, 0, sizeof(data_buffer));
  sendto(socket_fd, &send_frame, sizeof(CommFrame), 0, communication_result->ai_addr, communication_result->ai_addrlen);
  printf("\nFile done.\n");
  close(socket_fd);
}