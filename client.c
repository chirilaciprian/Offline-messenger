#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

extern int errno;
int port;
int main(int argc, char *argv[]) {

  int sd;                    // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare
  int nr = 0;
  char buf[100];
  bzero(buf, 100);
  /* exista toate argumentele in linia de comanda? */
  if (argc != 3) {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }
  /* stabilim portul */
  port = atoi(argv[2]);
  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  if (read(sd, buf, sizeof(buf)) < 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }
  /* afisam mesajul primit */
  printf("Mesajul primit este: %s\n", buf);

  while (1) {
    /* citirea mesajului */
    char buf[5000];
    bzero(buf, 5000);
    fflush(stdout);
    read(0, buf, sizeof(buf));
    buf[strlen(buf) - 1] = '\0';

    
    /* trimiterea mesajului la server */
    if (write(sd, buf, strlen(buf)) <= 0) {
      perror("[client]Eroare la write() spre server.\n");
      return errno;
    }

    // Verificam comanda quit
    if(strcmp(buf,"/quit")==0)
    {
      close(sd);
      exit(1);
    }
    
    bzero(buf, 5000);
    /* citirea raspunsului dat de server
       (apel blocant pina cind serverul raspunde) */
    if (read(sd, buf, sizeof(buf)) < 0) {
      perror("[client]Eroare la read() de la server.\n");
      return errno;
    }
    /* afisam mesajul primit */
    printf("%s\n", buf);
  }

  /* inchidem conexiunea, am terminat */
  close(sd);
}