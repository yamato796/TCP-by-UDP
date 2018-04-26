#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#define BUFLEN 512
#define PORT 6006
void handler(int sig) {
    //printf("time  out\n");
}
int main(int argc, char** argv)
{
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM,&act,NULL);
    struct sockaddr_in serv_addr,cli_addr;
    int sockfd, flag,flag2=0,i,cur, slen=sizeof(serv_addr);
    char buf[512],ack[10],temp[10];
    if(argc != 3)
    {
      printf("Usage : %s <Server-IP> filename\n",argv[0]);
      exit(0);
    }
 	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_aton(argv[1], &serv_addr.sin_addr);

    FILE* fp,*fd;
    fp = fopen(argv[2],"rb");
    i = 0, cur=0;
    int j=0,k=0;
    int thershold = 16;
    int winsize = 1;
    int ret;
    while(1){
        bzero(buf,sizeof(buf));
        for(k=0;k<winsize;k++,i++){
            if(k==0){
                cur = i;
            }
            bzero(buf,sizeof(buf));
            ret = fread(buf+10,1,502,fp);
            sprintf(temp,"%s%d","psh",i);
            for(j=0;j<10;j++){
                buf[j] = temp[j];
            }
            flag2 =0;
            if(ret <= 0){
                flag2=1;
                break;
            }
            int ret2;
            if(ret == 502){ // buf is full
                ret2 = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&serv_addr, slen);
                printf("send  data #%d, winsize = %d\n",i,winsize);
            }else{ // the last buffer
                sprintf(temp,"%s%d","las",i);
                for(j=0;j<10;j++){
                    buf[j] = temp[j];
                }
                ret2 = sendto(sockfd, buf, ret+10, 0, (struct sockaddr*)&serv_addr, slen);
                printf("send  data #%d, winsize = %d\n",i,winsize);
            }
        }
        alarm(1); //set timeout alarm
        flag =0;
        if(flag2==0){ // if the actual packet number is less than the window size
            k = winsize;
        }
        for(j=0;j<k;j++){
            int ret3 = recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr*)&serv_addr, &slen);
            if(ret3<0){
                int p;
                if(winsize%2 == 0){           
                    p = winsize/2; 
                }else{
                    p = winsize/2 +1;
                }
                printf("time  out    , thershold = %d\n",p);
                flag =1;
                break;
            }
            printf("recv  ack  #%d\n", atoi(ack+3));
        }
        if(flag == 0){ // recevice all ack
            if(flag2==1){ // if ret is 0
                sendto(sockfd, "end", strlen("end"), 0, (struct sockaddr*)&serv_addr, slen);
                break;
            }
            if(winsize<thershold){
                winsize*=2;
            }else if(winsize>=thershold){
                winsize++;
            }
        }else{ // time out
            i = cur;  //the first packet of current window
            fseek(fp,((cur)*502),SEEK_SET); // rewind the file for reading 
            if(winsize%2 == 0){             // adjust the size of thershold and window size
                thershold = winsize/2; 
            }else{
                thershold = winsize/2 +1;
            }
            winsize = 1;
        }

        alarm(0); // cancel the alarm
        
    }
    fclose(fp);
    close(sockfd);
    return 0;
}
 