#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <inttypes.h>	//SCNd8

#include "msg.h"

#define BUF 256
#define MAX_ID_DIGITS 10	//record id uint32_t can have up to 10 digits

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);

void put(int32_t socket_fd);
void get(int32_t socket_fd);
void del(int32_t socket_fd);

int 
main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

	int8_t choice;

	while(1){
		printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): ");
		scanf("%" SCNd8 "%*c", &choice);
		
		if (choice == PUT)
			put(socket_fd);
		
		else if (choice == GET)
			get(socket_fd);
		
		else if (choice == DEL)
			del(socket_fd);

		else if (choice == 0){
			printf("Exit\n");
			break;
		}

//		else
//			printf("Invalid choice. Please try again\n");
	}

  // Clean up.
  close(socket_fd);
  return EXIT_SUCCESS;
}

void del(int32_t socket_fd){
	struct msg m;
	m.type = DEL;

	printf("Enter the id: ");

	char buf[MAX_ID_DIGITS + 2];	//+2 for '\n' and '\0'
	fgets(buf, MAX_ID_DIGITS + 2, stdin);

	buf[strlen(buf) - 1] = '\0';
	m.rd.id = atoi(buf);


	//write to server
	while(1){
		ssize_t wres = write(socket_fd, &m, sizeof m);

		if (wres == 0) {
   			printf("socket closed prematurely \n");
   			close(socket_fd);
   			exit(EXIT_FAILURE);
   		}
   		else if (wres == -1) {
   			if (errno == EINTR)
   		     		continue;
   		   	
			printf("socket write failure \n");
   		   	close(socket_fd);
   		   	exit(EXIT_FAILURE);
   		}
		else if (wres != sizeof m){
			printf("Write to server failed. Please try again. \n");
			return;
		}
 		break;
	}
	printf("\n");

	//read from server
	while(1){
    		ssize_t res = read(socket_fd, &m, sizeof m);
    		if (res == 0) {
    			printf("socket closed prematurely \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}
    		if (res == -1) {
    			if (errno == EINTR)
    				continue;
    			printf("socket read failure \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}
	
		if (m.type == SUCCESS){
			printf("name: %s \n", m.rd.name);
			printf("id: %u \n", m.rd.id);
		}

		else if (m.type == FAIL){
			printf("Delete failed. Please try again \n");
		}

		else
			printf("Delete read error. Please try again \n");
		
		break;
	}
	printf("\n");
}


void get(int32_t socket_fd){
	struct msg m;
	m.type = GET;

	printf("Enter the id: ");

	char buf[MAX_ID_DIGITS + 2];	//+2 for '\n' and '\0'	
	fgets(buf, MAX_ID_DIGITS + 2, stdin);

	buf[strlen(buf) - 1] = '\0';
	m.rd.id = atoi(buf);


	//write to server
	while(1){
		ssize_t wres = write(socket_fd, &m, sizeof m);

		if (wres == 0) {
   			printf("socket closed prematurely \n");
   			close(socket_fd);
   			exit(EXIT_FAILURE);
   		}
   		else if (wres == -1) {
   			if (errno == EINTR)
   		     		continue;
   		   	
			printf("socket write failure \n");
   		   	close(socket_fd);
   		   	exit(EXIT_FAILURE);
   		}
		else if (wres != sizeof m){
			printf("Write to server failed. Please try again. \n");
			return;
		}
 		break;
	}
	printf("\n");

	//read from server
	while(1){
    		ssize_t res = read(socket_fd, &m, sizeof m);
    		if (res == 0) {
    			printf("socket closed prematurely \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}
    		if (res == -1) {
    			if (errno == EINTR)
    				continue;
    			printf("socket read failure \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}
	
		if (m.type == SUCCESS){
			printf("name: %s \n", m.rd.name);
			printf("id: %u \n", m.rd.id);
		}

		else if (m.type == FAIL){
			printf("Get failed. Please try again\n");
		}

		else
			printf("Get read error. Please try again \n");
		
		break;
	}
	printf("\n");
}

void put(int32_t socket_fd){
	struct msg m;
	m.type = PUT;

	printf("Enter the name: ");

	fgets(m.rd.name, MAX_NAME_LENGTH, stdin);
	m.rd.name[strlen(m.rd.name) - 1] = '\0';
	
	printf("Enter the id: ");

	char buf[MAX_ID_DIGITS + 2];	//+2 for '\n' and '\0'	
	fgets(buf, MAX_ID_DIGITS + 2, stdin);

	buf[strlen(buf) - 1] = '\0';
	m.rd.id = atoi(buf);

//	printf("type: %u \n", m.type);
//	printf("name: %s \n", m.rd.name);
//	printf("id: %u \n", m.rd.id);

	//write to server
	while(1){
		ssize_t wres = write(socket_fd, &m, sizeof m);

		if (wres == 0) {
   			printf("socket closed prematurely \n");
   			close(socket_fd);
   			exit(EXIT_FAILURE);
   		}
   		else if (wres == -1) {
   			if (errno == EINTR)
   		     		continue;
   		   	
			printf("socket write failure \n");
   		   	close(socket_fd);
   		   	exit(EXIT_FAILURE);
   		}
		else if (wres != sizeof m){
			printf("Write to server failed. Please try again. \n");
			return;
		}
		break;
	}
	printf("\n");
	
	//read from server
   	while(1){
    		ssize_t res = read(socket_fd, &m, sizeof m);
    		if (res == 0) {
    			printf("socket closed prematurely \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}
    		if (res == -1) {
    			if (errno == EINTR)
    				continue;
    			printf("socket read failure \n");
    			close(socket_fd);
    			exit(EXIT_FAILURE);
    		}

		if (m.type == SUCCESS)
			printf("Put success. \n");

		else if (m.type == FAIL)
			printf("Put failed. Please try again \n");
		
		else
			printf("Put read error. Please try again \n");

		break;
	}
	printf("\n");
}



void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}
