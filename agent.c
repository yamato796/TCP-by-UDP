#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>
#include <time.h>
#define BUFLEN 512
#define PORT 6006
struct sockaddr_in my_addr, cli_addr , rec_addr , rec_cli_addr;
int sockfd,sockfd2, flag,i,size,last=-1,list[32]; 
int j,k,acknum;
double num=0,drop=0;
socklen_t slen=sizeof(cli_addr),slen2=sizeof(rec_addr);
char buf[512],buff[512],ack[10],temp[10],tag[4];
void *receive() // function to receive from ./recv and fwd to ./send
{
    while(1){
        bzero(buff,sizeof(buff));
        int ret = recvfrom(sockfd2, buff,512, 0, (struct sockaddr *) &rec_addr, &slen);
        printf("get  ack #%d\n",atoi(buff+3));
        if(ret<0){
            exit(1);
        }
        printf("fwd  ack #%d\n",atoi(buff+3));
        int ret2 = sendto(sockfd, buff, ret , 0 , (struct sockaddr *) &cli_addr, slen);
        if(ret2<0){
            exit(1);
        }
    }
}
int main(void)
{
    srand(time(NULL));
    pthread_t *thread;
    thread = (pthread_t *)malloc(1*sizeof(pthread_t));
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockfd2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bzero(&my_addr, sizeof(my_addr));
    bzero(&rec_addr, sizeof(rec_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(5005);
    inet_aton("127.0.0.1", &rec_addr.sin_addr);

    bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr));
    pthread_create(&thread[0],NULL,&receive,NULL); // create a thread to listen from ./recv
    while(1){
        bzero(buf,sizeof(buf));
        int ret = recvfrom(sockfd, buf, 512, 0, (struct sockaddr*)&cli_addr, &slen);
        if(strcmp(buf,"end")==0){
            sendto(sockfd2, buf, strlen(buf), 0, (struct sockaddr*)&rec_addr, slen2);
            break;
        }else{
            num++;
            bzero(temp,sizeof(temp));
            bzero(tag,sizeof(tag));
            for(j=0,k=3;k<10;j++,k++){
                temp[j] = buf[k];
            }
            acknum = atoi(temp);
            printf("get  data #%d\n",acknum);
            if((rand()%40) == 1){ // randomly drop package
                drop++;
                printf("drop data #%d, loss rate = %lf\n",acknum, (drop/num));
            }else{
                sendto(sockfd2, buf, ret, 0, (struct sockaddr*)&rec_addr, slen2);
                printf("fwd  data #%d, loss rate = %lf\n",acknum, (drop/num));
            }
        }        
    }
    close(sockfd);
    return 0;
}