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
#include <sys/stat.h>
#define  SHARD_SIZE 1024


int main(){
    char * data="hello sancho\0";

    int shm_fd=shm_open("process_bridge", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shm_fd==-1){

        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd,SHARD_SIZE);
    void * shm_ptr;
    shm_ptr= mmap(NULL,SHARD_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if (shm_ptr == MAP_FAILED){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    memcpy(shm_ptr,data,strlen(data));

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }


    struct sockaddr_un sockaddr={0};
    sockaddr.sun_family=AF_UNIX;
    strncpy(sockaddr.sun_path,"/tmp/proc.sock",sizeof (sockaddr.sun_path)-1);


    char cmsg_buf[CMSG_SPACE(sizeof (int))];
    memset(cmsg_buf,0,sizeof(cmsg_buf));
    struct msghdr msg={0};
    msg.msg_control=cmsg_buf;
    msg.msg_controllen= CMSG_LEN(sizeof (int));
    msg.msg_name=&sockaddr;
    msg.msg_namelen=sizeof (sockaddr);

    struct cmsghdr* cmsg=NULL;
    cmsg= CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level=SOL_SOCKET;
    cmsg->cmsg_type=  SCM_RIGHTS;
    cmsg->cmsg_len=CMSG_LEN(sizeof (int));
    *(int *)CMSG_DATA(cmsg)=shm_fd;

   struct iovec iv={0};
   iv.iov_base=(void*) data;
   iv.iov_len= strlen(data);
   msg.msg_iov=&iv;
   msg.msg_iovlen=1;

   if (-1== sendmsg(sockfd,&msg,0)){
       perror("sendmsg");
       exit(EXIT_FAILURE);
   }

    close(shm_fd);
    munmap(shm_ptr,SHARD_SIZE);
    close(sockfd);

}