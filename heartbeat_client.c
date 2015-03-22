/**
--Usage
*) Correct
gcc -o heartbeat_client heartbeat_client.c
./heartbeat_client  -h127.0.0.1 -p1234 -s"XService" -c"XComponent"
./heartbeat_client  -h115.29.227.99 -p80 -s123456789012345678901234567890 -c1234567890
*) Illegal
echo -n "xxxxxxxxxxxxxxx"|nc 127.0.0.1 1234

--Data packet
[TCP segment of a reassembled PDU]
Frame 215: 194 bytes on wire (1552 bits), 194 bytes captured (1552 bits) on interface 0
Ethernet II, Src: CompalIn_ea:c4:61 (00:23:5a:ea:c4:61), Dst: Tp-LinkT_a2:33:44 (00:23:cd:a2:33:44)
Internet Protocol Version 4, Src: 192.168.2.104 (192.168.2.104), Dst: 115.29.227.99 (115.29.227.99)
Transmission Control Protocol, Src Port: 55468 (55468), Dst Port: http (80), Seq: 1, Ack: 1, Len: 128
  TCP segment data (128 bytes)
  
0000   00 23 cd a2 33 44 00 23 5a ea c4 61 08 00 45 00  .#..3D.#Z..a..E.
0010   00 b4 13 70 40 00 40 06 0d 43 c0 a8 02 68 73 1d  ...p@.@..C...hs.
0020   e3 63 d8 ac 00 50 90 98 2f 38 45 a9 1e 30 80 18  .c...P../8E..0..
0030   00 e5 1a 38 00 00 01 01 08 0a 00 59 3c 5b f2 5a  ...8.......Y<[.Z
0040   da af 31 32 33 34 35 36 37 38 39 30 31 32 33 34  ..12345678901234
0050   35 36 37 38 39 30 31 32 33 34 35 36 37 38 39 30  5678901234567890
0060   00 00 31 32 33 34 35 36 37 38 39 30 00 00 00 00  ..1234567890....
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0080   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00c0   00 00                                            ..

Frame Size 194=14(Ethernet Header)+20(IP Header)+32(TCP Header)+128(TCP Segment Data)
*/

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVICE_LEN 32
#define COMPONENT_LEN 96
#define DATA_LEN (SERVICE_LEN+COMPONENT_LEN)

int send_data(char *host,int port,char *data);

int main(int argc, char *argv[]){
  int opt,port;
  char *host,*service,*component;
  host=NULL;
  port=0;
  service=NULL;
  component=NULL;
  
  /*Accept command line arguments*/
  while((opt=getopt(argc,argv,"h:p:s:c:"))!=-1){
    switch(opt){
    case 'h':
      host=optarg;
      break;
    case 'p':
      port=atoi(optarg);
      break;
    case 's':
      service=optarg;
      break;
    case 'c':
      component=optarg;
      break;
    default: /*'?'*/
      fprintf(stderr,"Usage: %s [-h]host [-p]port [-s]service [-c]component\n",argv[0]);
      exit(-1);
    }
  }
  
  /*Check format*/
  if(host&&port&&service&&component){
    fprintf(stdout,"%s:%d %s.%s\n",host,port,service,component);
    
    if(strlen(service)>=SERVICE_LEN){
      fprintf(stderr,"The service name exceeds %d characters in length\n",SERVICE_LEN-1);
      exit(-1);
    }
    if(strlen(component)>=COMPONENT_LEN){
      fprintf(stderr,"The component name exceeds %d characters in length\n",COMPONENT_LEN-1);
      exit(-1);
    }
    
    char data[DATA_LEN];
    memset(data,'\0',DATA_LEN);
    int i;
    for(i=0;*service!='\0'&&i<SERVICE_LEN;i++)
      data[i]=*service++;
    for(i=SERVICE_LEN;*component!='\0'&&i<DATA_LEN;i++)
      data[i]=*component++;
    
    /*Fire up*/
    if(send_data(host,port,data)==-1){
      fprintf(stderr,"Error send data: %s\n",strerror(errno));
      exit(-1);
    }
  }else{
    fprintf(stderr,"Usage: %s [-h]host [-p]port [-s]service [-c]component\n",argv[0]);
    exit(-1);
  }

  return 0;
}

int send_data(char *host,int port,char *data){
  int sockfd;
  if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
    fprintf(stderr,"Error invoke socket(): %s\n",strerror(errno));
    return -1;
  }
  
  struct hostent *server;
  server=gethostbyname(host);
  if(server==NULL){
    fprintf(stderr,"Error invoke gethostbyname(): %s\n",strerror(h_errno));
    return -1;
  }
  
  struct sockaddr_in servaddr;
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  /*servaddr.sin_addr.s_addr=inet_addr(host);*/
  bcopy((char *)server->h_addr,(char *)&servaddr.sin_addr.s_addr,server->h_length);
  servaddr.sin_port=htons(port);
  if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))==-1){
    fprintf(stderr,"Error invoke connect(): %s\n",strerror(errno));
    return -1;
  }
  
  if(sendto(sockfd,data,DATA_LEN,0,(struct sockaddr *)&servaddr,sizeof(servaddr))==-1){
    fprintf(stderr,"Error invoke sendto(): %s\n",strerror(errno));
    return -1;
  }
  close(sockfd);
  
  return 0;
}

