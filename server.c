#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message);

//taking xor with key and current window of data and writing back to current window
void update_current_window(char* current_window,char* key)
{
	int len_key=strlen(key);
	for(int i=0;i<len_key;i++)
	{
		if(current_window[i]=='0' && key[i]=='0')
			current_window[i]='0';
		else if(current_window[i]=='0' && key[i]=='1')
			current_window[i]='1';
		else if(current_window[i]=='1' && key[i]=='0')
			current_window[i]='1';	
		else if(current_window[i]=='1' && key[i]=='1')
			current_window[i]='0';
	}
}

//shift current window to left by 1 bit
void shift_current_window(char* current_window)
{
	int len=strlen(current_window);
	for(int i=0;i<len-1;i++)
	{
		current_window[i]=current_window[i+1];
	}
}

//crc funtion 
int crc(char* data,char* key)
{
	int len_data=strlen(data);
	int len_key=strlen(key);
	//padding len_key-1 0 at the end of data
	for(int i=0;i<len_key-1;i++)
	data[i+len_data]='0';
	
	//taking window to store current window and update it after taking xor
	char current_window[16];
	for(int i=0;i<len_key;i++)
		current_window[i]=data[i];
		
	//update funtion is simply taking xor with key and updating current window
	update_current_window(current_window,key);
	
	//to left-shift by 1 bit
	shift_current_window(current_window);
	
	//performing operation till end of data 
	for(int i=len_key;i<len_data+len_key-1;i++)
	{
		current_window[len_key-1]=data[i];
		//if initial value is zero left-shift again
		if(current_window[0]=='0')
			shift_current_window(current_window);
		//else update then left-shift
		else
		{
		update_current_window(current_window,key);
		shift_current_window(current_window);
		}
	}
	for(int i=0;i<len_key-1;i++)
	{
		if(current_window[i] == '1')
			return 0;
	}
	return 1;
	
}


int crccheck()
{
	return 1;
}

//convert given string to binary
char* string_to_binary(char *m)
{
	int str_len = strlen(m);
	int it = 0;
	char *binary = (char*)malloc(str_len * 8 * sizeof(char));
	for(int i = 0; i<str_len; i++)
	{
		for(int j = 7; j>=0; j--)
		{
			int n = (((int)(m[i])) & (1<<j));
			if(n == 0)
				binary[it++] = '0';
			else
				binary[it++] = '1';
		}
	}
	binary[it] = '\0';
	return binary;
}

//convert binary data to string
char* binary_to_string(char *m)
{
	int str_len = strlen(m)-8;
	//printf("str_len : %d\n",strlen(m));
	int it1  = 0;
	int it2 = 0;
	char *message = (char*)malloc(((str_len/8)-1)*sizeof(char));
	for(int i = 0; i<str_len-8; i+=8)
	{
		int n = 0;
		for(int j = 7; j>=0; j--)
		{
			if(m[it1++] == '1')
			{
				n+= pow(2,j);
			}
		}
		message[it2++] = (char)n;
	}
	message[it2] = '\0';
	return message;
}

//funtion to insert random error based on given probability
int generate_error(char *data, float BER)
{
	srand(time(0));
	int n = strlen(data);
	if((((float)(rand()%10))/10) < BER)
	{
		printf("\nError generated...\n");
		for(int i = 0; i<n; i++)
		{
			if((((float)(rand()%10))/10) < BER)
			{
				if(data[i] = '0')
					data[i] = '1';
				else
					data[i] = '0';
			}
		}
	}
}

int main(int argc, char *argv[])
{
	system("clear");
	//
	int serv_sock,str_len,n;
	int clnt_sock;
	float BER;//error probability
	char message[1024];
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	printf("Enter the error probability : ");
	scanf("%f",&BER);
	fflush(stdin);
	fflush(stdout);
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	//socket creation
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");
	
	//address 
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));
	
	//binding
	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1 )
		error_handling("bind() error"); 
	
	//listen
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	
	clnt_addr_size=sizeof(clnt_addr);  
	
	//loop to handle multiple client
	while(1)
	{
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);
		if(clnt_sock==-1)
			error_handling("accept() error"); 
		//function to create child process
		if(fork() == 0)
		{
			close(serv_sock);
			while(1)
			{
				bzero(message,1024);
				str_len=read(clnt_sock, message, sizeof(message)+1);
				if(str_len==-1)
					error_handling("read() error!");
				//check if message is corrupted or not
				if(crc(message,"100000111")==1)
				{
					char* m = binary_to_string(message);
					printf("\nClient's message : %s \n",m); 
					if(!strncmp("exit", m, 4))
					{
						break;
					}
					bzero(message,1024);
					printf("Sending ACK...\n");
					sprintf(message,"ACK\0");
					m = string_to_binary(message);
					generate_error(m,BER);
					n = write(clnt_sock, m, strlen(m)+1);
					if(n<0)
					{
						printf("ERROR in writing....\n");
						exit(1);
					}
					
				}
				//message is corrupted
				else
				{
					bzero(message,30);
					printf("Message recieved is currupt...Sending NACK...\n");
					sprintf(message,"NACK\0");
					char* m = string_to_binary(message);
					generate_error(m,BER);
					n = write(clnt_sock, m, strlen(m)+1);
					if(n<0)
					{
						printf("ERROR in writing....\n");
						exit(1);
					}
				}
				
			}
		}
	}
	close(clnt_sock);	
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
