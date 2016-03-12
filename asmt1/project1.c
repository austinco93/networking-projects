#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* Austin Corotan
 * Project 1
 * Date: 01/21/2016
 * Description: TCP Client program that can communicate with many servers in sequence.
 * Address: IPv4
 * Port: 16-bit
 */

/* Socket struct with IP address represented as a string and the port as an int */
struct sockstr{
  char *addr;
  int port;
};

/* Get Socket
 * This function returns a socket struct with the string IP address and the 
 * integer port number properly divided. This format is used in relation to 
 * the connect_to function, which accepts a string IP and short port.
 */
struct sockstr getSock(char* buffer){
  struct sockstr conSock;
  
  conSock.addr = strtok (buffer," ");
  conSock.port = atoi(strtok (NULL, " "));

  return conSock;
}

/* Connect to
 * This function establishes a TCP connection to a remote server while handling 
 * all of the errors.
 */
int connect_to (const char* address,
		unsigned short port)
{
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if(!inet_aton(address, &addr.sin_addr)) {
    fprintf(stderr, "Could not convert \"%s\" to an address\n", address);
    exit(1);
  }


  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) {
    fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }

  if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
    fprintf(stderr, "Failed to connect to %s: %d because (%d) \"%s\"\n",
	    address, port, errno, strerror(errno));
    close(fd);
    exit(1);
  }
  return fd;
}

/* Client send
 * This function sends a  message to the remote server while handling possible 
 * errors in the process.
 */
void clientSend (int socket, char* message){
  if( send(socket , message, strlen(message) , 0) < 0){
    fprintf(stderr,"Error in sending message because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }
  else{
    printf("Client: Sent %s\n", message);
  }
}

/* Client receive
 * This function receives a message from the remote server while handling possible
 * errors in the process.
 */
void clientReceive (int socket, char* buffer, int bufsize){
  int num;
  num = recv(socket, buffer, bufsize,0);
  if(num <= 0){
    fprintf(stderr,"Error in receiving message because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }
  else{
    buffer[num] = '\0';
    printf("Client: Received %s\n",buffer);
  }
}

int main()
{
  char buffer[80];
  char *userid = "corotaa\n";
  char *secret = "enhydritic\n";
  struct sockstr connectSock;

  int sockfd = connect_to("140.160.136.211", 12367);

  while(1) {
    clientReceive(sockfd, buffer, sizeof(buffer));

    if(strcmp(buffer, "userid: ") == 0){
      clientSend(sockfd, userid);
    }
    else if(strcmp(buffer, "secret: ") == 0){
      clientSend(sockfd,secret);
    }
    else if(strcmp(buffer, "good job!\n") == 0){
      printf("Success!\n");
      break;
    }
    else{
      connectSock = getSock(buffer);
      close(sockfd);   
      printf("Connecting to %s %d...\n", connectSock.addr, connectSock.port);
      sockfd = connect_to(connectSock.addr, connectSock.port);
    }
  }
  close(sockfd);
  return 0;
}
