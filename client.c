#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

int sockfd;
char stdin_str_buffer[128];
int stdin_str_len;
pthread_mutex_t thread_lock;

void *thread(void *arg)
{

    while (1) {
        fgets(stdin_str_buffer, 128, stdin);
        stdin_str_len = strlen(stdin_str_buffer);
        stdin_str_buffer[stdin_str_len-1] = '\0';
        printf("stdin: %s\n", stdin_str_buffer);

    }
}

int main(int argc, char *argv[])
{
    
    char block[1024];
    int file, nread;
    int ret;
    struct sockaddr_in address;
    int addr_len;
    int str_len;
    struct epoll_event ev, events[2];
    int epollfd;
    int nfds, i;
    pthread_t tid;

    pthread_mutex_init(&thread_lock, NULL);

    ret = pthread_create(&tid, NULL, thread, NULL);
    if (ret) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    //get file name
#if 0
    fgets(stdin_str_buffer, 128, stdin);
    stdin_str_len = strlen(stdin_str_buffer);
    stdin_str_buffer[stdin_str_len-1] = '\0';
    if (0 == access(stdin_str_buffer, F_OK)){
        printf("file:%s exist\n", stdin_str_buffer);
    } else {
        printf("file:%s not exist\n", stdin_str_buffer);
        exit(EXIT_FAILURE);
    }

    file = open(stdin_str_buffer, O_RDONLY);
#endif

    //create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //connect to server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(9734);
    printf("s_addr:%d, port:%d\n", address.sin_addr.s_addr, address.sin_port);
    addr_len = sizeof(address);

    ret = connect(sockfd, (struct sockaddr *)&address, addr_len);
    if (-1 == ret) {
        perror("oops: client");
        exit(1);
    }

    printf("server is connected, now copying file\n");

    epollfd = epoll_create(2);
    if (-1 == epollfd){
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLOUT|EPOLLET;
    ev.data.fd = sockfd;
    if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev)){
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    while(1){
        nfds = epoll_wait(epollfd, events, 10, -1);
        if (-1 == nfds){
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for(i=0; i<nfds; i++){
            if (events[i].events & EPOLLOUT){
                #if 0
                nread = read(file, block, sizeof(block));
                if (nread > 0) {
                    write(sockfd, block, nread);
                } else{
                    goto exit;
                }
                #endif
                //printf("ready to write\n");
                if (0 != stdin_str_len) {
                    write(sockfd, stdin_str_buffer, stdin_str_len);
                    stdin_str_len = 0;
                }
                else {
                    usleep(1000);
                }

                ev.events = EPOLLOUT|EPOLLET;
                ev.data.fd = sockfd;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev);
            }

        }
    }

exit:
    close(sockfd);
    exit(EXIT_SUCCESS);

}

