/*
** server.c -- a stream socket server
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
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>

/*** Headers ***/
#include "enigma.h"

/*** Defines ***/
#define BUFFER 2048
#define QUEUE_LIMIT 10
#define check(expr) if (!(expr)) { perror(#expr); kill(0, SIGTERM); }

/*** Data ***/
typedef struct pthread_arg_t {
    int accepted_fd;
    struct sockaddr_in client_address;
	struct Enigma *machine;
} pthread_arg_t;

struct Rotor new_rotor(struct Enigma *machine, int rotornumber, int offset) {
    struct Rotor r;
    r.offset = offset;
    r.turnnext = 0;
    r.cipher = rotor_ciphers[rotornumber - 1];
    r.turnover = rotor_turnovers[rotornumber - 1];
    r.notch = rotor_notches[rotornumber - 1];
    machine->numrotors++;

    return r;
}

/*** Declarations ***/
int sendall(int s, char *buf, int *len);
void *pthread_routine(void *arg);
void signal_handler(int signal_number);
void init_enigma(struct Enigma *machine);
char encryptChar(char c, struct Enigma *machine);

/*** Init ***/
int main(int argc, char *argv[]){
	if(argc != 2) {
        printf("Invalid number of arguments, program usage: ./server port");
        return 1;
    }
	
	printf("l");
	
	int port;
	if((port = atoi(argv[1])) == 0 || port < 49152 || port > 65535){
        printf("Offset can only be a positive integer (49152 - 65535)");
		return 1;
    }
	
    int status;
    struct addrinfo hints, *servinfo;
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        // unspecified type of Address Family (to force: AF_INET or AF_INET6)
    hints.ai_socktype = SOCK_STREAM;    // we tell it to use TCP, to use UDP it's SOCK_DGRAM
    hints.ai_flags = AI_PASSIVE;        // this tells getaddrinfo() to assign the address of my local host to the socket structures.
                                        //   alternatively, use specific address in getaddrinfo(<address>, _, _), for example below instead of NULL
	
	status = getaddrinfo(NULL, argv[1], &hints, &servinfo);
	check(status == 0);

    int socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	check(socket_fd != -1)
	
	status = bind(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen);
	check(status != -1);

	status = listen(socket_fd, QUEUE_LIMIT);
    check(status != -1);
	
	check(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
	check(signal(SIGTERM, signal_handler) != SIG_ERR);
	check(signal(SIGINT, signal_handler) != SIG_ERR);
	
    pthread_attr_t pthread_attr;
	check(pthread_attr_init(&pthread_attr) == 0);
	check(pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) == 0);
	
	struct Enigma machine = {}; // initialized to defaults
	init_enigma(&machine);
	
    pthread_arg_t *pthread_arg;
    socklen_t client_address_len;
	int accepted_fd;
	pthread_t pthread;
	
	printf("Server started.");
	fflush(stdout);
	
	while(1){
		pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);
        if (!pthread_arg) {
            perror("malloc");
            continue;
        }
		
		client_address_len = sizeof pthread_arg->client_address;
        accepted_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        if (accepted_fd == -1) {
            perror("accept");
            free(pthread_arg);
            continue;
        }
		
		pthread_arg->accepted_fd = accepted_fd;
		
		pthread_arg->machine = &machine;
		
		if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0) {
            perror("pthread_create");
            free(pthread_arg);
            continue;
        }
		
	}
	
	freeaddrinfo(servinfo);
	
    return 0;
}

void init_enigma(struct Enigma *machine){
	machine->reflector = reflectors[1];
    machine->rotors[0] = new_rotor(machine, 3, 0);
    machine->rotors[1] = new_rotor(machine, 2, 0);
    machine->rotors[2] = new_rotor(machine, 1, 0);
}

/*** Threads ***/
void *pthread_routine(void *arg) {
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int accepted_fd = pthread_arg->accepted_fd;
    struct sockaddr_in client_address = pthread_arg->client_address;
	
	struct Enigma *machine = pthread_arg->machine;
	
    free(arg);
	
	printf("\nClient connected.");
	fflush(stdout);

	char c = '\0';
	int send_limit = 0;
	int recv_status, send_status;
	char message[BUFFER] = {};
	int bytesleft;
	
	struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(accepted_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(accepted_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
    while(1){
		recv_status = recv(accepted_fd, &message, BUFFER, 0);
		if(recv_status == -1){
			send_status = send(accepted_fd, &c, 1, 0);
			
			if(send_status == -1) send_limit++;
			else send_limit = 0;
			
			if(send_limit > 2) break;
			
			continue;
		}
		if(recv_status == 0) break;
		
		send_limit = 0;
		
		bytesleft = strlen(message);
		
		if(message[0] == '\0') continue;
		
		for(int i = 0; i < strlen(message); i++){
			if(isalpha(message[i]))
				message[i] = encryptChar(message[i], machine);
		}
		
		sendall(accepted_fd, message, &bytesleft);
		bzero(message, BUFFER);
	}
	
	printf("\nClient disconnected.");
	fflush(stdout);
	
    close(accepted_fd);
	
    return NULL;
}

/*** Communication ***/
int sendall(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

/*** Encryption ***/
char encryptChar(char c, struct Enigma *machine){
	
    c = toupper(c);
		
	// Plugboard
    int req_index = str_index(alpha, c);
		
	// Cycle first rotor before pushing through,
    rotor_cycle(&machine->rotors[0]);
		
	// Double step the rotor
    if(str_index(machine->rotors[1].notch,
                alpha[machine->rotors[1].offset]) >= 0 ) {
        rotor_cycle(&machine->rotors[1]);
    }
		
    // Stepping the rotors
	for(int i = 0; i < machine->numrotors - 1; i++) {
        c = alpha[machine->rotors[i].offset];

        if(machine->rotors[i].turnnext) {
            machine->rotors[i].turnnext = 0;
            rotor_cycle(&machine->rotors[i+1]);
        }
    }
		
	// Pass through all the rotors forward
    for(int i = 0; i < machine->numrotors; i++) {
        req_index = rotor_forward(&machine->rotors[i], req_index);
    }
		
	// Pass through the reflector
	// Inbound
    c = machine->reflector[req_index];
    // Outbound
    req_index = str_index(alpha, c);
		
	// Pass back through the rotors in reverse
    for(int i = machine->numrotors - 1; i >= 0; i--) {
        req_index = rotor_reverse(&machine->rotors[i], req_index);
    }
		
	// Pass through Plugboard
	c = alpha[req_index];
		
	return c;
}

/*** Signals ***/
void signal_handler(int signal_number) {
    exit(0);
}