#include <iostream>
#include <thread> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>
#include <string.h>
#include <vector>
#include <chrono>
#include <boost/lockfree/queue.hpp>


using namespace std;


#define UdpPort 3456


vector<int> _myvac;
mutex _mtxMyvac;

boost::lockfree::queue<char *> q{500};
//std::atomic<int> sum{0};

void Enqueu(char *buffer)
{
    q.push(buffer);
}

char*  Dequeu()
{
    char *buffer;
    q.pop(buffer);
    return buffer;
}


void WriteMassageToTcp ();
int Setup_tcpSocket (unsigned PortNo){
    int tcpfd, opt =1;
    struct sockaddr_in ServAddr;

    if ((tcpfd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
        perror ("TCP Socket Error");
        exit(1);
    }

    if((setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) != 0) {
        perror("tcp Socket Option Error");
        exit(1);
    }

    bzero(&ServAddr, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons (PortNo);
    ServAddr.sin_addr.s_addr = INADDR_ANY;

    if ((bind (tcpfd,(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != 0){
        perror ("tcp Binding Error");
        exit(1);
    }

    if ((listen (tcpfd, 10)) != 0){
        perror("Listening Error");
        exit(1);
    }
    return tcpfd;
}

int Setup_udpSocket (unsigned PortNo){

    int udpfd, opt = 1;
    struct sockaddr_in ServAddr;
    if ((udpfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
        perror ("udp Socket Error");
        exit(1);
    }
    if((setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) != 0) {
        perror("udp Socket Option Error");
        exit(1);
    }
    bzero(&ServAddr, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons (PortNo);
    ServAddr.sin_addr.s_addr = INADDR_ANY;

    if ((bind (udpfd, (struct sockaddr *)&ServAddr, sizeof(ServAddr))) != 0){
        perror ("udp Binding Error");
        exit(1);
    }
    cout << "Udp socket " << udpfd << " bind on port: " << PortNo << endl;
    return udpfd;
}

void recvMsgFromUdp (unsigned udpfd){
    long long start_time;

    int n, counter = 0;
    char buffer[512];
    // sockaddr_in ClientAddr;
    // bzero(&ClientAddr, sizeof(ClientAddr));
    // socklen_t len = sizeof(ClientAddr); 
    while(true){
        // n = recvfrom (udpfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&ClientAddr, &len);
        n = read (udpfd, buffer, sizeof(buffer));
        if (n<0){
            perror("no msg from udp:");
        }
        // cout << "buffer: " << buffer << endl;

        long long end_time = std::chrono::duration_cast <std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        Enqueu(buffer);
        
        auto thwrite = thread(WriteMassageToTcp);
        thwrite.join();
        
        sscanf (buffer, "%lld %s", &start_time, buffer);
        cout << "Msg: " << ++counter << "  Latency : "<< end_time-start_time << endl;
        bzero(buffer, 256);
    }
}

void WriteMassageToTcp (){
    int n;       
    _mtxMyvac.lock();
    char *buffer;
    buffer = Dequeu();
    for(int i=0; i < _myvac.size(); i++) {
        n = write(_myvac[i], buffer, strlen(buffer));        
        if(n<0){
            perror("Error Writing udp msg to tcp ");
            cout << "\nerror on socket:"<<_myvac[i]<<endl;
            _myvac.erase(_myvac.begin()+i-1);
        }
        // cout << "Massage written on TcpSocket: " << _myvac[i]<<endl;
    }
    _mtxMyvac.unlock();
}

void acceptTcpClients (int PortNo){
    
    struct sockaddr_in ClientAddr;
    socklen_t len = sizeof(ClientAddr);
    int tcpClientSockfd;
    int tcpfd = Setup_tcpSocket(PortNo);

    while ( true ){        
        memset( &ClientAddr, 0, sizeof(ClientAddr) );
        tcpClientSockfd = accept(tcpfd,(struct sockaddr *) &ClientAddr, &len);
        cout<<"Client accepted on socket: "<< tcpClientSockfd <<endl;
        if (tcpClientSockfd <= 0){
            perror("Error on Accept");
            exit(0);
        }
        else{
        _mtxMyvac.lock();
        _myvac.push_back(tcpClientSockfd);
        _mtxMyvac.unlock();
        }
    }
}

int main (int argc, char *argv[]){

    bool check = true;
    int tcpClientSockfd, n;
    char buff[512];
    
    if (argc < 2){
        cout << "No Port Given";
        exit (1);
    }

    int PortNo = atoi(argv[1]);
    if (PortNo <= 0){
        perror("Port number:");
        exit(1);
    }
    
    auto thtcp = thread(acceptTcpClients, PortNo);
    thtcp.detach();

    int udpfd = Setup_udpSocket(UdpPort);
    auto thudp = thread(recvMsgFromUdp, udpfd);
    thudp.join();


}