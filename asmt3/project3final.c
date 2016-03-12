#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "vector.h"
#include <sys/time.h>
#define PORT 2721

/*Austin Corotan 
 *Project 3 CSCI367
 *Chat Server Program
 *3/12/2016
 */

/*Client Stucture
	*/
typedef
struct client_st{
  int fd;
  char user[8];
  struct timeval deadline;
  Vector input;
  Vector output;
}client;

/*Array Structure                                                                                     
 */
typedef
struct array_st{
  client *array;
  int used;
  int size;
}ClientArray;

void initArray(ClientArray *a, size_t initialSize)
{
  // Allocate initial space                                                                           
  a->array = (client *)calloc(initialSize , sizeof(client));
  a->used = 0;           // no elements used                                                          
  a->size = initialSize; // available nr of elements                                                  
}

// Add element to array                                                                               
void insertClient(ClientArray *a, client c)
{
  if (a->used == a->size){
    a->size *= 2;
    a->array = (client *)realloc(a->array, a->size * sizeof(client));
  }

  a->array[a->used].fd = c.fd;
  strcpy(a->array[a->used].user, c.user);
  a->array[a->used].deadline = c.deadline;
  a->array[a->used].input = c.input;
  a->array[a->used].output = c.output;
  a->used++;
}

void removeClient(ClientArray *a, client *c){
  //search ClientArray for client and remove                                                          
  int i;
  int position = -1;
  for(i = 0; i < a->used; ++i){
    if(a->array[i].fd == c->fd){
      position = i;
    }
  }
  printf("%d\n", position);
  //client found                                                                                      
  if(position >= 0){
    int j;
    for(j = position; j < a->size - 1; ++j){
      a->array[j] = a->array[j+1];
    }
    a->used--;
  }
  else{
    printf("Client not found");
  }
}
int searchClient(ClientArray *a, char* user){
  int i;
  int clientIndex = -1;
  for(i= 0; i < a->used; i++){
    if(strcmp(a->array[i].user, user) == 0){
      clientIndex = i;
    }
  }
  if(clientIndex == -1){
    printf("Client not found\n");
  }

  return clientIndex;
}

void vector_init(Vector *vector) {
  vector->len = 0;
  vector->size = VECTOR_INITIAL_CAPACITY;
  vector->data = malloc(sizeof(char) * vector->size);
}

void vector_double_capacity_if_full(Vector *vector) {
  if(vector->size <= vector->len) {
    vector->size *= 2;
    vector->data = realloc(vector->data, sizeof(char) * vector->size);
  }
}
void vector_append(Vector *vector, char *user, char *message)
{
  if(vector->size <= (vector->len + strlen(message))){
    vector->size *= 2;
    vector->data = realloc(vector->data, sizeof(char) * vector->size);
  }
  int sn = snprintf(vector->data + vector->len, vector->size, "%s%c%s%c", user, '\0', message, '\0');
  vector->len+= sn;
}

void vector_free(Vector *vector) {
  free(vector->data);
}

void vector_pop_front(Vector *vector, int count){
  memmove (vector->data, vector->data + count, vector->size - count);
  vector->len -= count;
  memset(vector->data + vector->len, '\0', vector->size - vector->len);
}

void initClient(client *c, int fd, char* user){
  struct timeval Timeout = {5, 0};
  struct timeval currTime;
  struct timeval deadline;
  gettimeofday(&currTime, NULL);
  timeradd(&currTime, &Timeout, &deadline);
  Vector client_in, client_out;
  vector_init(&client_in);
  vector_init(&client_out);
  c->fd = fd;
  strcpy(c->user, user);
  c->deadline = deadline;
  c->input = client_in;
  c->output = client_out;
}

/* Create a new server socket                                                                         
 * port   The port to listen on                                                                       
 * queue  The maximum number of clients waiting to connect                                            
 *                                                                                                    
 * Note:  Lots of early returns.                                                                      
 */
