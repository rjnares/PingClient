#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

/* Used "http://www.linuxhowtos.org/data/6/client_udp.c" for reference.
 * In particular, I copied the lines of code used to set up the socket.
 * 
 * These lines mainly include the setup and use of function: socket()
 * as well as the intermediate lines used to specify server address parameters. 
 */

#define BUFFER_SIZE 130 // max char input is 128; using buffer size 130 to detect if input exceeds 128 chars


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

  int client_socket, server_port_num;
  int num_bytes_recv, num_bytes_sent;
  socklen_t server_addr_len;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];

  /* Create client socket to use IPv4 communication domain and UDP connection */
  if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    error("ERROR on socket");
  }

  /* Zero out memory buffer for server address */
  bzero((char*) &server_addr, sizeof(server_addr));
  
  /* Set up server address semantics */
  server_port_num = atoi(argv[2]);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port_num);
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr_len = (socklen_t) sizeof(struct sockaddr_in);

  /* No connection needed for UDP; now try to get user input */
  printf("Enter string: ");
  bzero(buffer, sizeof(buffer));
  fgets(buffer, sizeof(buffer), stdin);

  /* Pass string to the server */
  num_bytes_sent = sendto(client_socket, buffer, sizeof(buffer),
			  0, (const struct sockaddr*) &server_addr, server_addr_len);

  if (num_bytes_sent == -1) {
    error("ERROR on sendto");
  }

  /* Wait for response from server */
  bzero(buffer, sizeof(buffer));
  num_bytes_recv = recvfrom(client_socket, buffer, sizeof(buffer),
			    0, (struct sockaddr*) &server_addr, &server_addr_len);

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


