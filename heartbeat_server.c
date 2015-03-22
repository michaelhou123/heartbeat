/**
--Usage
*) Debian 32bit
apt-get install libmysqlclient-dev
gcc heartbeat_server.c -o heartbeat_server -lpthread -I/usr/include/mysql -lmysqlclient
*) CentOS 64bit
gcc heartbeat_server.c -o heartbeat_server -lpthread -I/usr/include/mysql -L/usr/lib64/mysql -lmysqlclient

--Process
main --> spawn a producer_thread -> receive from socket -> enqueue
     `-> spawn consumer_threads -> dequeue -> insert into database
      
--MySQL
http://dev.mysql.com/doc/refman/5.5/en/c-api-function-overview.html
mysql_config --cflags, gcc -c `mysql_config --cflags` progname.c
mysql_config --libs, gcc -o progname progname.o `mysql_config --libs`

--Thread Safe
pthread_mutex_t mutex;
pthread_mutex_init(&mutex,NULL);
pthread_mutex_lock(&mutex);
pthread_mutex_unlock(&mutex);
pthread_mutex_destroy(&mutex);
*/


#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <my_global.h>
#include <mysql.h>

/*Socket*/
#define SERVER_PORT    1234
#define BACKLOG        512
/*Data*/
#define CURRENT_TIME_LEN 20
#define HOST_LEN         16
#define SERVICE_LEN      32
#define COMPONENT_LEN    96
#define DATA_LEN      (SERVICE_LEN+COMPONENT_LEN)
/*Queue*/
#define QUEUE_MAX_LEN  65535
#define CONSUMER_THREADS_COUNT  10
#define CONSUMER_SLEEP_INTERVAL 5 /*Seconds*/
/*DB*/
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PWD  "123456"
#define DB_NAME "db_heartbeat"
/*LOG*/
#define LOGFILE "/tmp/heartbeat_server.log"

struct node{
  struct node *next;
  char current_time[CURRENT_TIME_LEN];
  char host[HOST_LEN];
  char data[DATA_LEN];
};
struct queue{
  struct node *head;
  struct node *tail;
  unsigned len;
  /*A simple but low performance thread-safe implementation*/
  pthread_mutex_t mutex;
};
void init(struct queue *q);
int enqueue(struct queue *q,char current_time[],char host[],char data[]);
int dequeue(struct queue *q,char current_time[],char host[],char data[]);

void produce(struct queue *q);
void consume(struct queue *q);

int get_current_time(char current_time[]);
void logger(FILE *stream,char *format,...);
static FILE *logfd;

int main(){
  if(!(logfd=fopen(LOGFILE,"a"))){
    perror("Error invoke fopen()\n");
    exit(-1);
  }

  struct queue q;
  init(&q);
  
  pthread_t producer_thread;
  pthread_t consumer_threads[CONSUMER_THREADS_COUNT];
    pthread_t tid;
  if(!pthread_create(&producer_thread,NULL,(void *)&produce,(void *)&q)){
    logger(stdout,"Success invoke pthread_create(), created a producer thread\n");
    /*printf("tid %lu\n",pthread_self());*/
    /*pthread_detach(producer_thread);*/
  }
  int i;
  for(i=0;i<CONSUMER_THREADS_COUNT;i++){
    if(!pthread_create(&consumer_threads[i],NULL,(void *)&consume,(void *)&q)){
      logger(stdout,"Success invoke pthread_create(), created a consumer thread %d\n",i);
      /*pthread_detach(consume_thread);*/
    }
    /*If no sleep, process may throw "Segmentation fault (core dumped)"*/
    sleep(1);
  }
    
  pthread_join(producer_thread,NULL);
  for(i=0;i<CONSUMER_THREADS_COUNT;i++)
    pthread_join(consumer_threads[i],NULL);
  
  pthread_mutex_destroy(&q.mutex);

  return 0;
}

int get_current_time(char current_time[]){
  time_t rawtime;
  time(&rawtime);

  struct tm *timeinfo;
  if((timeinfo=localtime(&rawtime))==NULL){
    logger(stderr,"Error invoke localtime(): %s\n",strerror(errno));
    return -1;
  }
  
  /*sizeof(current_time)=4 in 32bit; %T=%H:%M:%S*/
  if(strftime(current_time,CURRENT_TIME_LEN,"%Y-%m-%d %T",timeinfo)==0) {
    logger(stderr, "Error invoke strftime()\n");
    return -1;
  }
  
  return 0;
}

