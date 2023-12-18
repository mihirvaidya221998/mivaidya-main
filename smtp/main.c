#include <stdio.h>
#include <string.h>

int connect_smtp(const char* host, int port);
void send_smtp(int sock, const char* msg, char* resp, size_t len);



/*
  Use the provided 'connect_smtp' and 'send_smtp' functions
  to connect to the "lunar.open.sice.indian.edu" smtp relay
  and send the commands to write emails as described in the
  assignment wiki.
 */
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Invalid arguments - %s <email-to> <email-filepath>", argv[0]);
    return -1;
  }

  char* rcpt = argv[1];
  char* filepath = argv[2];

  //First we will connect to SMTP server
  int smtpSocket = connect_smtp("lunar.open.sice.indiana.edu", 25);
  if(smtpSocket < 0){
    perror("FAILED TO CONNECT SMTP PLEASE TRY AGAIN!!!!");
    return -1;
  }

  char res[4096];
  send_smtp(smtpSocket, "HELO iu.edu\n", res, 4096);
  printf("%s\n", res);

  // //Clearing the memory
  char reqMsg[4096];
  // memset(reqMsg, 0, sizeof(reqMsg));
  // memset(res, 0, sizeof(res));

  //Here we are Constructing an email
  snprintf(reqMsg, sizeof(reqMsg), "MAIL FROM: <%s>\r\n", rcpt);
  send_smtp(smtpSocket, reqMsg, res, sizeof(res));
  printf("%s\n",res);

  //Resetting once more
  memset(reqMsg, 0, sizeof(reqMsg));
  memset(res, 0, sizeof(res));

  //Here we are constructing the recepient side
  snprintf(reqMsg, sizeof(reqMsg), "RCPT To: <%s>\r\n", rcpt);
  send_smtp(smtpSocket, reqMsg, res, sizeof(res));
  printf("%s\n", res);

  memset(reqMsg, 0, sizeof(reqMsg));
  memset(reqMsg, 0, sizeof(res));

  //Sending the Body of the email
  send_smtp(smtpSocket, "DATA\r\n", res, sizeof(res));
  printf("%s\n", res);

  //Reading the file
  FILE* filePath = fopen(filepath, "r");
  if(filePath != NULL){
    //Reading and sending the email body
    while(fgets(reqMsg, sizeof(reqMsg), filePath) != NULL){
      send_smtp(smtpSocket, reqMsg, res, sizeof(res));
    }
    fclose(filePath);
  }
  
  //Ending the email message
  send_smtp(smtpSocket, "\r\n.\r\n", res, sizeof(res));
  printf("%s\n",res);

  //Quitting the SMTP session
  send_smtp(smtpSocket,"QUIT\r\n", res, sizeof(res));

  return 0;
}
