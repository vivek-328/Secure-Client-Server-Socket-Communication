#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
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
void crc(char* data,char* key)
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
	//printf("CRC : %s\n",current_window);
	//padding crc at the end of original data
	int j=0;
	for(int i=len_data;i<len_data+len_key-1;i++)
	{
		data[i]=current_window[j];
		j++;
	}
	data[len_data+len_key-1] = '\0';
}

//convert binary data to string
char* binary_to_string(char *m)
{
	int str_len = strlen(m);
	int it1  = 0;
	int it2 = 0;
	char *message = (char*)malloc((str_len/8)*sizeof(char));
	for(int i = 0; i<str_len; i+=8)
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

int main(int argc, char* argv[])
{
	system("clear");
	int sock,n;
	//BER error probability
	float BER;
	struct sockaddr_in serv_addr;
	
	int str_len,serv_port;
	char message[1024],serv_msg[128];
	char *m;
	if(argc!=3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	printf("Enter the error probability : ");
	scanf("%f",&BER);
	fflush(stdin);
	fflush(stdout);
	
	//client socket creation
	sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");
	
	//address updation 
	serv_port = atoi(argv[2]);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	
	//connect to server
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) 
		error_handling("connect() error!");
	
	//timeout setting
	struct timeval tv;
    tv.tv_sec = 5; 
    tv.tv_usec = 0; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));
	
	//for client to server communication
	while(1)
	{
		bzero(message,1024);
		printf("\nClient : ");
		fgets(message, 1024, stdin); 
		if(message[0] == 10)
		{
			bzero(message,1024);
			fgets(message, 1024, stdin);
		}
		m = string_to_binary(message);
		crc(m,"100000111");
		//insert error
		generate_error(m,BER);
		n = write(sock, m, strlen(m)+1);
		if(n<0)
		{
			printf("ERROR in writing....\n");
			exit(1);
		}
		//until server sends ACK client keep on resending
		while(1)
		{
			bzero(serv_msg,128);
			
			//timeout occours
			if ((recv(sock, serv_msg, sizeof(serv_msg)+1, 0)) < 0) 
			{
				printf("Timeout. Re-transmitting...\n");
				m = string_to_binary(message);
				crc(m,"100000111");
				n = write(sock, m, strlen(m)+1);
				if(n<0)
				{
					printf("ERROR in writing....\n");
					exit(1);
				}
			}
			char* reply = binary_to_string(serv_msg);
			
			//ACK recived close the connection
			if(strcmp(reply, "ACK") == 0)
			{
				printf("\nServer : %s\n", reply);
				printf("\nAcknowledgement recieved...\n");
				break;
			}
			//NACK recieved so resend message
			else if(strcmp(reply, "NACK") == 0)
			{
				printf("\nServer : %s\n", reply);
				printf("\nNACK recieved....Resending message...%s\n",message);
				m = string_to_binary(message);
				crc(m,"100000111");
				generate_error(m,BER);
				n = write(sock, m, strlen(m)+1);
				if(n<0)
				{
					printf("ERROR in writing....\n");
					exit(1);
				}
			} 
		}
	}
	
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
