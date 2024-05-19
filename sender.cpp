#include "Common.h"

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 9000
#define BUFFER_SIZE 512
#define WINDOW_SIZE 4
#define TIMEOUT_INTERVAL 15
#define MAX_PACKETS 5

void char_array_as_binary(const char* data, int length, char* checksum) {
	unsigned char xor_result = 0;
	for( int i = 0; i < length; ++i) {
		xor_result ^= data[i];
	}
}

int main(int argc, char *argv[]) {
    int retval;

    // 명령행 인수가 있으면 IP 주소로 사용
    
    const char *server_ip = (argc > 1) ? argv[1] : SERVER_ADDRESS;

    // 소켓 생성
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) err_quit("socket()");

    // 서버 주소 설정
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVER_PORT);

    // 서버에 연결
    retval = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 데이터 통신에 사용할 변수들
    char buffer[BUFFER_SIZE + 1];
    int length;

    int sliding_window[WINDOW_SIZE] = {0, 1, 2, 3};

    char packet_data[MAX_PACKETS][99] = {
        "packet 0: I am a boy.",
        "packet 1: You are a girl.",
        "packet 2: There are many animals in the zoo.",
        "packet 3: 철수와 영희는 서로 좋아합니다!",
        "packet 4: 나는 점심을 맛있게 먹었습니다."
    };

    char packets_to_send[99];
    char single_packet[9];
    char packet_contents[99];
    char checksum[9];
    int packet_send_times[MAX_PACKETS] = {0, 0, 0, 0, 0};

    char ack_messages[MAX_PACKETS][10];
    int ack_received_time = 0;

    char received_ack[99];
    char last_received_ack[99] = "NULL";

    int current_time = 0;
    int sequence_number = 0;
    int cumulative_sequence = 0;
    int previous_sequence = 0;
    char ack_sequence_number[99];
    int duplicate_ack_count = 0;

    // 데이터 전송
    for (; sequence_number < WINDOW_SIZE; sequence_number++) {
        current_time++;

        int index = 10;
        while (packet_data[sequence_number][index] != '\0') {
            packet_contents[index - 10] = packet_data[sequence_number][index];
            index++;
        }
        packet_contents[index - 10] = '\0';

        previous_sequence = cumulative_sequence;
        cumulative_sequence += (int)strlen(packet_contents);
        char_array_as_binary(packet_contents, cumulative_sequence - previous_sequence, checksum);
        strncpy(single_packet, packet_data[sequence_number], 8);

        packets_to_send[0] = '\0';
        strcpy(packets_to_send, packet_data[sequence_number]);
        strcat(packets_to_send, "&");
        strcat(packets_to_send, checksum);

        if (sequence_number == 1) {
            packet_send_times[sequence_number] = current_time;
            sprintf(ack_messages[sequence_number], "ACK = %d", cumulative_sequence);
            strncpy(single_packet, packet_data[sequence_number], 8);
            printf("\"%s\" is transmitted. ", single_packet);
            printf("(%s)\n", packet_contents);
            sleep(1);
            continue;
        }
        retval = send(client_socket, packets_to_send, (int)strlen(packets_to_send), 0);
        packet_send_times[sequence_number] = current_time;
        sprintf(ack_messages[sequence_number], "ACK = %d", cumulative_sequence);
        strncpy(single_packet, packet_data[sequence_number], 8);
        printf("\"%s\" is transmitted. ", single_packet);
        printf("(%s)\n", packet_contents);

        sleep(1);
    }

    while (sliding_window[0] < MAX_PACKETS) {
        current_time++;
        for (int i = 0; i < WINDOW_SIZE; i++) {
            if (sliding_window[i] == MAX_PACKETS + 1) sliding_window[i] = 0;
        }

        if (sequence_number >= sliding_window[WINDOW_SIZE - 1]) {
            received_ack[0] = '\0';
            retval = recv(client_socket, received_ack, (int)strlen(ack_messages[sliding_window[0]]), MSG_WAITALL);
            received_ack[retval] = '\0';

            if (strcmp(received_ack, ack_messages[sliding_window[0]]) == 0) {
                printf("(%s) is received.\n", received_ack);
                strcpy(last_received_ack, received_ack);

                for (int i = 0; i < WINDOW_SIZE; i++) sliding_window[i]++;

                int index = 6;
                while (received_ack[index] != '\0') {
                    ack_sequence_number[index - 6] = received_ack[index];
                    index++;
                }
                ack_sequence_number[index - 6] = '\0';
            } else {
                printf("(%s) is received.\n", received_ack);
                duplicate_ack_count++;
            }
        }

        int timeout_flag = 0;
        if (packet_send_times[sliding_window[0]] != 0 && current_time - packet_send_times[sliding_window[0]] >= TIMEOUT_INTERVAL) {
            printf("%s is timeout.\n", packet_data[sliding_window[0]]);
            sequence_number = sliding_window[0];
            timeout_flag = 1;
            cumulative_sequence = atoi(ack_sequence_number);
        } else if (duplicate_ack_count >= 3) {
            sequence_number = sliding_window[0];
            cumulative_sequence = atoi(ack_sequence_number);
        }

        if (sequence_number <= sliding_window[WINDOW_SIZE - 1] || sequence_number == sliding_window[0]) {
            if (sequence_number == MAX_PACKETS) {
                sleep(1);
                continue;
            }
            int index = 10;
            while (packet_data[sequence_number][index] != '\0') {
                packet_contents[index - 10] = packet_data[sequence_number][index];
                index++;
            }
            packet_contents[index - 10] = '\0';
            previous_sequence = cumulative_sequence;
            cumulative_sequence += (int)strlen(packet_contents);
            char_array_as_binary(packet_contents, cumulative_sequence - previous_sequence, checksum);
            strncpy(single_packet, packet_data[sequence_number], 8);

            packets_to_send[0] = '\0';
            strcpy(packets_to_send, packet_data[sequence_number]);
            strcat(packets_to_send, "&");
            strcat(packets_to_send, checksum);

            retval = send(client_socket, packets_to_send, (int)strlen(packets_to_send), 0);
            packet_send_times[sequence_number] = current_time;

            if (duplicate_ack_count >= 3 || timeout_flag == 1) {
                printf("\"%s\" is retransmitted. ", single_packet);
                printf("(%s)\n", packet_contents);
                duplicate_ack_count = 0;
                timeout_flag = 0;
            } else {
                sprintf(ack_messages[sequence_number], "ACK = %d", cumulative_sequence);

                printf("\"%s\" is transmitted. ", single_packet);
                printf("(%s)\n", packet_contents);
            }
            sequence_number++;
        }

        sleep(1);
    }

    // 소켓 닫기
    close(client_socket);
    return 0;
}
