#include <sys/socket.h> // for socket()
#include <arpa/inet.h>  // for add6
#include <stdio.h>      // for printf()
#include <unistd.h>     // for read()
#include <stdlib.h>     // for malloc()
#include <string.h>     // for strlen()
#include <time.h>       // for time()
#include <fcntl.h>      // for fcntl()
#include <ncurses.h>	 // for getch()
#include <poll.h>	 // for poll() instead of read() - nonblocking socket

// sockets
#define PORT 0xC399     // get it? CS 399?
#define DOMAIN AF_INET6 // ipv6
#define LOOPBACK "::1"  // "loopback" ipv6 - network locally

// debug
#define VERBOSE 1
#define BUFF 16

// booleans
#define TRUE 1
#define FALS 0
//typedef int bool;	//gcc ERROR here ???

// gameplay
#define HIGH 23
#define WIDE 80

#define SNAK '&'
#define SNEK 'O'

#define REDO 'r'
#define QUIT 'q'

#define FORE 'w'
#define BACK 's'
#define LEFT 'a'
#define RITE 'd'


//macros by Juni
#define STAY '0'

#define SPEED 1

// shorter names for addresses and casts
typedef struct sockaddr_in6 *add6;
typedef struct sockaddr *add4;

//client loop
int cloop(int *sock)
{

	char buffer[BUFF]; //16
	char to_send[2];
	char *c;
	int r;
	
	//configure stdin to be non-blocking
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	
	//initscr();
	while(buffer[0] != QUIT) //blocking wait "non-busy wait"
	{
		//fgets(buffer, BUFF, stdin); //this will block until '\n'
		//non-blocking get from stdin
		//c = (char)fgetc(stdin); //this only sends to program after '\n'
		//c = getch();
		
		//"Default" behavior: buffer should be "0"
		memset(buffer, '\0', BUFF);
		memset(to_send, '\0', 2);
		//buffer[0] = '0';
		//printf("buf cleared\n");
		
		//wait for one second
		//unsigned sleep(unsigned seconds);
		sleep(SPEED);
		
		//read a single character from console

		//BUFF number bytes 
		r = read(0, buffer, BUFF);
		//buffer.len - 2?? to account for the '\n'
		/*switch(buffer[strlen(buffer)-2])
		{
			case REDO:
				to_send[0] = REDO;
				break;
			case QUIT:
				to_send[0] = QUIT;
				break;
			case FORE:
				to_send[0] = FORE;
				break;
			case BACK:
				to_send[0] = BACK;
				break;
			case LEFT:
				to_send[0] = LEFT;
				break;
			case RITE:
				to_send[0] = RITE;
				break;
			default:
				to_send[0] = STAY;
				break;
		}*/
		
		char t = buffer[strlen(buffer)-2];
		if(t != REDO && t != QUIT && t != FORE && t != BACK && t != LEFT && t != RITE)
		{
			to_send[0] = STAY;
		} else {
			to_send[0] = t;
		}
    
		/*if (r == -1) {	exit(-1); } 
		else */
		if(VERBOSE) { if (r > 0) { fprintf(stderr,"buf set to: %s\n",buffer); } }
		
		//check to see if SPEED seconds have passed since last transmission.
		write(*sock, to_send, 1);
		//printf("sent buf: \"%s\"\n",to_send);
		if (VERBOSE) { fprintf(stderr,"sent buf: \"%s\"\n",to_send); }
		
		//write to socket		
		//write(sock, buffer, strlen(buffer)); //length param is in *bytes*
		//write(sock, &c, 1);
		
		/* if (read) { send the character that was read } else { send null character }
			a second passed, so send
			store the last packet server-side so snek continues in last direction
		*/
	}
	//endwin();
	printf("Quit command received. Exiting client.\n");
	
	return 0;
}

//server loop
int sloop(int *sock, struct sockaddr_in6 *address)
{
	int addrlen = sizeof(*address); 
	int connection = accept(*sock, (add4)address, &addrlen); 
	printf("Connection received. Starting game.\n"); 
	    
	//
	char buffer[BUFF] = {0};
	char c;
	
	while(buffer[0] != QUIT) 	//shuts down the server when QUIT is recieved from client
	{
		memset(buffer, '\0', BUFF);
	    	read(connection, buffer, BUFF); //blocking call
	    	
	    	/* 
	    	if the buffer is "0"
	    		local character keeps its value
	    	else
	    		overwrite local character with new input received
	    	*/
	    	if(buffer[0] != STAY)
	    	{
	    		c = buffer[0];
	    	}
	    	
	    	if (VERBOSE) { fprintf(stderr,"c: %c\tbuf: \"%s\"\n", c, buffer); }
	}
	printf("\nQuit command received. Closing server.\n");
	
	return 0;
}

//shared networking tasks
void get_sock(int *sock, struct sockaddr_in6 *address, bool is_server)
{
	if (VERBOSE) { fprintf(stderr,"Getting socket...\n"); }
	
	*sock = socket(DOMAIN, SOCK_STREAM, 0);

	address->sin6_family = DOMAIN; 
	address->sin6_port = htons(PORT); // "In practice, only the port field needs to be formatted with htons()"
}

//initializes client
int client()
{ 
	int sock;
	struct sockaddr_in6 address; 
	
	printf("Starting client...\n");
	
	get_sock(&sock, &address, FALS);
	inet_pton(DOMAIN, "::1", &address.sin6_addr); // "::1" is IPv6 "loopback" address

	connect(sock, (add4)&address, sizeof(address));
	
	cloop(&sock);
	
	return 0; 
}

//initializes server
int server()
{ 
	int sock; 
	struct sockaddr_in6 address; 

	printf("Starting server...\n");
	    
	get_sock(&sock, &address, TRUE);
	address.sin6_addr = in6addr_any; 
		
	int opt = 1; // True
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	    	
	bind(sock, (add4)&address, sizeof(address));
	listen(sock, 1);
	
	sloop(&sock, &address);
	
	return 0; 
}

//main: accepts one commandline argument, to run either server or client.
int main(int argc, char const *argv[])
{
	printf("%s, expects (1) arg, %d provided", argv[0], argc-1);
	if (argc == 2)
	{
		printf(": \"%s\".\n", argv[1]);
	} else {
		printf(".\n");
		return 1;
	}
	
	if (argv[1][1] == 's')
	{
		server();
	}
	
	if (argv[1][1] == 'c')
	{
		client();
	}
	
	if (argv[1][1] == 'h')
	{
		printf("HELP: Usage:\n-s for server\n-c for client\nq to quit\n");
	}
	
	srand(time(NULL)); // Initialization, should only be called once
	int r = rand();    // A pseudo-random integer in [0,RAND_MAX]
	if(VERBOSE) { fprintf(stderr,"%d",r) }
	
	return 0;
}
