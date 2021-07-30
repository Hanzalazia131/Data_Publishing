#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <mutex>
#include <chrono>

class TcpClient{

private:    
    int sockfd;
    int Port_num;

    void CreateSocket () {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0){
            perror("Error Opening Socket:");
            exit(1);
        }
    }

    void ConnectSocket () {
        struct sockaddr_in servAddr;
        bzero((char *)&servAddr, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(Port_num);
        servAddr.sin_addr.s_addr = INADDR_ANY;

        if ((connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0){
            perror("Error on Connect in client");
            exit(1);
        }

    }

public:

    explicit TcpClient(int portNum) : Port_num(portNum) {
        CreateSocket();
        ConnectSocket();
    }

    ~TcpClient() {
        if (sockfd<0) close(this->sockfd);
    }

    void RunReceiver() {
        int n;
        long long start_time;
        char buffer[512];
        unsigned counter = 0;
        while( true ) {
            bzero(buffer,512);
            n = read(sockfd, buffer, sizeof(buffer));
            if (n <= 0) {
                perror("ERROR reading from socket");
                break;
            }

            long long end_time = std::chrono::duration_cast <std::chrono::
                    microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            
            sscanf (buffer, "%lld %s", &start_time, buffer);
            
            std::cout << "Msg: " << ++counter << "  Latency : "<< end_time-start_time << std::endl;
        }
        std::cout << "Closing socket: " << sockfd << std::endl;
        close (sockfd); 
    }
};



int main(int argc, char *argv[]) {
    

    int portno;    
    if (argc < 2) {
       fprintf(stderr,"usage %s port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[1]);

    TcpClient client(portno);

    client.RunReceiver();

}
