#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#define SERVER_PORT 9734

void setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(sock, F_GETFL)");
        exit(1);
    }

    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock, F_SETFL)");
        exit(1);
    }

}

int main(int argc, char *argv[])
{
    int server_sockfd, client_sockfd;
    int server_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char read_buffer[1024];
    int nr_read;
    int file;
    int nfds, i;
    struct epoll_event ev, events[2];
    int epollfd;
    int close_flag;

    //create server socket fd
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(server_sockfd);

    //name the socket
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);
    server_len = sizeof(server_address);
    bind(server_sockfd, (struct sockaddr *)&server_address, server_len);

    //create a connection queue and wait for clients
    listen(server_sockfd, 5);
    printf("listening client.\n");

    epollfd = epoll_create(2);
    if (-1 == epollfd){
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_sockfd;
    if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, server_sockfd, &ev)){
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    while(1){
        nfds = epoll_wait(epollfd, events, 10, -1);
        if (-1 == nfds){
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (i=0; i<nfds; i++){
            if (events[i].data.fd == server_sockfd){
                client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &server_len);
                if (-1 == client_sockfd){
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                setnonblocking(client_sockfd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_sockfd;
                if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sockfd, &ev)){
                    perror("epoll_ctl: client_sockfd");
                    exit(EXIT_FAILURE);
                }
                printf("has one client connected!\n");
            } else {
                close_flag = 0;
                nr_read = read(events[i].data.fd, read_buffer, sizeof(read_buffer)); 
                printf("nr_read: %d\n", nr_read);
                if (0 == nr_read){
                    close_flag = 1;
                    printf("client[%d] is closed\n", events[i].data.fd);
                }   
                else if(-1 == nr_read){
                    if (errno != EAGAIN && errno != EWOULDBLOCK){
                        close_flag = 1;
                        perror("read");
                    }
                    else {
                        printf("read again\n");
                    }
                }
                else if (nr_read > 0) {
                    printf("server read: %s\n", read_buffer);
                }
                
                //ev.events = EPOLLIN|EPOLLET;
                //ev.data.fd = events[i].data.fd;
                //epoll_ctl(epollfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);

                if (1 == close_flag) {
                    close(events[i].data.fd);

                    ev.events = EPOLLIN|EPOLLET;
                    ev.data.fd = events[i].data.fd;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                }
                
            }
        }
    }

    close(server_sockfd);

    return EXIT_SUCCESS;
}
