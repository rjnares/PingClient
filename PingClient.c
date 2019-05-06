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
#include <stdint.h>
#include <errno.h>
#include <float.h>
#include <netdb.h>
#include <math.h>

#define BUFFER_SIZE 50
#define MAX_SEQ_NUM 10
#define S_MULT 1000 // convert s to ms
#define NS_DIV 1000000 // convert ns to ms

void error(const char* msg) {
  perror(msg);
  exit(1);
}

double min(double a, double b) {
  if (a < b) return a;
  else return b;
}

double max(double a, double b) {
  if (a > b) return a;
  else return b;
}

double avg(double *vals, int total) {
  int i;
  double avg = 0;
  for (i = 0; i < MAX_SEQ_NUM; i++) {
    avg += (vals[i] / total);
  }
  avg = round(avg * 1000) / 1000;
  return avg;
}

int main(int argc, char* argv[]) {
  
  /* If no server IP address or port number is specified, then end process */
  if (argc != 3) {
    fprintf(stderr, "ERROR on input: Invalid number of inputs\n");
    exit(1);
  }
  
  int client_socket, server_port_num, num_bytes_recv, num_bytes_sent;
  int seq_num, num_packets_sent, num_packets_recv;
  socklen_t server_addr_len;
  struct sockaddr_in server_addr;
  struct hostent *host_entry;
  struct timeval timeout;
  struct timespec timestamp, start, end;
  double timestamp_ms, start_ms, end_ms, rtt_ms, min_rtt, avg_rtt, max_rtt, packet_loss;
  double rtt[MAX_SEQ_NUM] = {0};
  char send_buffer[BUFFER_SIZE];
  char recv_buffer[BUFFER_SIZE];
  
  /* Create client socket to use IPv4 communication domain and UDP connection */
  if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    error("ERROR on socket");
  }
  
  /* Set socket timeout to 1 sec */
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
    error("ERROR on socket options");
  }

  /* gethostbyname: get the server's DNS entry */
  host_entry = gethostbyname(argv[1]);
  if (host_entry == NULL) {
    fprintf(stderr,"ERROR: no such host\n");
    exit(1);
  }
  
  /* Zero out memory buffer for server address */
  bzero((char*) &server_addr, sizeof(server_addr));
  
  /* Set up server address semantics */
  server_port_num = atoi(argv[2]);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port_num);
  //server_addr.sin_addr.s_addr = inet_addr(argv[1]); only works for having IP address directly
  bcopy((char*) host_entry->h_addr, (char*) &server_addr.sin_addr.s_addr, host_entry->h_length);
  server_addr_len = (socklen_t) sizeof(struct sockaddr_in);

  num_packets_sent = num_packets_recv = 0;
  min_rtt = DBL_MAX;
  max_rtt = DBL_MIN;
  
  for (seq_num = 1; seq_num < 11; seq_num++) {
    bzero(send_buffer, sizeof(send_buffer));
    bzero(recv_buffer, sizeof(recv_buffer));
    
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) == -1) {
      error("ERROR on timestamp");
    }
    
    timestamp_ms = ((double) timestamp.tv_sec * S_MULT) + ((double) timestamp.tv_nsec / NS_DIV);
    timestamp_ms = round(timestamp_ms * 1000) / 1000;

    /* timestamp is simply the result of clock_gettime */
    if (sprintf(send_buffer, "PING %d %lf ms\n", seq_num, timestamp_ms) < 0) {
      fprintf(stderr,"ERROR: cannot write to send buffer\n");
      exit(1);
    }
    
    /* Start timer right before sending */
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) { 
      error("ERROR on timer start");
    }    
    num_bytes_sent = sendto(client_socket, send_buffer, sizeof(send_buffer), 
			    0, (const struct sockaddr*) &server_addr, server_addr_len);  
    if (num_bytes_sent == -1) {
      error("ERROR on sendto");
    }
    num_packets_sent++;
    num_bytes_recv = recvfrom(client_socket, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*) &server_addr, &server_addr_len);
    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
      error("ERROR on timer end");
    }
    /* End timer right after receiving */
    
    /* If timeout and packet loss occurs, then move onto next iteration */
    if (num_bytes_recv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      //printf("TIMEOUT\n");
      continue;
    }
    /* If some error occurs during receive operation, then quit */
    else if (num_bytes_recv == -1) {
      error("ERROR on recvfrom");
    }
    else if (num_bytes_recv == 0) {
      error("ERROR no messages available or peer has performed an orderly shutdown");
    }
    /* If reply received before timeout, then calculate and store the RTT */ 
    else if (num_bytes_sent == num_bytes_recv && (strcmp(send_buffer, recv_buffer) == 0)) {
      num_packets_recv++;
      start_ms = ((double) start.tv_sec * S_MULT) + ((double) start.tv_nsec / NS_DIV);
      start_ms = round(start_ms * 1000) / 1000;
      end_ms = ((double) end.tv_sec * S_MULT) + ((double) end.tv_nsec / NS_DIV);
      end_ms = round(end_ms * 1000) / 1000;
      rtt_ms = end_ms - start_ms; 
      printf("PING received from %s: seq#=%d time=%.3lf ms\n", argv[1], seq_num, rtt_ms); 
      rtt[seq_num - 1] = rtt_ms;
      min_rtt = min(min_rtt, rtt_ms);
      max_rtt = max(max_rtt, rtt_ms);
      sleep(1);
    }
  }

  packet_loss = ((double)(MAX_SEQ_NUM - num_packets_recv)) / MAX_SEQ_NUM;
  avg_rtt = avg(rtt, num_packets_recv);
  printf("--- ping statistics --- %d packets transmitted, %d received, %d%% packet loss rtt min/avg/max = %.3lf %.3lf %.3lf ms\n",
  	 num_packets_sent, num_packets_recv, (int)(packet_loss *100), min_rtt, avg_rtt, max_rtt);
  close(client_socket);

  return 0;
  
}


