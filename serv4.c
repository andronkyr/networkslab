#include <stdio.h>
#include "keyvalue.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>

void worker(int msocket);

/*void sig_chld(int x) {
 while (waitpid(0, NULL, WNOHANG) > 0) {}
 signal(SIGCHLD,sig_chld);
}*/

ssize_t writen(int fd, const void *vptr, size_t n)
{
 size_t nleft;
 ssize_t nwritten;
 const char *ptr;
 ptr = vptr;
 nleft = n;
 while (nleft > 0) 
 {
 	if ( (nwritten = write(fd, ptr, nleft)) <= 0 )
  	{
 		if (errno == EINTR)
 			nwritten = 0; /* and call write() again */
 		else
 			return -1; /* error */
 	}
 nleft -= nwritten;
 ptr += nwritten;
 }
 return n;
}


char key_store[1000][2048];
char value_store[1000][2048];
int count =0;
//declare mutex
//pthread_mutex_t locker;


int main(int argc, char *argv[])
{
	void *doit(void*);
	struct sockaddr_in serverAddress;
	struct sockaddr_in serverStorage;
	socklen_t addr_size;
		
	char* port = argv[1];	

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(port));
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	//Socket Configuration
	int socket1 = socket(AF_INET,SOCK_STREAM,0);
	if (socket1<0)
		perror("Error creating socket");
	int enable = 1;
	setsockopt(socket1,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
	if(bind(socket1,(struct sockaddr*) &serverAddress,sizeof(serverAddress))<0)
		printf("Error Binding! \n");
	listen(socket1,5);
	addr_size = sizeof(serverStorage);
	//initialize mutex
	//int tmp = pthread_mutex_init(&locker,NULL);

	//signal(SIGCHLD,sig_chld);
	int *socket;
	while(1)
	{
		socket = malloc(sizeof(int));
		 *socket = accept(socket1,(struct sockaddr *) &serverStorage,&addr_size);
		if (socket < 0 )
		{
			perror("Error Accepting Connection! \n");
			exit(0);
		}
		
		pthread_t id;

		//create threads and assign them work to do
		pthread_create(&id,NULL,&doit,socket);				
	}	
	//destroy mutex
	//pthread_mutex_destroy(&locker);	
}

void *doit(void * arg)
{
	void worker(int);
	int connfd = *((int*)arg);
	pthread_detach(pthread_self());
	worker((int)connfd);
	close((int) connfd);
	return (NULL);
}

void worker(int msocket)
{	
	//acquire the mutexv
	//pthread_mutex_lock(&locker);
	int socket = (int)msocket;
	int size;
	int buffer_size = 50000;
	//int buffer_size = 2048;
	char buffer[buffer_size];
	char rbuffer[buffer_size];
	char tmp[buffer_size];
	int flag = 1;
	int full = 0;
	memset(&buffer,0,sizeof(buffer));
	while( (size = read(socket,buffer,buffer_size) ) >0 && flag !=0 )
	{ 
		char key[2048];
		char value[2048];
		char command;
		int k;

		if (buffer[0] == '\0')
		{		
			close(socket);
			flag = 0;
			break;
		}

		if (size == buffer_size)
			full = 1;

		
		for (int i = 0;i<size;i++)
		{
			memset(&key,0,sizeof(key));
			memset(&value,0,sizeof(value));	
			memset(&command,0,sizeof(command));
			command = buffer[i];	
			k = 0;
			i++;
			while(buffer[i]!= '\0')
			{
				key[k] = buffer[i];
				i++;
				k++;
			}
			//i++;
			if (command == 'p')
			{
				k = 0;
				i++;
				while(buffer[i]!= '\0')
				{
					value[k] = buffer[i];
					i++;
					k++;
				}
			}

				
			if(command == 'g')
			{				
				memset(&rbuffer,0,sizeof(rbuffer));
				memset(&tmp,0,sizeof(tmp));
				strcpy(tmp, get(key));
				if (strcmp(tmp,"NULL") == 0)
				{
					rbuffer[0] = 'n';
					int k = writen(socket,rbuffer,strlen(rbuffer));
				}
				else
				{
					rbuffer[0] = 'f';
					strcat(rbuffer,tmp);
					rbuffer[strlen(rbuffer)] = '\0';
					int k = writen(socket,rbuffer,strlen(rbuffer)+1);
				}
			}
			else if(command == 'p')
					put(key,value);				
			else
			{	
				close(socket);
				flag = 0;
			}

			memset(&command,0,sizeof(command));
			memset(&key,0,sizeof(key));
			memset(&value,0,sizeof(value));	

			if(full == 1)
			{
				memset(&buffer,0,i);
				memcpy(buffer,buffer+i+1,buffer_size-i-1);
				char buffer2[buffer_size];
				int size2 = 0;						
				int size_tmp = 0;	//the number of bytes read returns
				int position = 0;	//the position of buffertmp that we should start writing from
				char buffertmp[buffer_size];
				do
				{
					size_tmp = read(socket,buffertmp,i+1);
					//copy what read has returned in order to read again if needed, concat with old buffertmp
					memcpy(buffer2+position,buffertmp,size_tmp);
					//increase position in order to show where we should write next time
					position = position+size_tmp;
					//flush buffetmp
					memset(&buffertmp,0,sizeof(buffertmp));
				}while(position>i+1);
				size2=position;
				size = size - i -1 + size2;
				memcpy(buffer+buffer_size-i-1,buffer2,size2);
				memset(&buffer2,0,buffer_size);
				i = -1;
				if(size >= buffer_size)
					full = 1;
				else
					full = 0;
			}
		}
		memset(&buffer,0,sizeof(buffer));			
	}
	//close(socket);
	//release the mutex
	//pthread_mutex_unlock(&locker);
}

void put(char* key,char* value)
{
	int flag = 0;
	for (int i = 0;i<count;i++)
	{
		if (strcmp(key_store[i],key) == 0)
		{
			strcpy(value_store[i],value);
			flag = 1;
		}
	}
	if (flag == 0)
	{	
		strcpy(key_store[count],key);
		strcpy(value_store[count],value);
		count++;
	}
}

char *get(char *key)
{
	for (int i = 0;i<count;i++)
	{
		if (strcmp(key_store[i],key) == 0)
		  return value_store[i];	
	}
	return "NULL" ;
}