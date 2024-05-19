#include "Common.h"

#define SERVER_PORT 9000
#define BUFFER_SIZE 512
#define WINDOW_SIZE 4
#define MAX_PACKETS 5

void char_array_as_binary(const char* data, int length, char* checksum) {
	unsigned char xor_result = 0;
	for( int i = 0; i < length; ++i) {
		xor_result ^= data[i];
	}
}

int main(int argc, char *argv[]) {
    int retval;

    // 소켓 생성
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) err_quit("socket()");

    // 소켓 주소 설정
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);
    retval = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // 연결 대기
    retval = listen(server_socket, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // 데이터 통신에 사용할 변수들
    SOCKET client_socket;
    struct sockaddr_in client_address;
    socklen_t address_length;
    char buffer[BUFFER_SIZE + 1];

    while (1) {
        // 클라이언트 연결 수락
        address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &address_length);
        if (client_socket == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // 클라이언트 정보 출력
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
	printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",client_ip, ntohs(client_address.sin_port));
        int window[WINDOW_SIZE] = {0, 1, 2, 3};
        const char* packets[MAX_PACKETS] = {"packet 0", "packet 1", "packet 2", "packet 3", "packet 4"};
        int dropped_packets[MAX_PACKETS] = {0, 0, 0, 0, 0};

        const char* ack_messages[8] = {"ACK 0", "ACK 1", "ACK 2", "ACK 3", "ACK 4", "ACK 5", "ACK 6", "ACK 7"};
        char send_ack[10];
        char last_ack[10];

        char received_packet[999];
        char received_contents[99];
        char received_checksum[9];
        char computed_checksum[9];
        char current_packet[9];

        int current_time = 0;
        int sequence_index = 0;
        int cumulative_seq = 0;
        int previous_seq = 0;
        int is_retransmission = 0;

        // 클라이언트와 데이터 통신
        while (window[0] < MAX_PACKETS) {
            current_time++;
            for (int i = 0; i < WINDOW_SIZE; i++) {
                if (window[i] == MAX_PACKETS + 1) window[i] = 0;
            }

            retval = recv(client_socket, received_packet, sizeof(received_packet), 0);
            received_packet[retval] = '\0';
            strncpy(current_packet, received_packet, 8);
            current_packet[8] = '\0';

            int index = 10;
            while (received_packet[index] != '&') {
                received_contents[index - 10] = received_packet[index];
                index++;
            }
            received_contents[index - 10] = '\0';

            received_checksum[0] = '\0';
            while (received_packet[index] != '\0') {
                received_checksum[index - 11 - (int)strlen(received_contents)] = received_packet[index];
                index++;
            }
            received_checksum[index - 11 - (int)strlen(received_contents)] = '\0';

            printf("%s is received ", current_packet);

            char_array_as_binary(received_contents, (int)strlen(received_contents), computed_checksum);
            if (strcmp(computed_checksum, received_checksum) == 0) printf("and there is no error. ");
            else printf("and there is error. ");

            printf("(%s) ", received_contents);

            if (strcmp(current_packet, packets[window[0]]) == 0) {
                for (int i = 0; i < WINDOW_SIZE; i++) window[i]++;
                cumulative_seq += (int)strlen(received_contents);

                sprintf(send_ack, "ACK = %d", cumulative_seq);
                retval = send(client_socket, send_ack, (int)strlen(send_ack), 0);
                printf("(%s) is transmitted.\n", send_ack);
                strcpy(last_ack, send_ack);

                sequence_index++;
            } else {
                retval = send(client_socket, last_ack, (int)strlen(last_ack), 0);
                printf("(%s) is retransmitted.\n", last_ack);
            }

            sleep(1);
        }

        // 클라이언트 소켓 닫기
        close(client_socket);
        printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_address.sin_port));
    }

    // 서버 소켓 닫기
    close(server_socket);
    return 0;
}
