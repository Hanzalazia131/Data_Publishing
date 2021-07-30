#include <iostream>
#include <unistd.h>
#include <string>
#include <string.h>
#include <chrono>
#include <thread>
#include <time.h>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define PortNo 3456


class udpPublisher{

private:
    int sockfd;

    void createsocket () {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if ( sockfd < 0 ) {
            perror("socket creation failed");
            exit(EXIT_FAILURE); 
        }
    }
public:

    udpPublisher () {
        createsocket();
    }

    ~udpPublisher() {
        if(sockfd<0) close(this->sockfd);
    }

    void PublishMassage() {

        char buffer[512];
        struct sockaddr_in servaddr;
        socklen_t len = sizeof(servaddr);
        
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PortNo);
      
        servaddr.sin_addr.s_addr = INADDR_ANY;

        int n;
        
        while (true) {
            
            long long start_time = std::chrono::duration_cast <std::chrono::
                    microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            int len = sprintf(buffer, "%lld : The quick brown fox jumps over the lazy dog", start_time );

            std::cout << buffer<<std::endl;

            n = sendto(sockfd, (const char *)buffer, len, MSG_CONFIRM,(struct sockaddr *)&servaddr, sizeof(servaddr));

            if (n<0) {
                perror("Error on udp sendto");
                exit(1);
            }

            std::cout << "Massage sent on:" << start_time << std::endl;
            bzero(buffer, 512);
            std::this_thread::sleep_for(std::chrono::seconds(1));            
        }
    }
};

int main(int argc, char *argv[]) {

    udpPublisher publisher;

    publisher.PublishMassage();
}