int listen_on(unsigned short port, int queue)
{

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) {
    fprintf(stderr, "Call to socket failed because (%d) \"%s\"\n",
            errno, strerror(errno));
    return -1;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
    fprintf(stderr, "Call to bind failed because (%d) \"%s\"\n",
            errno, strerror(errno));
    close (fd);
    return -1;
  }

  if(listen(fd, queue)) {
    fprintf(stderr, "Call to listen failed because (%d) \"%s\"\n",
            errno, strerror(errno));
    close (fd);
    return -1;
  }

  return fd;
}
/* Client receive                                                                                     
 * This function receives a message from the remote server while handling possible                    
 * errors in the process.                                                                             
 */
int clientReceive (int socket, char* buffer, int bufsize){
  int num;
  num = read(socket, buffer, bufsize);
  if(num < 0){
    fprintf(stderr,"Error in receiving message because (%d) \"%s\"\n",
            errno, strerror(errno));
    exit(1);
  }
  return num;
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
/* Accept Client                                                                                      
 *                                                                                                    
 * This function waits for a client to connect.  When a connection occurs, it                         
 * prints out the address and port of the client before it returns the new                            
 * socket.                                                                                            
 */
int accept_client(int server)
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  bzero(&addr, len);

  int client = accept(server, (struct sockaddr*)&addr, &len);
  if(client == -1){
    fprintf(stderr, "Call to accept failed because (%d) \"%s\"\n",
            errno, strerror(errno));
    return -1;
  }

  fprintf(stderr, "Received a connection from %s:%d\n",
          inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  return client;
}
/* msg_pointer                                                                             
 * this function finds the index of the second null terminator,                            
 * used for parsing messages.                                                              
 */
int auth_msg_pointer(Vector *auth){
  int char_count = 0;
  int ind = -1;
  int msg_ind = 0;
  char null = '\0';
  int i;
  for(i = 0; i < auth->len; ++i){
    if(auth->data[i] == null){
      char_count++;
      if(char_count == 2){
        ind = i+1;
      }
    }
  }
  return ind;
}
/* msg_pointer                                                                             
 * this function finds the index of the second null terminator,                            
 * used for parsing messages.                                                              
 */
int auth_user_pointer(Vector *auth){
  int char_count = 0;
  int ind = -1;
  int msg_ind = 0;
  int i;
  for(i = 0; i < auth->len; ++i){
    if(auth->data[i] == '\0'){
      char_count++;
      if(char_count == 1){
        ind = i+1;
      }
    }
  }
  return ind;
}

//counts null characters in client output buffer                                           
int null_char_count(client *c){
  int char_count = 0;
  int i;
  for(i = 0; i < c->output.len; ++i){
    if(c->output.data[i] == '\0'){
      char_count++;
    }
  }
  return char_count;
}

/*This sets the fd for a client                                                            
 */
void client_fd_set(client *c, fd_set *rd, fd_set *wr) {
  FD_SET(c->fd, rd);
  if(c->output.len > 0)                                                                   
    FD_SET(c->fd, wr);
}

void client_do_socket(ClientArray *a, client *c, fd_set *rd, fd_set *wr){
  if(FD_ISSET(c->fd, rd)){
  int count = clientReceive(c->fd, &c->input.data[c->input.len], c->input.size - c->input.\
len);
  struct timeval currTime;
  gettimeofday(&currTime, NULL);
	//Disconnect if read is 0
  if(count == 0){
    close(c->fd);
    removeClient(a,c);
  }else{
      struct timeval Timeout = {5, 0};
      timerclear(&c->deadline);
      timeradd(&currTime, &Timeout, &c->deadline);
      c->input.len += count;
			vector_double_capacity_if_full(&c->input);
      int msg_ind = auth_msg_pointer(&c->input);
      int user_ind = auth_user_pointer(&c->input);

      if(user_ind != -1 && msg_ind != -1){
        char user[user_ind];
        int msglen = msg_ind - user_ind;
        char msg[msglen];
        strncpy(user, c->input.data, user_ind);
        strncpy(msg, c->input.data + user_ind, msg_ind - user_ind);
        printf("Received: %s %s\n", user, msg);
        if(strcmp(c->user, "standby") == 0){
          strcpy(c->user, user);
          char authentication[80];
          char* ok = "OK";
          char* msg = "";
          int sn = snprintf(authentication,4, "%s%c%s%c", ok, '\0', msg, '\0');
          clientSend(c->fd, authentication, 4);
          printf("Authentication of %s sucessful\n", user);
        }else{

        if(msg_ind - user_ind <= 1){
          printf("Client %s sent a heartbeat\n", c->user);
          vector_pop_front(&c->input, msg_ind);
        }
        if(msg_ind - user_ind > 1){
          if(user_ind > 1){
            int i;
            for(i = 0; i < a->used; ++i){
 							if((strcmp(a->array[i].user, user) == 0)){
                int sn = snprintf(a->array[i].output.data + a->array[i].output.len, a->array[i].output.size, "%s%c%s%c", c->user, '\0', msg, '\0');
                a->array[i].output.len += sn;
                printf("Client %s sent %s to %s\n", c->user, msg, user);
                if(null_char_count(&a->array[i]) >= 4){
                  close(a->array[i].fd);
                  removeClient(a, &a->array[i]);
                }
              }
            }
          } else {
            int i;
            for(i = 0; i < a->used; ++i){
              if(a->array[i].fd != c->fd){
                int sn = snprintf(a->array[i].output.data + a->array[i].output.len, a->array[i].output.size, "%s%c%s%c", c->user, '\0', msg, '\0');
                a->array[i].output.len += sn;                     
                if(null_char_count(&a->array[i]) >= 4){
                  close(a->array[i].fd);
                  removeClient(a, &a->array[i]);
                }
              }
            }
            printf("Client %s broadcast %s\n", c->user, msg);
          }                                         
        } 
				}
        vector_pop_front(&c->input, msg_ind);
      }                                              
    }                                                                                   
  }
  if(FD_ISSET(c->fd, wr)){
    clientSend(c->fd, c->output.data, c->output.len);
    vector_pop_front(&c->output, c->output.len);
  }                                                                    
}

int main(){
  //initialize client array                                                                
  ClientArray clients;
  initArray(&clients, 25);

  int server = listen_on(PORT, 2);
  int maxfd = server;
  Vector auth;
  vector_init(&auth);

  fd_set reads;
  fd_set writes;
  FD_SET(server,&reads);
  struct timeval Timeout = {5,0};
  struct timeval currTime;                                                       
  int working = 1;
  while(working){
    int j;                                                      
    for(j = 0; j < clients.used; ++j){
        client_fd_set(&clients.array[j], &reads, &writes);
    }
    if(0 <= select(maxfd+1, &reads, &writes, NULL, &Timeout)){
      gettimeofday(&currTime, NULL);
      int k;
      for(k = 0; k < clients.used; ++k){
        if(timercmp(&currTime, &clients.array[k].deadline, >) != 0){
          close(clients.array[k].fd);
          removeClient(&clients, &clients.array[k]);
        }
      }
      if(FD_ISSET(server, &reads)){         
        int client_fd = accept_client(server);
        if(client_fd > maxfd){
          maxfd = client_fd;
        }
				//initialize client and set temporary username
        client new_client;
        char* user = "standby";
        initClient(&new_client, client_fd, user);
        insertClient(&clients, new_client);
        printf("New client %s connected on fd %d total number of clients now %d\n", client\
s.array[clients.used-1].user, client_fd, clients.used);
        int q;
      }

      int i;
      for(i = 0; i < clients.used; ++i){
          client_do_socket(&clients, &clients.array[i], &reads, &writes);
      }
      FD_ZERO(&reads);
      FD_ZERO(&writes);
      FD_SET(server, &reads);
    }
    Timeout.tv_sec = 5;
    Timeout.tv_usec = 0;
  }
  close(server);
  return EXIT_SUCCESS;
}

