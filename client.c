#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <unistd.h>

//a function from Stevens book
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
	/* Command Line data manupulation*/
	int i,j = 0,k=0;

	//store input as host & port
	char* host = argv[1];
	char* port = argv[2]; 
		
	//Convert Domain to IP
	int socket_fd;
	struct addrinfo temp, *server_info, *pointer;
	struct sockaddr_in *h;
	int error;

	memset(&temp,0,sizeof temp);
	temp.ai_family = AF_UNSPEC;
	temp.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(host,"http",&temp,&server_info))!= 0)
	{
		printf("Error in getaddrinfo \n");
		return 1;
	}

	for (pointer = server_info;pointer!=NULL; pointer = pointer->ai_next)
	{
		h = (struct sockaddr_in*) pointer ->ai_addr;
		strcpy(host,inet_ntoa(h->sin_addr));
	}
	freeaddrinfo(server_info);

	//printf("Server IP: %s \n",host);
	
	//Server Configuration	
	struct sockaddr_in server_address;
	socklen_t address_size;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(port));
	server_address.sin_addr.s_addr = inet_addr(host); 

	//Client Configuration
	int client = socket(AF_INET,SOCK_STREAM,0);
	if (client < 0)
		printf("Error creating client's socket \n");

	//Try to connect to server
	if (connect(client,(struct sockaddr *)&server_address,sizeof(server_address))<0)
		perror("Error connecting \n");
	
	for (i =3; i<argc;i++)
	{
		//Each message can be up to 100 chars as each value and key are a maximum size of 50 bytes 
		char message[100];
		char buffer[100];
		int j = 0;
		int length = 0;
		//If put is read from command line
		if ( strcmp(argv[i],"put") == 0 )
		{		
			message[j] = 'p';
			j++;

			for (int m = 0 ; m<strlen(argv[i+1]);m++)
			{
				message[j]= argv[i+1][m];
				j++;
			}
			message[j]='\0';
			j++;

			for (int m = 0 ; m<strlen(argv[i+2]);m++)
			{
				message[j] = argv[i+2][m];
				j++;
			}
			message[j]='\0';

			//Compute Length = p + length(key) + null terminator + length(value) + null terminator 
			length = 1+strlen(argv[i+1])+1+strlen(argv[i+2])+1;

			//try to send with writen in order to assure all bytes are delivered
			int k = writen(client,message,length);
			i = i + 2;	
		}
		//If get is read from command line
		else if (strcmp(argv[i],"get")==0)
		{
			message[j] = 'g';
			j++;
			for (int m = 0 ; m<strlen(argv[i+1]);m++)
			{
				message[j]= argv[i+1][m];
				j++;
			}
			message[j] = '\0';
			
			//Compute Length = g + length(key) + null terminator  
			length = 1+strlen(argv[i+1])+1;
			
			//try to send with writen in order to assure all bytes are delivered
			if(writen(client,message,length)<0)
				printf("Error Writing ! \n");	

			int size;	//the number of bytes read returns
			int position = 0;	//the position of buffertmp that we should start writing from
			char buffertmp[2048];

			do
			{
				size = read(client,buffertmp,2048);
				//copy what read has returned in order to read again if needed, concat with old buffertmp
				memcpy(buffer+position,buffertmp,size);
				//increase position in order to show where we should write next time
				position = position+size;
				//if the key was not found size would be 1 and buffer[0] == 'n' according to protocol
				if(size==1&&buffer[size-1] =='n')
					break;
				//flush buffetmp
				memset(&buffertmp,0,sizeof(buffertmp));
			}while( buffer[position-1]!='\0');
			
			//print according to protocol
			if (buffer[0] == 'n' )
				printf("\n");
			else if(buffer[0] == 'f')
			{
				for (int i=1;i<strlen(buffer);i++)
					printf("%c",buffer[i]);
				printf("\n");
			}
			i=i+1;
			memset(&buffer,0,sizeof(buffer));
			memset(&buffertmp,0,sizeof(buffertmp));
		}
		//if trash is read send it and null-terminate it
		else
		{
			strcpy(message,argv[i]);
			message[strlen(argv[i])] = '\0';
			length = 1;
			int k = writen(client,message,length);
			i = i+strlen(argv[i]);
		}		
	}
	close(client);
}
