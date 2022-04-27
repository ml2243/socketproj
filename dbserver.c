#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>	//open

#include "msg.h"

#define MAX_ID_DIGITS 10

void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);
int  Listen(char *portnum, int *sock_family);
void HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen,
                  int sock_family, int32_t db_fd);

int put(int32_t db_fd, struct record rd);
int get(int32_t db_fd, struct record *rd_ptr);
int del(int32_t db_fd, struct record *rd_ptr);

int 
main(int argc, char **argv) {
	// Expect the port number as a command line argument.
	if (argc != 2) {
		Usage(argv[0]);
	}
	
	int sock_family;
	int listen_fd = Listen(argv[1], &sock_family);
	if (listen_fd <= 0) {
		// We failed to bind/listen to a socket.  Quit with failure.
		printf("Couldn't bind to any addresses.\n");
		return EXIT_FAILURE;
	}
	
	//create and open a new file called "database"
	int32_t db_fd = open("database", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (db_fd == -1){
		perror("open failed");
		exit(EXIT_FAILURE);
	}

	// Loop forever, accepting a connection from a client and doing
	// an echo trick to it.
	while (1) {
		struct sockaddr_storage caddr;
		socklen_t caddr_len = sizeof(caddr);
		int client_fd = accept(listen_fd,
		                       (struct sockaddr *)(&caddr),
		                       &caddr_len);
		if (client_fd < 0) {
			if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
				continue;
			printf("Failure on accept:%s \n ", strerror(errno));
			break;
		}
		
		HandleClient(client_fd,
		             (struct sockaddr *)(&caddr),
		             caddr_len,
		             sock_family, db_fd);
	}
	
	// Close socket
	close(listen_fd);
	return EXIT_SUCCESS;
}

void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void 
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void 
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
	printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
   }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}

void 
HandleClient(int c_fd, struct sockaddr *addr, size_t addrlen,
                  int sock_family, int32_t db_fd) {
	// Print out information about the client.
	printf("\nNew client connection \n" );
	PrintOut(c_fd, addr, addrlen);
	PrintReverseDNS(addr, addrlen);
	PrintServerSide(c_fd, sock_family);
	
	// Loop, reading data and echo'ing it back, until the client
	// closes the connection.
	while (1) {
		struct msg m;
		
		//read from client
		ssize_t res = read(c_fd, &m, sizeof m);
		
		if (res == 0) {
			printf("[The client disconnected.] \n");
			break;
		}
	
		else if (res == -1) {
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
	
			printf(" Error on client socket:%s \n ", strerror(errno));
			break;
		}

		if (m.type == PUT){
			if (put(db_fd, m.rd) == -1)
				m.type = FAIL;
			else
				m.type = SUCCESS;
		}

		else if (m.type == GET){
			int get_result = get(db_fd, &(m.rd));	
			if (get_result == 1)
				m.type = SUCCESS;
			else
				m.type = FAIL;
		}

		else if (m.type == DEL){
			if (del(db_fd, &(m.rd)) == -1)
				m.type = FAIL;
			else
				m.type = SUCCESS;
		}

		//write to client
		while(1) {
			ssize_t wres = write(c_fd, &m, sizeof m);

			if (wres == 0){
				printf("socket closed prematurely \n");
				close(c_fd);
				return;
			}
			else if (wres == -1){
				if (errno == EINTR)
					continue;

				printf("socket write failure \n");
				close(c_fd);
				return;
			}
			break;
		}
	}

	close(c_fd);
}

//delete a record from file
//return 0 if del was successful
//return -1 if del was unsuccessful
int del(int32_t db_fd, struct record *rd_ptr){
	char buf[sizeof *rd_ptr] = {0};	//array the size of record with '\0' bytes

	off_t eof = lseek(db_fd, 0, SEEK_END);	//offset at end of file 
	struct record temp;

	//traverse through the file
	//i = current offset
	for (off_t i = 0; i < eof; i += sizeof temp){
	
		//adjust file offset
		if (lseek(db_fd, i, SEEK_SET) == -1){
			return -1;
		}
		
		ssize_t res = read(db_fd, &temp, sizeof temp);

		if (res == 0)
			continue;
	
		else if (res == -1)
			return -1;

		if(temp.id == rd_ptr -> id){
			if (lseek(db_fd, i, SEEK_SET) == -1){
				return -1;
			}
			if (write(db_fd, buf, sizeof buf) != sizeof buf){
				printf("Server could not delete record");
				return -1;
			}
			strcpy(rd_ptr -> name, temp.name);
			return 0;
		}
	} 

	return -1;
}

//get record from database
//return 0 if get was successful but none matched
//return 1 if get was successful and one matched
//return -1 if get failed
int get(int32_t db_fd, struct record *rd_ptr){
	off_t eof = lseek(db_fd, 0, SEEK_END);	//offset at end of file 
	struct record temp;

	//traverse through the file
	//i = current offset
	for (off_t i = 0; i < eof; i += sizeof temp){
	
		//adjust file offset
		if (lseek(db_fd, i, SEEK_SET) == -1){
			return -1;
		}
		
		ssize_t res = read(db_fd, &temp, sizeof temp);

		if (res == 0)
			continue;
	
		else if (res == -1)
			return -1;

		if(temp.id == rd_ptr -> id){
			strcpy(rd_ptr -> name, temp.name);
			return 1;
		}
	} 

	return 0;
}

//write to database
//return 0 if put was successful and -1 if error
int put(int32_t db_fd, struct record rd){
	struct record temp;
	off_t eof = lseek(db_fd, 0, SEEK_END);	//eof is at end of file
	
	if (eof == 0){	//a brand new file
		if (write(db_fd, &rd, sizeof rd) != sizeof rd)
			return -1;
		return 0;
	}

	//...file has data in it...

	//traverse through file
	//i = current offset
	for (off_t i = 0; i < eof; i += sizeof temp){

		//adjust file offset to current offset
		if (lseek(db_fd, i, SEEK_SET) == -1)
			return -1;
		
		//read the section
		if (read(db_fd, &temp, sizeof temp) == -1)
			return -1;
	
		//a hole in file
		if (strlen(temp.name) == 0){
			//adjust file offset to before read
			if (lseek(db_fd, i, SEEK_SET) == -1)
				return -1;
			
			if (write(db_fd, &rd, sizeof rd) != sizeof rd)
				return -1;
			return 0;
		}
	
	}

	//...no hole in file. Currently at end of file...
	
	//write to end of file
	if (write(db_fd, &rd, sizeof rd) != sizeof rd)
		return -1;

//	lseek(db_fd, 0, SEEK_END);	//move file offset to end of file
//	if (write(db_fd, &rd, sizeof rd) != sizeof rd)
//		return -1;
	return 0;
}
