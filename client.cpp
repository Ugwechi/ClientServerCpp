#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <exception>
#include <fstream>


void peer(int sockfd, std::string path); //this is the thread service
void server_signal_handler(int sig)
{
        std::cout << "received signal" << std::endl;
        exit(0);
}

int main(int argc, char** argv)
{
if(argc !=4)
{
    std::cerr << "ERROR: Invalid number of inputs.\nUsage : ./client port serveradd file" << '\n';
    exit(1);
}


    std::string address = "";
    std::string filename = "";
    int portno = 0;
    try{
      portno = std::stoi(std::string(argv[1]));
      address = std::string(argv[2]);
      filename = std::string(argv[3]);
    }catch(std::exception &e){
      std::cout << e.what() << '\n';
    }
    
    if(portno <= 1023){
        //error message
        std::cerr << "ERROR: You can use only port numbers greater than 1023 " << '\n';
        exit(1);
    }

  // Register signal and signal handler
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = server_signal_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, NULL);
   sigaction(SIGTERM, &sigIntHandler, NULL);

  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serverAddr;  
if(inet_addr(address.c_str()) < 0)
{
struct hostent *he;
struct in_addr **addr_list;
//resolve the hostname, its not an ip address
if ( (he = gethostbyname( address.c_str() ) ) == NULL)
{
//gethostbyname failed
std::cerr << "Failed to resolve hostname\n";
return 1;
}
//Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
addr_list = (struct in_addr **) he->h_addr_list;

for(int i = 0; addr_list[i] != NULL; i++)
{
serverAddr.sin_addr = *addr_list[i];
break;
}

}
else
{
  serverAddr.sin_addr.s_addr = inet_addr(address.c_str());
}
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(portno);     // short, network byte order
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0){
    std::cerr << "ERROR: setsockopt send timeout" << '\n';
    return 1;
  }
  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    std::cerr << "unable to connect";
    return 2;
  }

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    std::cerr << "getsockname failed";
    return 3;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;


  // send/receive data to/from connection
  std::string input;
  std::stringstream ss;
  std::streampos size;
  char * memblock;

  std::ifstream file (filename, std::ios::in|std::ios::binary|std::ios::ate);
  if (file.is_open())
  {
    size = file.tellg();
    memblock = new char [1024];
    file.seekg (0, std::ios::beg);
    while (file.read (memblock, 1024)) {
        if (send(sockfd, (void*)memblock, 1024, 0) == -1) {
          std::cerr << "ERROR : send failed";
          return 4;
        }
        memset(memblock,0,1024);
      }    
    file.close();
  std::cout << "the entire file content has been sent";
    delete[] memblock;
  }
  

  close(sockfd);

  return 0;
}