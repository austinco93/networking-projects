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
 * Project 2
 * Date: 02/19/2016
 * Description: TCP Chat Client Program
 */

/* my_context struct
 */
typedef 
struct mycontext_st{
    char* buf;
    size_t len;
    size_t size;
}mycontext_t;

/* ctx_init
 * initializes mycontext_t
 */
void ctx_init(mycontext_t *ctx){
  //initialize length and size
  ctx->len = 0;
  ctx->size = 80;
  ctx->buf = (char *)malloc(sizeof(char)*ctx->size);
}

/*printbuf
 * this function prints the buffer
 */
void printbuf(mycontext_t *ctx, int len){
 for (int i = 0; i < len; ++i){
    if (ctx->buf[i] == '\0') {
      printf(" ");
    }
    else{
      printf("%c",ctx->buf[i]);
    }
  }
  printf("\n");
}

/* ctx_double_capacity
 * this function doubles the buffer capacity
 */
void ctx_double_capacity(mycontext_t *ctx){
  if (ctx->len >= ctx->size) {
    ctx->size *= 2;
    ctx->buf = (char *)realloc(ctx->buf, sizeof(char) * ctx->size);
  }
}

/* Connect to
 * This function establishes a TCP connection to a remote server while handling 
 * all of the errors.
 */
int connect_to (const char* address,unsigned short port){
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if(!inet_aton(address, &addr.sin_addr)){
    fprintf(stderr, "Could not convert \"%s\" to an address\n", address);
    exit(1);
  }

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) {
    fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }

  if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))){
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
void clientSend (int socket, char* message, int messageSize){
  int num = write(socket , message, messageSize);
  if(num < 0){
    fprintf(stderr,"Error in sending message because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }
}

/* Client receive
 * This function receives a message from the remote server while handling possible
 * errors in the process.
 */
int clientReceive (int socket, char* buffer, int bufsize){
  int num;
  num = read(socket, buffer, bufsize);
  if(num <= 0){
    fprintf(stderr,"Error in receiving message because (%d) \"%s\"\n",
	    errno, strerror(errno));
    exit(1);
  }
  return num;
}

/* do_stdin 
 * This function will handle user sending
 * format: @user message
 */
void do_stdin (int fd, mycontext_t *ctx){
    char msg[1024];
    char name[1024];
    char message[1024];
    
    fgets (msg, 1024, stdin);
    
    if(msg[0] != '@'){
        int sn = snprintf(ctx->buf, 1024, "%s%c%s%c","",'\0',msg, '\0');
        clientSend(fd, ctx->buf, sn);
    } else {
        memmove (msg, msg+1, strlen (msg+1) + 1);
        sscanf(msg, "%s %s", name, message);
        int sn = snprintf(ctx->buf, 1024, "%s%c%s%c",name,'\0',message, '\0');
        clientSend(fd, ctx->buf, sn);
    }    
}

/* msg_pointer
 * this function finds the index of the second null terminator,
 * used for parsing messages.
 */
int msg_pointer(mycontext_t *ctx){
    int char_count = 0;
    int ind = -1;
    int msg_ind = 0;
    for(int i = 0; i < ctx->len; ++i){        
        if(ctx->buf[i] == '\0'){
            char_count++;
            if(char_count == 2){
                ind = i+1;
            }
        }
    }
    return ind;
}

/* do_server
 * this function handles any messages received from the server
 * */
int do_server (int fd, mycontext_t *ctx){    
    int count = clientReceive(fd, &ctx->buf[ctx->len], ctx->size - ctx->len);
    ctx->len += count;
    //gets index of second null
    int msg_ind = msg_pointer(ctx);
    
    if(ctx->len >= ctx->size) {
        ctx_double_capacity(ctx);
    }
    while(msg_ind > 0){
        printbuf(ctx, msg_ind);
        memmove (ctx->buf, ctx->buf + msg_ind, ctx->size - msg_ind);
        ctx->len -= msg_ind;
        memset(ctx->buf + ctx->len, '\0', ctx->size - ctx->len);
        msg_ind = msg_pointer(ctx);
    }
    return count;
}

/* do_timeout
 * this function handles the timeout case
 */
void do_timeout(int fd){
    char* heartbeat = "\0\0";
    clientSend(fd, heartbeat, 2);
}

int main()
{
  //authorization method  
  char* auth = "corotaa\0enhydritic\0";

  //initialize struct
  mycontext_t ctx;
  ctx_init(&ctx);
  
  //authentication
  int fd = connect_to("140.160.136.211", 14367);
  clientSend(fd, auth, 19);
  
  fd_set reads;
  FD_ZERO(&reads);
  FD_SET(0,&reads);
  FD_SET(fd,&reads);
  
  struct timeval Timeout = {2,0};
  int working = 1;
  
  //select loop
  while(working > 0 && 0 <= select(fd +1, &reads, NULL, NULL, &Timeout)){
      if(FD_ISSET(0, &reads)){
        do_stdin(fd, &ctx);
      }
      else if (FD_ISSET(fd, &reads)){
        working = do_server(fd, &ctx);
      }
      else if(select(fd +1, &reads, NULL, NULL, &Timeout) == 0){
        do_timeout(fd);
      }
      Timeout.tv_sec = 2;
      Timeout.tv_usec = 0;
      FD_SET(0, &reads);
      FD_SET(fd, &reads);
  }  
  close(fd);
  return 0;
}