void logger(FILE *stream,char *format,...){
  time_t timestamp;
  time(&timestamp);
  
  char *current_time,*ptr;
  current_time=ctime(&timestamp);
  /*strrchr(current_time,'\n');*/
  current_time[strlen(current_time)-1]='\0';
  fprintf(logfd,"%s -- ",current_time);
           
  va_list ap;
  va_start(ap,format);
  vfprintf(logfd,format,ap);
  va_end(ap);
  
  fflush(logfd);
}

void produce(struct queue *q){
  int server_fd;
  if((server_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
    logger(stderr,"Error invoke socket(): %s\n",strerror(errno));
    exit(-1);
  }
  
  int optval=1;
	if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval))==-1){
	  logger(stderr,"Error invoke setsockopt(): %s\n",strerror(errno));
		exit(-1);
	}
  
  struct sockaddr_in server_addr;
  bzero(&server_addr,sizeof(server_addr));
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(SERVER_PORT);
  server_addr.sin_addr.s_addr=htons(INADDR_ANY);
  if(bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1){
    logger(stderr,"Error invoke bind(): %s\n",strerror(errno));
    close(server_fd);
    exit(-1);
  }
  
  if(listen(server_fd, BACKLOG)==-1){
    logger(stderr,"Error invoke listen(): %s\n",strerror(errno));
    exit(-1);
  }
  
  int client_fd;
  struct sockaddr_in client_addr;
  char current_time[CURRENT_TIME_LEN],host[HOST_LEN],data[DATA_LEN];
  while(1){
    socklen_t len=sizeof(client_addr);
    if((client_fd=accept(server_fd,(struct sockaddr *)&client_addr,&len))==-1){
      logger(stderr,"Error invoke accept(): %s, will skip current client\n",strerror(errno));
      continue;
    }
    
    memset(data,'\0',DATA_LEN);
    if(recvfrom(client_fd,data,DATA_LEN,0,(struct sockaddr *)&client_addr,&len)==-1){
      logger(stderr,"Error invoke recvfrom(): %s, will skip current client\n",strerror(errno));
      close(client_fd);
      continue;
    }
    
    memset(current_time,'\0',CURRENT_TIME_LEN);
    if(get_current_time(current_time)==-1){
      logger(stderr,"Error invoke get_current_time()\n");
      continue;
    }
    
    logger(stdout,"Info raw data %s => %s:%d ->- %s:%d => %s.%s\n",\
           current_time,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),\
           inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port),data,&data[SERVICE_LEN]);
    
    if(data[SERVICE_LEN-1]=='\0'&&data[SERVICE_LEN]!='\0'&&data[DATA_LEN-1]=='\0'){
      logger(stdout,"Info correct data => %s.%s\n",data,&data[SERVICE_LEN]);
      
      memset(host,'\0',HOST_LEN);
      strncpy(host,inet_ntoa(client_addr.sin_addr),HOST_LEN);
      
      pthread_mutex_lock(&q->mutex);
      if(q->len<QUEUE_MAX_LEN)
        if(enqueue(q,current_time,host,data)==-1){
          logger(stderr,"Error invoke enqueue()\n");
          pthread_mutex_unlock(&q->mutex);
          pthread_exit(0);
        }
      logger(stdout,"Info produce thread, current queue length is %u\n",q->len);
      pthread_mutex_unlock(&q->mutex);
    }else{
      data[DATA_LEN-1]='\0';
      logger(stdout,"Info illegal data => %s\n",data);
    }
    
    close(client_fd);
  }
  
  close(server_fd);
  pthread_exit(0);
  fclose(logfd);
}

