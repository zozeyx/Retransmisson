#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define WINSIZE 4
#define MaxPacket 5

int main(int argc, char *argv[])
{
   int retval;

   // 소켓 생성
   SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0); // 인터넷 주소 체계의 연결형 서비스
                                                          // 성공적으로 생성시 소켓 디스크립터 반환
   if (listen_sock == INVALID_SOCKET) err_quit("socket()");

   // bind()
   struct sockaddr_in serveraddr;
   memset(&serveraddr, 0, sizeof(serveraddr)); // serveraddr 세팅
   serveraddr.sin_family = AF_INET;
   serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // IP 주소
   serveraddr.sin_port = htons(SERVERPORT); // 포트 번호
   retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // 소켓에 IP와 포트 할당
   if (retval == SOCKET_ERROR) err_quit("bind()");

   // listen()
   retval = listen(listen_sock, SOMAXCONN); // 연결 요청 대기 상태, SOMAXCONN만큼의 연결 요청 대기 큐
   if (retval == SOCKET_ERROR) err_quit("listen()");

   // 데이터 통신에 사용할 변수
   SOCKET client_sock;
   struct sockaddr_in clientaddr;
   socklen_t addrlen;
   char buf[BUFSIZE + 1];

   while (1) {
      // accept()
      addrlen = sizeof(clientaddr);
      client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen); // 대기중인 클라이언트의 연결 요청을 수락
      if (client_sock == INVALID_SOCKET) {
         err_display("accept()");
         break;
      }

      // 접속한 클라이언트 정보 출력
      char addr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr)); // 클라이언트 IP를 AF_INET를 기준으로 binary에서
                                                                      // 알아볼 수 있게 변환해서 저장
      printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
         addr, ntohs(clientaddr.sin_port));
        
        int window[WINSIZE] = {0,1,2,3};

        // char* packets[MaxPacket] = {"packet 0: I am a boy.", "packet 1: You are a girl.", "packet 2: There are many animals in the zoo.",
        //                     "packet 3: 철수와 영희는 서로 좋아합니다!", "packet 4: 나는 점심을 맛있게 먹었습니다."};
        
        char* packets[MaxPacket] = {"packet 0", "packet 1", "packet 2", "packet 3", "packet 4"};
        
        int is_dropped[MaxPacket] = {0,0,0,0,0};

        char* acks[8] = {"ACK 0", "ACK 1", "ACK 2", "ACK 3", "ACK 4", "ACK 5", "ACK 6", "ACK 7"};
        char send_ack[10];
        char last_ack[10];

        char recv_packet[999];
        char recv_contents[99];
        char recv_checksum[9];
        char checksum[9];
        char packet[9];

        int time = 0;
        int seq = 0;
        int seqNum = 0;
        int last_seqNum = 0;
        int is_re = 0;
      // 클라이언트와 데이터 통신
      while (window[0] < MaxPacket) {
            time++;
            for(int i=0; i<4; i++) {
                if(window[i] == MaxPacket+1) window[i] = 0;
            }

         retval = recv(client_sock, recv_packet, 999, 0); //(int)strlen(packets[seq])
            recv_packet[retval] = '\0';
            strncpy(packet, recv_packet, 8);
            packet[8] = '\0';

            int index = 10;
            while(recv_packet[index] != '&') {
                recv_contents[index-10] = recv_packet[index];
                index++;
            }
            recv_contents[index-10] = '\0';
            
            recv_checksum[0] = '\0';
            while(recv_packet[index] != '\0') {
                recv_checksum[index-11-(int)strlen(recv_contents)] = recv_packet[index];
                index++;
            }
            recv_checksum[index-11-(int)strlen(recv_contents)] = '\0';

            printf("%s is received ", packet);

            char_array_as_binary(recv_contents, (int)strlen(recv_contents), checksum);
            // printf("\nchecksum: %s, recv_checksum: %s\n",checksum, recv_checksum);
            if(strcmp(checksum, recv_checksum) == 0) printf("and there is no error. ");
            else printf("and there is error. ");

            printf("(%s) ", recv_contents);
            
            if(strcmp(packet, packets[window[0]]) == 0) {
                for(int i = 0; i < WINSIZE; i++) window[i]++;
                seqNum += (int)strlen(recv_contents);

                sprintf(send_ack, "ACK = %d", seqNum);
                retval = send(client_sock, send_ack, (int)strlen(send_ack), 0);
                printf("(%s) is transmitted.\n", send_ack);
                strcpy(last_ack, send_ack);

                seq++;
            }
            else {
                retval = send(client_sock, last_ack, (int)strlen(last_ack), 0);
                printf("(%s) is retransmitted.\n", last_ack);
            }

            sleep(1);
      }

      // 소켓 닫기
      close(client_sock);
      printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
         addr, ntohs(clientaddr.sin_port));
   }

   // 소켓 닫기
   close(listen_sock);
   return 0;
}
