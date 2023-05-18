//
// Created by Sancho on 2023/5/17.
//
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(){

    int sock_fd;
    if ((sock_fd= socket(AF_UNIX,SOCK_DGRAM,0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un sock_addr={0};
    //通信类型为进程间通信
    sock_addr.sun_family=AF_UNIX;
    strncpy(sock_addr.sun_path,"/tmp/proc.sock",sizeof (sock_addr.sun_path)-1);
    if (bind(sock_fd,(struct sockaddr *)&sock_addr,sizeof (sock_addr))==-1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    struct msghdr msg={0};
    char cmsg_buf[CMSG_SPACE(sizeof (int))];
    memset(cmsg_buf,0,sizeof(cmsg_buf));
    msg.msg_control=cmsg_buf;
    msg.msg_controllen= sizeof (cmsg_buf);

    char iv_buf[BUFFER_SIZE]={0};
    struct iovec iv={0};
    iv.iov_base=iv_buf;
    iv.iov_len= BUFFER_SIZE;
    msg.msg_iov=&iv;
    //num of io vectors
    msg.msg_iovlen=1;

    printf("receiving message...\n");

    if ((recvmsg(sock_fd,&msg,0))==-1){
        perror("recvmsg");
        exit(EXIT_FAILURE);
    }

    struct cmsghdr* cmsghdr=NULL;
    cmsghdr= CMSG_FIRSTHDR(&msg);
    if (cmsghdr == NULL || cmsghdr->cmsg_len != CMSG_LEN(sizeof(int))) {
        fprintf(stderr, "Invalid control message\n");
        exit(EXIT_FAILURE);
    }

    if (cmsghdr->cmsg_level != SOL_SOCKET || cmsghdr->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "Invalid control message\n");
        exit(EXIT_FAILURE);
    }
    int shm_fd=*(int*)(CMSG_DATA(cmsghdr));

    void * ptr=NULL;
    if((ptr=mmap(NULL,BUFFER_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0))==MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    printf("data:%s\n",(char*)ptr);

    close(shm_fd);
    munmap(ptr,BUFFER_SIZE);
    close(sock_fd);



}