#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define WINSIZE 4
#define TIMEOUTINTERVER 15
#define MaxPacket 5

int main(int argc, char *argv[])
{
   int retval;

   // 명령행 인수가 있으면 IP 주소로 사용
   if (argc > 1) SERVERIP = argv[1];

   // 소켓 생성
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); // 인터넷 주소 체계의 연결형 서비스
                                                   // 성공적으로 생성시 소켓 디스크립터 반환
   if (sock == INVALID_SOCKET) err_quit("socket()");

   // connect()
   struct sockaddr_in serveraddr;
   memset(&serveraddr, 0, sizeof(serveraddr)); // serveraddr 세팅
   serveraddr.sin_family = AF_INET;
   inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr); // 서버 IP를 AF_INET 기준으로 binary 형태로 변환해서 저장
   serveraddr.sin_port = htons(SERVERPORT);
   retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // 서버에 연결 요청, 바로 수락되지 않고
                                                                                // 서버의 대기 큐에서 대기 중이면 블로킹 상태
   if (retval == SOCKET_ERROR) err_quit("connect()");

   // 데이터 통신에 사용할 변수
   char buf[BUFSIZE + 1];
   int len;
    
    int window[4] = {0,1,2,3};

    char packets[MaxPacket][99] = {"packet 0: I am a boy.", "packet 1: You are a girl.", "packet 2: There are many animals in the zoo.",
                        "packet 3: 철수와 영희는 서로 좋아합니다!", "packet 4: 나는 점심을 맛있게 먹었습니다."};
    char send_packets[99];
    char send_packet[9];
    char send_contents[99];
    char checksum[9];
    int packets_sendT[MaxPacket] = {0,0,0,0,0};

    char acks[MaxPacket][10];
    int ack_recvT = 0;

    char recv_ack[99];
    char last_ack[99] = "NULL";

    int time = 0;
    int seq = 0;
    int seqNum = 0;
    int last_seqNum = 0;
    char acked_seqNum[99];
    int is_dupl = 0;
   // 서버와 데이터 통신
    for(; seq < 4; seq++) {
        time++;

        int index = 10;
        while(packets[seq][index] != '\0') {
            send_contents[index-10] = packets[seq][index];
            index++;
        }
        send_contents[index-10] = '\0';
        
        last_seqNum = seqNum;
        seqNum += (int)strlen(send_contents);
        char_array_as_binary(send_contents, seqNum-last_seqNum, checksum);
        strncpy(send_packet, packets[seq], 8);

        send_packets[0] = '\0';
        strcpy(send_packets, packets[seq]);
        strcat(send_packets, "&");
        strcat(send_packets, checksum);
        // printf("send_packets: %s\n", send_packets);

        if(seq == 1) {
            packets_sendT[seq] = time;
            sprintf(acks[seq], "ACK = %d\0", seqNum);
            strncpy(send_packet, packets[seq], 8);
            printf("\"%s\" is transmitted. ", send_packet);
            printf("(%s)\n", send_contents);
            sleep(1);
            continue;
        }
        retval = send(sock, send_packets, (int)strlen(send_packets), 0);
        packets_sendT[seq] = time;
        sprintf(acks[seq], "ACK = %d\0", seqNum);
        strncpy(send_packet, packets[seq], 8);
        printf("\"%s\" is transmitted. ", send_packet);
        printf("(%s)\n", send_contents);

        sleep(1);
    }
    
   while (window[0] < MaxPacket) {
        time++;
        for(int i=0; i<4; i++) {
            if(window[i] == MaxPacket+1) window[i] = 0;
        }

        if(seq >= window[WINSIZE-1]) {
            recv_ack[0] = '\0';
            retval = recv(sock, recv_ack, (int)strlen(acks[window[0]]), MSG_WAITALL);
            recv_ack[retval] = '\0';

            if(strcmp(recv_ack, acks[window[0]]) == 0) {
                printf("(%s) is received.\n", recv_ack);
                strcpy(last_ack, recv_ack);

                for(int i = 0; i < WINSIZE; i++) window[i]++;

                int index = 6;
                while(recv_ack[index] != '\0') {
                    acked_seqNum[index-6] = recv_ack[index];
                    index++;
                }
                acked_seqNum[index-6] = '\0';
            }
            else {
                printf("(%s) is received.\n", recv_ack);
                is_dupl++;
            }
        }
        
        int is_time_over = 0;
        if(packets_sendT[window[0]] != 0 && time-packets_sendT[window[0]] >= TIMEOUTINTERVER) {
            printf("%s is timeout.\n", packets[window[0]]);
            seq = window[0];
            is_time_over = 1;
            seqNum = atoi(acked_seqNum);
        }
        else if(is_dupl >= 3) {
            seq = window[0];
            seqNum = atoi(acked_seqNum);
        }
        
      if(seq <= window[WINSIZE-1] || seq == window[0]) {
            if(seq == MaxPacket) {
                sleep(1);
                continue;
            }
            int index = 10;
            while(packets[seq][index] != '\0') {
                send_contents[index-10] = packets[seq][index];
                index++;
            }
            send_contents[index-10] = '\0';
            last_seqNum = seqNum;
            seqNum += (int)strlen(send_contents);
            char_array_as_binary(send_contents, seqNum-last_seqNum, checksum);
            strncpy(send_packet, packets[seq], 8);

            send_packets[0] = '\0';
            strcpy(send_packets, packets[seq]);
            strcat(send_packets, "&");
            strcat(send_packets, checksum);
            // printf("send_packets: %s\n", send_packets);

            retval = send(sock, send_packets, (int)strlen(send_packets), 0);
            packets_sendT[seq] = time;

            if(is_dupl >= 3 || is_time_over == 1) {
                // seqNum += (int)strlen(send_contents);
                printf("\"%s\" is retransmitted. ", send_packet);
                printf("(%s)\n", send_contents);
                is_dupl = 0;
                is_time_over = 0;
            }
            else {
                // seqNum += (int)strlen(send_contents);
                sprintf(acks[seq], "ACK = %d", seqNum);

                printf("\"%s\" is transmitted. ", send_packet);
                printf("(%s)\n", send_contents);
            }
            seq++;
        }

        sleep(1);
   }

   // 소켓 닫기
   close(sock);
   return 0;
}
