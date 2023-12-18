from typing import BinaryIO
import socket

MAX_PACKET_SIZE = 1024
TIMEOUT = 0.0020
INITIAL_WINDOW_SIZE = 4
CHUNK_SIZE = 1000

def gbn_server(interface: str, port: int, output_file: BinaryIO) -> None:
    print("Server: Initializing")
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_address = socket.getaddrinfo(interface, port)[0][4]
    udp_protocol(server_socket, server_address, output_file.name)
    server_socket.close()

def udp_protocol(server_socket, server_address, file_path):
    server_socket.bind(server_address)
    output_file = open(file_path, "wb")
    expected_sequence = 0
    continue_receiving = True

    def generate_ack(sequence_ack, ack_type):
        return str(sequence_ack).encode() + b"-" + ack_type

    while continue_receiving:
        received_packets_count = 0
        while True:
            received_data, client_address = server_socket.recvfrom(MAX_PACKET_SIZE)
            data_values = received_data
            temp = data_values.split(b"-", 2)
            sequence_number = int(temp[0])
            length = int(temp[1].decode())
            data = temp[2]

            if not data:
                if sequence_number == expected_sequence:
                    # Send multiple acknowledgments for reliability
                    for _ in range(6):
                        ack_info = generate_ack(sequence_number, b"A")
                        server_socket.sendto(ack_info, client_address)
                    continue_receiving = False
                    break
            elif sequence_number == expected_sequence:
                expected_sequence = (sequence_number + 1) % 2
                received_packets_count += 1
                output_file.write(data)
                if received_packets_count == length:
                    ack_info = generate_ack(sequence_number, b"A")
                    server_socket.sendto(ack_info, client_address)
                    break
            else:
                print("Server: Expected", expected_sequence, "but got", sequence_number)
                # Send negative acknowledgment for out-of-sequence packet
                nack_info = generate_ack(expected_sequence, b"N")
                server_socket.sendto(nack_info, client_address)
                break

    print("Server: Data reception complete")
    output_file.close()

def gbn_client(server_host: str, server_port: int, file_ptr: BinaryIO) -> None:
    print("Client: Initializing")
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = socket.getaddrinfo(server_host, server_port)[0][4]
    file_reader = open(file_ptr.name, "rb")
    window_size = INITIAL_WINDOW_SIZE
    buffer = []
    packet_count = 0
    file_data_chunk = file_reader.read(CHUNK_SIZE)
    
    while file_data_chunk:
        buffer.append(file_data_chunk)
        file_data_chunk = file_reader.read(CHUNK_SIZE)
        packet_count += 1
    
    file_reader.close()
    next_expected_packet = 0
    sequence_number = 0

    def generate_packet(seq, window_size, data):
        return str(seq).encode() + b"-" + str(window_size).encode() + b"-" + data

    def extract_ack(data):
        temp = data.split(b"-", 1)
        ack_seq = int(temp[0])
        return ack_seq if ack_seq >= 0 else -1

    while next_expected_packet < packet_count:
        for i in range(next_expected_packet, min((next_expected_packet + window_size), packet_count)):
            data = buffer[i]
            sequence_number = (sequence_number + 1)
            print("Client: Sending packet with sequence number", sequence_number)
            client_socket.sendto(generate_packet(sequence_number, window_size, data), server_address)

        client_socket.settimeout(TIMEOUT)  # Countdown timer set to 20 ms
        # AIMD implementation to implement a congestion control mechanism
        try:
            received_packet, _ = client_socket.recvfrom(MAX_PACKET_SIZE)
            ack_seq = extract_ack(received_packet)
            if ack_seq == sequence_number:
                next_expected_packet = (ack_seq + 1)
                window_size += 1  # Additive increase
                client_socket.settimeout(0)
            elif ack_seq == -1:
                print("Client: Received negative acknowledgment")
                if window_size // 2 > 1:
                    window_size = window_size // 2  # Multiplicative decrease
                sequence_number = min((next_expected_packet + window_size), packet_count)
                client_socket.settimeout(0)
        except socket.timeout:
            print("Client: Timeout occurred")
            for i in range(next_expected_packet, min((next_expected_packet + window_size), packet_count)):
                data = buffer[i]
                print("Client: Resending packet with sequence number", i + 1)
                client_socket.sendto(generate_packet(i + 1, window_size, data), server_address)
            sequence_number = min((next_expected_packet + window_size), packet_count)

    print("Client: Data transmission complete")
    end_msg = generate_packet(sequence_number, 1, b'')
    for _ in range(5):
        client_socket.sendto(end_msg, server_address)

    client_socket.close()