void consume(struct queue *q){
  MYSQL *conn;
  char current_time[CURRENT_TIME_LEN];
  char host[HOST_LEN];
  char data[DATA_LEN];
  char sql[256];
  /*Theoretically, in the worst case, need length*2+1*/
  char escaped_service[65]; /*32*2+1*/
  char escaped_component[193]; /*96*2+1*/
  unsigned long escaped_len;
  
  while(1){
    pthread_mutex_lock(&q->mutex);
    if(q->len>0){
      if(dequeue(q,current_time,host,data)==-1){
        logger(stderr,"Error invoke dequeue()\n");
        pthread_mutex_unlock(&q->mutex);
        pthread_exit(0);
      }
      logger(stdout,"Info consume thread, current queue length is %u\n",q->len);
    pthread_mutex_unlock(&q->mutex);
      
      /*Update Data in MySQL*/
      if((conn=mysql_init(NULL))==NULL){
        logger(stderr,"Error invoke mysql_init(), maybe insufficient memory, will try again after 10 seconds\n");
        sleep(10);
        continue;
      }
      if(!mysql_real_connect(conn,DB_HOST,DB_USER,DB_PWD,DB_NAME,0,NULL,0)){
        logger(stderr,"Error invoke mysql_real_connect(): %s, will try again after 10 seconds\n",
                mysql_error(conn));
        sleep(10);
        continue;
      }
      
      /*Escape Special Characters Like "\", "'", """, NUL(ASCII 0), "\n", "\r"*/
      memset(escaped_service,'\0',65);
      escaped_len=mysql_real_escape_string(conn,escaped_service,data,strlen(data));
      escaped_service[escaped_len]='\0';
      memset(escaped_component,'\0',193);
      escaped_len=mysql_real_escape_string(conn,escaped_component,&data[SERVICE_LEN],strlen(&data[SERVICE_LEN]));
      escaped_component[escaped_len]='\0';
      
      /*Construct SQL*/
      memset(sql,'\0',256);
      sprintf(sql,"update tb_heartbeat set host='%s',beat_time='%s' where service='%s' and component='%s'",
              host,current_time,escaped_service,escaped_component);
      logger(stdout,"Info sql is %s\n",sql);
      
      if(mysql_query(conn,sql))
        logger(stderr,"Error invoke mysql_query(): %s\n",mysql_error(conn));
      if(mysql_affected_rows(conn)==0)
        logger(stdout,"Warning unregistered heartbeat name %s => %s.%s\n",host,data,&data[SERVICE_LEN]);
        
      mysql_close(conn);
    }else{
    pthread_mutex_unlock(&q->mutex);
      /*logger(stdout,"Info consume thread, current queue is empty\n");*/
      sleep(CONSUMER_SLEEP_INTERVAL);
    }/*End if(q->len>0){*/
  }/*while(1){*/
  
  pthread_exit(0);
}

void init(struct queue *q){
  q->head=NULL;
  q->tail=NULL;
  q->len=0;
  
  if(pthread_mutex_init(&q->mutex,NULL)!=0){
    logger(stderr,"Error invoke pthread_mutex_init(), maybe insufficient resource or no privilege\n");
    exit(-1);
  }
}

int enqueue(struct queue *q,char current_time[],char host[],char data[]){
  struct node *new_node;
  new_node=malloc(sizeof(struct node));
  if(new_node==NULL){
    logger(stderr,"Error invoke malloc() when enqueue %s\n",strerror(errno));
    return -1;
  }
  
  new_node->next=NULL;
  int i;
  for(i=0;i<CURRENT_TIME_LEN;i++)
    new_node->current_time[i]=current_time[i];
  for(i=0;i<HOST_LEN;i++)
    new_node->host[i]=host[i];
  for(i=0;i<DATA_LEN;i++)
    new_node->data[i]=data[i];
  
  if(q->len<=QUEUE_MAX_LEN){
    if(q->len==0){
      q->head=new_node;
      q->tail=new_node;
    }else{
      q->tail->next=new_node;
      q->tail=new_node;
    }
    q->len+=1;
  }else{
    logger(stderr,"Error exceeds QUEUE_MAX_LEN %u\n",QUEUE_MAX_LEN);
    return -1;
  }
  
  return 0;
}

int dequeue(struct queue *q,char current_time[],char host[],char data[]){
  if(q->len==0){
    logger(stdout,"Info current queue is empty\n");
    return 0;
  }
  
  int i;
  for(i=0;i<CURRENT_TIME_LEN;i++)
    current_time[i]=q->head->current_time[i];
  for(i=0;i<HOST_LEN;i++)
    host[i]=q->head->host[i];
  for(i=0;i<DATA_LEN;i++)
    data[i]=q->head->data[i];
  
  struct node *old_node;
  old_node=q->head;
  q->head=q->head->next;
  free(old_node);
  q->len-=1;
  
  return 0;
}
