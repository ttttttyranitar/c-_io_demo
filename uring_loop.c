#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <liburing.h>

#define BUF_SIZE 1024
#define RING_ENTRY_SIZE 1024
#define RING_FETCH_SIZE 32


enum {
    ACCEPT=0,
    READ,
    WRITE
};

typedef struct {
    int connfd;
    int event;

} conn_info;

void set_accept_opt(struct io_uring* ring,int sockfd, struct sockaddr *addr,
        socklen_t *addrlen, int flags){
    struct io_uring_sqe *sqe=io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe,sockfd,addr,addrlen,flags);
    conn_info info={
            .connfd=sockfd,
            .event=ACCEPT,
    };
    //append metadata to sqe node
    memcpy(&sqe->user_data,&info,sizeof(info));
}

void set_recv_opt(struct io_uring* ring,int clientFd, void *buf, size_t len, int flags){

    struct io_uring_sqe *sqe= io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe,clientFd,buf,len,flags);

    conn_info info={
            .connfd=clientFd,
            .event=READ,
    };
    memcpy(&sqe->user_data,&info,sizeof(conn_info));
}

void set_write_opt(struct io_uring* ring,int clientFd, void *buf, size_t len, uint64_t offset){
    struct io_uring_sqe *sqe= io_uring_get_sqe(ring);
    io_uring_prep_write(sqe,clientFd,buf,len,offset);

    conn_info info={
            .connfd=clientFd,
            .event=WRITE,
    };

    memcpy(&sqe->user_data,&info,sizeof(conn_info));
}



int main(){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // io

    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof (struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    servaddr.sin_port = htons(9999);

    if (-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))) {
        printf("bind failed: %s", strerror(errno));
        return -1;
    }

    listen(sockfd, 10);


    struct io_uring_params params;
    memset(&params,0 ,sizeof (struct io_uring_params));
    struct io_uring ring;
    io_uring_queue_init_params(RING_ENTRY_SIZE,&ring,&params);

    struct sockaddr_in client;
    socklen_t len=sizeof(struct  sockaddr);
    set_accept_opt(&ring,sockfd,(struct sockaddr*)&client,&len,0);


    char buffer[BUF_SIZE]={0};
    uint32_t nready;

    while(1){

        io_uring_submit(&ring);


        struct io_uring_cqe* cqes[RING_FETCH_SIZE];
        nready=io_uring_peek_batch_cqe(&ring,cqes,RING_FETCH_SIZE);


        int i=0;
        for(i=0;i<nready;i++){
            struct io_uring_cqe* cqe= cqes[i];
            conn_info _info;
            //set cqe data into info
            memcpy(&_info,&cqe->user_data, sizeof(_info));

            if (_info.event==ACCEPT){
                printf("accept --> %d, %d\n", _info.connfd, cqe->res);
                if(cqe->res<0){
                    continue;
                }

                int connfd=cqe->res;
                set_recv_opt(&ring,connfd,buffer,BUF_SIZE,0);
                set_accept_opt(&ring,sockfd,(struct sockaddr*)&client,&len,0);
            }
            else if (_info.event==READ){
                if (cqe->res<0){
                    continue;
                }

                if (cqe->res==0){
                    close(_info.connfd);
                }else{
                    printf("recv --> %s, %d\n", buffer, cqe->res);
                    set_write_opt(&ring,_info.connfd,buffer,cqe->res,0);
                }


            }
            else if (_info.event==WRITE){
                set_recv_opt(&ring,_info.connfd,buffer,BUF_SIZE,0);
            }
        }

        io_uring_cq_advance(&ring,nready);

    }


}


