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
if(argc !=3)
{
    std::cerr << "ERROR: Invalid number of inputs.\nUsage : ./server port path" << '\n';
    exit(1);
}


    std::string path = "";
    
    int portno = 0;
    try{
      portno = std::stoi(std::string(argv[1]));
      path = std::string(argv[2]);
    }catch(std::exception &e){
      std::cout << e.what() << '\n';
    }
    
    if(portno <= 1023){
        //this displays error message
        std::cerr << "ERROR: You can use only port numbers greater than 1023 " << '\n';
        exit(1);
    }

  // this allows to register signal and signal handler
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = server_signal_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, NULL);
   sigaction(SIGTERM, &sigIntHandler, NULL);

  // this creates a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // this allows others to reuse the address and includes the time delay of 10 secs as required
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    std::cerr << "ERROR: setsockopt" << '\n';
    return 1;
  }
    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0){
    std::cerr << "ERROR: socket receipt timeout" << '\n';
    return 1;
  }
  // this allows to bind an address to the socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(portno);     //  this allows to convert the port portno value to network byte order
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    std::cerr << "ERROR: bind error" << '\n';
    return 2;
  }
  if (listen(sockfd, 1) == -1) {
    std::cerr << "ERROR: unable to listen on port " << '\n';
    return 3;
  }
int connectionid=0;
  while(true){
  // set socket to listen status
    std::stringstream out;
    out << path << "/" << connectionid;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    // this alloes to accept a new connection
    int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
    // this allows to check for errors
    if (clientSockfd == -1) {
      //std::cerr << "ERROR: accept" << '\n';
      continue;
    }

  connectionid++;
  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Accept a connection from: " << ipstr << ":" << ntohs(clientAddr.sin_port) << std::endl;
    //start thread to handle connection
    std::thread threadObj(peer, clientSockfd, out.str());
    threadObj.detach();
}
  return 0;
}

void peer(int clientSockfd, std::string path){

  // read/write data from/into the connection
  bool isEnd = false;
  char buf[1024] = {0};
  std::ofstream outfile;
  //open the output file
  try{
  outfile.open(path, std::ios::app | std::ios::out | std::ios::binary); // append instead of overwrite
  }catch(std::exception &e){
    std::cerr << "ERROR: unable to listen on port " << e.what() << '\n';
    return;
  }
 if (outfile.is_open())
  {
  while (!isEnd) {
    memset(buf, 0, 1024);
    //read 1kb at a time
    int read = recv(clientSockfd, buf, 1024, 0);
    if ( read == -1) {
      std::cout << "=== End of File read last input == -1 ===\n";
      break;
    }
    else
    {
      if(read==0)
      break;
      for(int x=0;x<read;x++)
      {
        outfile.put(buf[x]);
      }
        
    }
  }
std::cout << "=== End of File read ===\n";
outfile.close();
  }
  else
  {
    std::cerr << "ERROR : Could not open file ===\n";
  }
close(clientSockfd);
}