#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 50

void error(const char* msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char* argv[]) {

    /* If no server IP address or port number is specified, then end process */
    if (argc != 3) {
        fprintf(stderr, "ERROR on input: Invalid number of inputs\n");
        exit(1);
    }

    int client_socket, server_port_num, seq_num;
    int num_bytes_recv, num_bytes_sent;
    socklen_t server_addr_len;
    struct sockaddr_in server_addr;
    struct hostent *addr;
    struct timeval tv;
    struct timespec timestamp, start, end;
    char buffer[BUFFER_SIZE];

    /* Create client socket to use IPv4 communication domain and UDP connection */
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        error("ERROR on socket");
    }

    /* Set socket timeout to 1 sec */
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        error("ERROR on socket options");
    }

    /* gethostbyname: get the server's DNS entry */
    addr = gethostbyname(argv[1]);
    if (addr == NULL) {
        fprintf(stderr,"ERROR: no such host\n");
        exit(1);
    }

    /* Zero out memory buffer for server address */
    bzero((char*) &server_addr, sizeof(server_addr));
  
    /* Set up server address semantics */
    server_port_num = atoi(argv[2]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_num);
    //server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    bcopy((char*) addr->h_addr, (char*) &server_addr.sin_addr.s_addr, addr->h_length);
    server_addr_len = (socklen_t) sizeof(struct sockaddr_in);

    seq_num = 1;

    while(1) {
        bzero(buffer, sizeof(buffer));
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp);
        


        if (sprintf(buffer, "PING %d %d", seq_num, timestamp) < 0) {
            fprintf(stderr,"ERROR: cannot write to buffer\n");
            exit(1);
        }

        /* Pass string to the server */
        num_bytes_sent = sendto(client_socket, buffer, sizeof(buffer), 
                        0, (const struct sockaddr*) &server_addr, server_addr_len);

    if (num_bytes_sent == -1) {
      error("ERROR on sendto");
    }

  /* Wait for response from server */
  bzero(buffer, sizeof(buffer));
  num_bytes_recv = recvfrom(client_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &server_addr, &server_addr_len);

  if (num_bytes_recv == -1) {
    error("ERROR on recvfrom");
  }

  /* If server could not compute, then end */
  if (strcmp(buffer, "Sorry, cannot compute!") == 0) {
    printf("From server: Sorry, cannot compute!\n");
  } else {
        
    /* Else continue until single digit received */
    while (1) {
      printf("From server: %s\n", buffer);
      
      if (strlen(buffer) == 1) {
	break;
      }

      bzero(buffer, sizeof(buffer));
      num_bytes_recv = recvfrom(client_socket, buffer, sizeof(buffer),
				0, (struct sockaddr*) &server_addr, &server_addr_len);
      
      if (num_bytes_recv == -1) {
	error("ERROR on recvfrom");
      }
    }
  }

  
  close(client_socket);

  return 0;
  
}


