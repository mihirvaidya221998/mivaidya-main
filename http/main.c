#include <stdio.h>
#include <string.h>

void send_http(char* host, char* msg, char* resp, size_t len);


/*
  Implement a program that takes a host, verb, and path and
  prints the contents of the response from the request
  represented by that request.
 */
void build_http_req(char* finRequest, const char* requestVerb, const char* requestPath, const char* requestHost){
  snprintf(finRequest, 4096,
  "%s %s %s\r\n"
  "Host: %s\r\n"
  "\r\n\r\n",
  requestVerb, requestPath, "HTTP/1.1", requestHost);
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Invalid arguments - %s <host> <GET|POST> <path>\n", argv[0]);
    return -1;
  }
  char* host = argv[1];
  char* verb = argv[2];
  char* path = argv[3];

  /*
    STUDENT CODE HERE
   */

  char finReq[4096];
  char httpRes[4096];
  build_http_req(finReq, verb, path, host);
  printf("Formatted HTTP Request:\n%s\n", finReq);

  send_http(host, finReq, httpRes, 4096);
  printf("%s\n", httpRes);
  
  return 0;
}
