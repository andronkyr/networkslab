#include <stdio.h>
#include "keyvalue.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

//declare key_store,value_store and count as global variables in order to be seen by all functions
char key_store[1000][2048];
char value_store[1000][2048];
int count = 0;

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

int main(int argc, char *argv[])
{
	//Server Configuration
	struct sockaddr_in serverAddress;
	struct sockaddr_in serverStorage;
	socklen_t addr_size;

	//read port number from command line
	char* port = argv[1];	

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(port));
	serverAddress.sin_addr.s_addr = INADDR_ANY; //set to INADDR_ANY in order to receive connections to all interfaces

	//Socket Configuration
	int socket1 = socket(AF_INET,SOCK_STREAM,0);
	if (socket1<0)
		perror("Error creating socket ");	

	//Try binding to port read from command line
	if(bind(socket1,(struct sockaddr*) &serverAddress,sizeof(serverAddress))<0)
		printf("Error Binding! \n");

	//Set listen backlog to 5 according 
	listen(socket1,5);
	addr_size = sizeof(serverStorage);
	int size;
	int buffer_size = 2048;
	char buffer[2048];
	char tmp[2048];
	char rbuffer[2048];
	int full = 0;
	
	while(1)
	{
		//try acceptin connction
		int socket = accept(socket1,(struct sockaddr *) &serverStorage,&addr_size);
		if (socket <0 )
			printf("Error Accepting Connection! \n");
		
		int flag = 1;	//flag indicates an error in what was received (nor put or get)
		while( (size = read(socket,buffer,2048) ) >0 && flag!=0)
		{ 
				if (size == buffer_size)
						full = 1;

				char key[100];
				char value[100];
				char command;
				int j = 0,k;

				//if nothing was received ->close socket
				if(buffer[0] == '\0')
				{
					close(socket);
					flag =0;
					break;
				}

				//iterate all received bytes
				for (int i = 0;i<size;i++)
				{
					//read command
					command = buffer[i];	
					k = 0;
					i++;
					//read key
					while(buffer[i]!= '\0')
					{
						key[k] = buffer[i];
						i++;
						k++;
					}
					//i++;

					//if we should also have a value, read it!
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
				
					//execute get command
					if(command == 'g')
					{	
						//flush memory
						memset(&rbuffer,0,strlen(rbuffer));
						memset(&tmp,0,strlen(tmp));
						strcpy(tmp, get(key));
						
						//if null is returned, the value was not found so return 'n'
						if (strcmp(tmp,"NULL") == 0)
						{
							rbuffer[0] = 'n';
							int k = writen(socket,rbuffer,strlen(rbuffer));
						}
						else	//else concat 'f' with value found and null terminate it
						{
							rbuffer[0] = 'f';
							strcat(rbuffer,tmp);
							rbuffer[strlen(rbuffer)] = '\0';
							int k = writen(socket,rbuffer,strlen(rbuffer)+1);
						}
					}
					//execute put command
					else if(command == 'p')
						put(key,value);
					else // if trash were read
					{	
						close(socket);
						flag = 0;
					}

					//prepare memory for next iteration
					command = '\0';
					memset(&key,0,sizeof(key));
					memset(&value,0,sizeof(value));	

					//if we have found that buffer is full
					if(full == 1)
					{
						//clean the space that last command occupied
						memset(buffer,0,i);
						//shift all buffer's content to the beginning of it
						memcpy(buffer,buffer+i+1,buffer_size-i-1);
						char buffer2[buffer_size];
						int size2 = 0;
						//read in a temporary buffer nr of bytes equal with the size that last command previously occupied
						size2 = read(socket,buffer2,i+1);
						//update size's value in order to show how many bytes are left
						size = size - i -1 + size2;
						//concatenate the two buffers
						memcpy(buffer+buffer_size-i-1,buffer2,size2);
						//flush buffer2
						memset(&buffer2,0,buffer_size);
						//update i in order to make it read from the beginning of the buffer in the next iteration
						i = -1;
						//check if buffer is full
						if(size >= buffer_size)
							full = 1;
						else
							full = 0;
					}
					
				}
				//prepare buffer for next connection
				memset(&buffer,0,sizeof(buffer));
		}
	}	
}

void put(char* key,char* value)
{	
	//flag indicates if a key is found
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
		//increase count in order to write to next position in our arrays
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
	//if it reaches this point, a key will have not been found
	return "NULL" ;
}