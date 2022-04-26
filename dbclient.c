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

  // Read something from the remote host.
  // Will only read BUF-1 characters at most.
 //   char readbuf[BUF] = {'h', 'i', 'm', '\0'};
 //   int res = 4;
//  while (1) {
//    res = read(socket_fd, readbuf, BUF-1);
//    if (res == 0) {
//      printf("socket closed prematurely \n");
//      close(socket_fd);
//      return EXIT_FAILURE;
//    }
//    if (res == -1) {
//      if (errno == EINTR)
//        continue;
//      printf("socket read failure \n");
//      close(socket_fd);
//      return EXIT_FAILURE;
//    }
//    readbuf[res] = '\0';
//    printf("%s", readbuf);
//    break;
//  }

  // Write something to the remote host.
//  while (1) {
   // int wres = write(socket_fd, readbuf, res);
   // if (wres == 0) {
   //  printf("socket closed prematurely \n");
   //   close(socket_fd);
   //   return EXIT_FAILURE;
   // }
   // if (wres == -1) {
   //   if (errno == EINTR)
   //     continue;
   //   printf("socket write failure \n");
   //   close(socket_fd);
   //   return EXIT_FAILURE;
   // }

   // res = read(socket_fd, readbuf, BUF-1);
   // if (res == 0) {
   //   printf("socket closed prematurely \n");
   //   close(socket_fd);
   //   return EXIT_FAILURE;
   // }
   // if (res == -1) {
   //   if (errno == EINTR)
   //     continue;
   //   printf("socket read failure \n");
   //   close(socket_fd);
   //   return EXIT_FAILURE;
   // }
   // readbuf[res] = '\0';
   // printf("%s\n", readbuf);
   // break;
//  }

	while(1){
		put(socket_fd);
	}

  // Clean up.
  close(socket_fd);
  return EXIT_SUCCESS;
}

void put(int32_t socket_fd){

	//user_input = "msg.type" + "name + \0" + "id" + '\n' +'\0'
	char* user_input = (char*) malloc( (1 + MAX_NAME_LENGTH + MAX_ID_DIGITS + 2) * sizeof(char));
	*user_input = '1';

	printf("Enter the name: ");

	fgets(user_input + 1, MAX_NAME_LENGTH, stdin);
	
	user_input[strlen(user_input) - 1] = '\0';	//replace '\n' with '\0'
	int32_t ui_len = strlen(user_input) + 1; 	//add null byte

	printf("Enter the id: ");

	char* id_user_input = user_input + ui_len;
	
	fgets(id_user_input, MAX_ID_DIGITS + 2, stdin);
	id_user_input[strlen(id_user_input) - 1] = '\0';	//replace '\n' with '\0'
	
	//now id_user_input = "id" + '\0' + '\0'
	ui_len += strlen(id_user_input) + 1;	//+1 for 1 null byte

	//now ui_len = "msg.type" + "name + \0" + "id" + '\0'

//	printf("user input = = %s \n", user_input);
//	printf("id = %s \n", user_input + strlen(user_input) + 1);
//	printf("len of ui = %d \n", ui_len);

	while(1){
		int wres = write(socket_fd, user_input, ui_len);

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
		else if (wres != ui_len){
			printf("Incomplete write. Attempting write again.");
			continue;
		}
   

		char readbuf[BUF];
    		int res = read(socket_fd, readbuf, BUF-1);
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
    		readbuf[BUF - 1] = '\0';
    		printf("%s\n", readbuf);

		break;
	}
	return;
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
