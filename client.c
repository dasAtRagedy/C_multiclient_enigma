/*
** client.c -- a stream socket client
*/

/*** Libraries ***/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

/*** Defines ***/
#define BUFFER 2048
#define check(expr) if (!(expr)) { perror(#expr); kill(0, SIGTERM); }
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data ***/
struct termios orig_termios;
typedef struct pthread_arg_t {
    int socket_fd;
    struct sockaddr_in client_address;
} pthread_arg_t;

/*** Declarations ***/
void *pthread_routine(void *arg);
int sendall(int s, char *buf, int *len);
void disableRawMode();
void enableRawMode();

/*** Init ***/
int main(int argc, char *argv[]){
    if(argc != 2) {
        printf("Invalid number of arguments, program usage: ./client port");
        return 1;
    }

	int port;
	if((port = atoi(argv[1])) == 0 || port < 49152 || port > 65535){
        printf("Offset can only be a positive integer (49152 - 65535)");
		return 1;
    }

    struct addrinfo hints;
    struct addrinfo *servinfo;
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int status = getaddrinfo(NULL, argv[1], &hints, &servinfo);
    check(status == 0);

    int socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    check(socket_fd != -1);
    check(connect(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen) != -1);

	pthread_attr_t pthread_attr;
	check(pthread_attr_init(&pthread_attr) == 0);
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

	pthread_arg_t *pthread_arg;
	pthread_t pthread;

	pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);
    if (!pthread_arg) {
        perror("malloc");
        exit(1);
    }

	pthread_arg->socket_fd = socket_fd;
	
	if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
        perror("pthread_create");
        free(pthread_arg);
		exit(1);
    }
	
	char message[BUFFER] = {};
    int bytesleft;
	
	bzero(message, BUFFER);
	
    while (read(STDIN_FILENO, &message, BUFFER) > 0){
		message[strcspn(message, "\n")] = 0;
		bytesleft = strlen(message);
		if(sendall(socket_fd, message, &bytesleft) == -1) {
			printf("Unable to send the message, terminating the program, string length left unsent: %d", bytesleft);
			break;
		}
		bzero(message, BUFFER);
    }
	
	pthread_cancel(pthread);

    freeaddrinfo(servinfo);

	pthread_join(pthread, NULL);

    return 0;
}

/*** Terminal ***/
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    tcgetattr(STDIN_FILENO, &raw);
	raw.c_iflag &= ~(IXON); // disable CTRL-Q (and CTRL-S)
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/*** Threads ***/
void *pthread_routine(void *arg) {
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int socket_fd = pthread_arg->socket_fd;
    struct sockaddr_in client_address = pthread_arg->client_address;

    free(arg);
	
	printf("Connected.\n");
	int recv_status;
	char message_reply[BUFFER] = {};
	while(1){
		recv_status = recv(socket_fd, &message_reply, BUFFER, 0);
		if(strlen(message_reply) == 0) continue;
		if(recv_status <= 0) break;
		printf("%d ", strlen(message_reply));
		puts(message_reply);
		fflush(stdout);
		bzero(message_reply, BUFFER);
	}
	printf("Lost connection to the server.\n");
    close(socket_fd);
	
    return NULL;
}

/*** Communication ***/
int sendall(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
	
	struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
	
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

