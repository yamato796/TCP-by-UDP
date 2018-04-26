#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#define BUFLEN 512
#define PORT 5005
int main(void)
{

    struct sockaddr_in my_addr, cli_addr;
    int sockfd, flag,i,now=-1,size,last=-1,list[32],list2[32]; 
    int j,k,max=0;
    socklen_t slen=sizeof(cli_addr);
    char buf[512],buffer[32][502],ack[10],temp[10],tag[4];
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bzero(&my_addr, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr));
    FILE* fp;
    for(j=0;j<32;j++){
        list[j] = -1;
        list2[j] = j;
    }
    fp = fopen("destination","wb");
    i=0;
    int acknum;
    while(1){
        bzero(buf,sizeof(buf));
        int ret = recvfrom(sockfd, buf, 512, 0, (struct sockaddr*)&cli_addr, &slen);
        if(strcmp(buf,"end")==0){
            printf("flush\n"); // flush when the transmit is done
            for(j=0;j<32;j++){
                if(list[j]!=-1){
                    if(last!=j){
                        fwrite(buffer[j],1,502,fp);
                    }else{
                        fwrite(buffer[j],1,list[j],fp);
                    }
                }
            }
            fflush(fp);
            break;
        }

        flag =0;
        for(j=0;j<32;j++){
            if(list[j] < 0){
                flag =1;
                break;
            }
        }
        bzero(temp,sizeof(temp));
        for(j=0,k=3;k<10;j++,k++){
                temp[j] = buf[k];
        }
        acknum = atoi(temp);
        if(flag == 0){
            now = acknum;
            printf("flush\n"); // flush when buffer is full
            for(j=0;j<32;j++){
                if(last!=j){
                    fwrite(buffer[j],1,502,fp);
                }else{
                    fwrite(buffer[j],1,list[j],fp);
                }
                list2[j]+=32;
            }
            fflush(fp);
            bzero(buffer,sizeof(buffer));
            for(j=0;j<32;j++){
                list[j] = -1;
            }
        }else if(flag ==1){
            bzero(temp,sizeof(temp));
            bzero(tag,sizeof(tag));
            for(j=0,k=3;k<10;j++,k++){
                temp[j] = buf[k];
            }
            acknum = atoi(temp);
            printf("recv  data #%d\n",acknum);
            for(j=0;j<3;j++){
                tag[j] = buf[j];
            }
            if(tag[0]== 'l'){ //last buffer
                size = ret-10;
                last = acknum%32;
            }else{
                size = 502;
            }
            if((list[acknum%32] == -1)&&list2[acknum%32] == acknum){// if the buffer is empty and the packet is valid
                for(j=0,k=10;j<size;j++,k++){
                    buffer[acknum%32][j] = buf[k];
                }    
                list[acknum%32] = size;
                sprintf(ack,"%s%d","ack",acknum);
                printf("send  ack  #%d\n",acknum);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr*)&cli_addr, slen);
            }else if(list[acknum%32] != -1 &&list2[acknum%32] == acknum){ // if it is already received
                printf("ignor data #%d\n" , acknum);
                sprintf(ack,"%s%d","ack",acknum);
                printf("send  ack  #%d\n",acknum);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr*)&cli_addr, slen);
            }else if(list2[acknum%32] >= acknum){ //not in its range but already received
                printf("ignor data #%d\n" , acknum);
                sprintf(ack,"%s%d","ack",acknum);
                printf("send  ack  #%d\n",acknum);
                sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr*)&cli_addr, slen);
            }else{ // if it is out of the range of buffer
                printf("drop  data #%d\n", acknum);
            }
        }        
    }
    fclose(fp);
    close(sockfd);
    return 0;
}