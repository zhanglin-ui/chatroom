#include "server.h"

struct client_head *g_head;

struct client_head* init_client_head() {
    struct client_head *head = (struct client_head *)malloc(sizeof(struct client_head));
    if (NULL == head) {
        return NULL;
    }

    head->client_num = 0;
    head->client = NULL;

    return head;
}

void add_client(struct client_node *node) {
    if (NULL == node) {
        printf("add node is null\n");
        return;
    }

    if (g_head->client == NULL) {
        g_head->client = node;
        g_head->client_num++;
        return;
    }

    struct client_node *start = g_head->client;
    while (start->next != NULL) {
        start = start->next;
    }

    start->next = node;
    node->next = NULL;

    return;
}

void del_client(struct client_node node) {
    if (g_head->client == NULL) {
        return;
    }

    struct client_node *start = g_head->client;
    if (start->fd == node.fd) {
        g_head->client = start->next;
        g_head->client_num--;
        return;
    }

    while (start->next->fd != node.fd) {
        start = start->next;
    }

    if (start->next == NULL) {
        return;
    }

    struct client_node *del;
    del = start->next;
    start->next = start->next->next;
    del->next = NULL;

    free(del);

    return;
}

void send_client(char *buff, int fd) {
    struct client_node *start;

    start = g_head->client;
    while (start != NULL) {
        int ret;
        if (start->fd != fd) {
            printf("send buff : %s to %d\n", buff, start->fd);
            ret = send(start->fd, buff, strlen(buff), 0);
            if (ret < 0) {
                perror("send error");
            }
        }
        start = start->next;
    }
}

void *thread_send(void *arg) {
    struct c_msg *msg = (struct c_msg *)arg;
    struct client_node *start;

    start = g_head->client;
    while (start != NULL) {
        int ret;
        if (start->fd != msg->fd) {
            printf("send buff : %s to %d\n", msg->buff, start->fd);
            ret = send(start->fd, msg->buff, strlen(msg->buff), 0);
            if (ret < 0) {
                perror("send error");
            }
        }

        start = start->next;
    }

    return NULL;
}

void free_client() {
    if (g_head->client_num == 0) {
        return;
    }

    struct client_node *node = g_head->client;
    while (node->next != NULL) {
        struct client_node *temp = node;
        node = node->next;
        free(temp);
    }
    
    return;
}

int server_init(char *port) {
    int fd;
    int epfd = 0;
    int new_fd;
    char buff[BUFF_MAX];

    socklen_t socklen;
    pthread_t th;
    struct sockaddr_in sock;
    struct epoll_event ev;
    struct epoll_event events[EPOLL_MAX] = {0};

    if ((epfd = epoll_create(EPOLL_MAX)) == -1) {
        perror("epoll create error");
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("create socket ");
        return -1;
    }

    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = INADDR_ANY;
    sock.sin_port = htons(atoi(port));
    socklen = sizeof(sock);

    if (bind(fd, (struct sockaddr *)&sock, socklen) < 0) {
        perror("bind error");
        close(fd);
        return -1;
    }

    if (listen(fd, 10) < 0) {
        perror("listen error");
        close(fd);
        return -1;
    }

    ev.data.fd = fd;
    ev.events = EPOLLIN;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll add error");
        close(fd);
        return -1;
    }

    g_head = init_client_head();

    while (1) {
        int ep_num = epoll_wait(epfd, events, EPOLL_MAX, -1);
        for (int i = 0; i < ep_num; i++) {
            if (events[i].data.fd == fd) {
                new_fd = accept(fd, (struct sockaddr *)&sock, &socklen);
                if (new_fd < 0) {
                    perror("accept error");
                    continue;
                }

                ev.data.fd = new_fd;
                ev.events = EPOLLIN;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev) < 0) {
                    perror("epoll add error");
                    close(new_fd);
                    continue;
                }

                struct client_node *node = malloc(sizeof(struct client_node));
                if (NULL == node) {
                    perror("malloc error");
                    break;
                }

                node->fd = ev.data.fd;
                node->next = NULL;
                add_client(node);
            } else {
                if (events[i].events & EPOLLIN) {
                    int len = 0;
                    memset(buff, 0, BUFF_MAX);

                    len = recv(events[i].data.fd, buff, BUFF_MAX, 0);
                    if (len == 0) {
                        ev.data.fd = events[i].data.fd;
                        close(ev.data.fd);
                        struct client_node node;
                        node.fd = ev.data.fd;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
                        del_client(node);
                        continue;
                    }

                    struct c_msg msg;
                    msg.fd = events[i].data.fd;
                    strcpy(msg.buff, buff);
                    pthread_create(&th, NULL, thread_send, (void *)&msg);
                    // send_client(buff, events->data.fd);
                } else {
                    printf("epoll : %d\n", events[i].events);
                }
            }
        }
    }

    free_client();
    close(epfd);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("input error\n");
        tips();
        return -1;
    }

    if (strcmp(argv[1], "server") == 0) {
        server_init(argv[2]);
    }

    return 0;
}

void tips() {
    printf("[./a.out] [server] [port]\n");
}
