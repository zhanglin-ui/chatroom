#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define EPOLL_MAX 10
#define BUFF_MAX 1024

struct client_node {
    int fd;
    struct client_node *next;
};

struct client_head {
    int client_num;
    struct client_node *client;
};

struct c_msg {
    int fd;
    char buff[BUFF_MAX];
};

struct client_head* init_client_head();
void add_client(struct client_node *node);
void del_client(struct client_node node);
void send_client(char *buff, int fd);
void* thread_send(void *arg);
void free_client();