#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

struct utilizator {
  char name[50];
  char parola[50];
  int online;
};

typedef struct thData {
  int idThread;     // id-ul thread-ului tinut in evidenta de acest program
  int cl;           // descriptorul intors de accept
  char u_name[100]; // numele userului conectat
} thData;

void Register(struct thData *t);
void Reply(struct thData *tdL, char name[], char message[]);
void History(struct thData *t, char name[]);
void Send(struct thData *t, char name[], char mesaj[]);
void Login(struct thData *t);
void Menu(struct thData *t);
static void *treat(void *);
struct utilizator users[100];
int nr_utilizatori;
int main() {
  // Deschidem fisierul text cu utilizatorii

  FILE *fp = fopen("users.txt", "a+");
  if (fp == NULL) {
    perror("Unable to open file!");
    return 1;
  }

  // Punem utilizatorii din fisier intr-o structura de date

  char tmp[100];
  int i = 0;
  while (fgets(tmp, sizeof(tmp), fp) != NULL) {
    char *p = strtok(tmp, " ");
    strcpy(users[i].name, p);
    p = strtok(NULL, " ");
    p[strlen(p) - 1] = '\0';
    strcpy(users[i].parola, p);
    // users[i].id = i;  ???
    users[i].online = 0;
    i++;
  }
  nr_utilizatori = i;
  fclose(fp);
  struct sockaddr_in server;
  struct sockaddr_in from;
  int sd;
  pthread_t th[100];
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("[server]Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
    perror("[server]Eroare la bind().\n");
    return errno;
  }
  if (listen(sd, 2) == -1) {
    perror("[server]Eroare la listen().\n");
    return errno;
  }
  while (1) {
    int client;
    thData *td; // parametru functia executata de thread
    int length = sizeof(from);

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0) {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    /* s-a realizat conexiunea, se astepta mesajul */

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);
  }
}

static void *treat(void *arg) {
  struct thData tdL;
  tdL = *((struct thData *)arg);
  fflush(stdout);
  pthread_detach(pthread_self());
  // Login(&tdL);
  Menu(&tdL);
  close((intptr_t)arg);
  return (NULL);
};

void Menu(struct thData *t) {
  char comanda[1000];
  int logat = 0;
  ;
  char meniu[1000], words[10][1000];
  strcpy(
      meniu,
      "/ login - pentru conectare \n / register - pentru inregistrare\n "
      "/send<User><Mesaj> -trimite un mesaj unui utilizator\n / reply <user> "
      "<Mesaj Repply> <Mesaj>- Raspunde la un anumit mesaj\n/history <User> - "
      "Conversatia cu un utilizator\n /new - Mesajele necitite \n/quit - "
      "Pentru a iesi \n/onusers - Pentru a vedea utilizatorii online\n/logout "
      "- Pentru deconectare");
  if (write(t->cl,
            "Bine ati venit la Offline messenger, tastati /menu pentru a vedea "
            "comenzile.",
            strlen("Bine ati venit la Offline messenger, tastati /menu pentru "
                   "a vedea comenzile.")) <= 0)
    perror("eroare la write Mesaj Afisare");
  while (1) {
    bzero(comanda, 1000);
    if (read(t->cl, comanda, sizeof(comanda)) <= 0) {
      perror("Eroare la read Meniu");
    }
    // Comanda pentru menu

    if (strcmp(comanda, "/menu") == 0) {
      if (write(t->cl, meniu, strlen(meniu) + 1) <= 0) {
        perror("Eroare la write Login");
      }
    }

    // Comanda pentru register

    if (strcmp(comanda, "/register") == 0) {
      if (logat == 1) {
        if (write(t->cl, "Sunteti logat deja", strlen("Sunteti logat deja")) <=
            0)
          perror("Eroare la write verificare logat");
      } else if (logat == 0) {
        Register(t);
        logat = 1;
      }
    }

    // Comanda pentru login

    if (strcmp(comanda, "/login") == 0) {
      int ok = 0;
      for (int i = 0; i < nr_utilizatori; i++)
        if (strcmp(t->u_name, users[i].name) == 0) {
          ok = 1;
        }
      if (ok == 0) {
        Login(t);
        logat = 1;
      } else if (ok == 1) {
        if (write(t->cl, "Sunteti logat deja", strlen("Sunteti logat deja")) <=
            0) {
          perror("Eroare la write login");
        }
      }
    }

    // Comanda pentru send

    if (strncmp(comanda, "/send ", 6) == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0) {
          perror("Eroare la write verificare logat");
        }
      }
      if (logat == 1) {
        int ok = 0;
        int i;
        for (i = 0; i < nr_utilizatori; i++) {
          if (strncmp(comanda + 6, users[i].name, strlen(users[i].name)) == 0) {
            ok = 1;
            break;
          }
        }
        if (ok == 1) {
          Send(t, users[i].name, comanda + 7 + strlen(users[i].name));
          if (write(t->cl, "Mesajul a fost trimis",
                    strlen("Mesajul a fost trimis")) <= 0) {
            perror("Eroare la write client [Send]");
          }
        }
        if (ok == 0) {
          if (write(t->cl, "Comanda invalida", strlen("Comanda invalida")) <=
              0) {
            perror("Eroare la write Login");
          }
        }
      }
    }
    // Comanda history pentru a vedea conversatia
    if (strncmp(comanda, "/history ", 9) == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0)
          perror("Eroare la write verificare logat");
      }
      if (logat == 1) {
        int ok = 0;
        int i;
        for (i = 0; i < nr_utilizatori; i++) {
          if (strncmp(comanda + 9, users[i].name, strlen(users[i].name)) == 0) {
            ok = 1;
            break;
          }
        }
        if (ok == 1) {
          History(t, users[i].name);
        } else if (ok == 0) {
          if (write(t->cl, "Comanda invalida history",
                    strlen("Comanda invalida history")) <= 0) {
            perror("Eroare la write History");
          }
        }
      }
    }
    // comanda unread messages
    if (strcmp(comanda, "/new") == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0)
          perror("Eroare la write verificare logat");
      }
      if (logat == 1) {
        char f_name[100], buf[1000];
        bzero(buf, 1000); // Nu e neaparat necesar?
        sprintf(f_name, "%s_unread.txt", t->u_name);
        FILE *fd = fopen(f_name, "a+");
        fread(buf, sizeof(buf), 1, fd);
        if (strlen(buf) != 0)
          if (write(t->cl, buf, strlen(buf)) <= 0) {
            perror("Eroare la write client [New]");
          }
        if (strlen(buf) == 0) {
          if (write(t->cl, "Nu aveti mesaje necitite",
                    strlen("Nu aveti mesaje necitite")) <= 0) {
            perror("Eroare la write client [New]");
          }
        }
        fclose(fd);
        fclose(fopen(f_name, "w"));
      }
    }
    // Comanda reply
    if (strncmp(comanda, "/reply ", 7) == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0)
          perror("Eroare la write verificare logat");
      }
      if (logat == 1) {
        int ok = 0;
        int i;
        for (i = 0; i < nr_utilizatori; i++) {
          if (strncmp(comanda + 7, users[i].name, strlen(users[i].name)) == 0) {
            ok = 1;
            break;
          }
        }
        if (ok == 1) {
          Reply(t, users[i].name, comanda + 8 + strlen(users[i].name));

        } else if (ok == 0) {
          if (write(t->cl, "Comanda incorecta reply",
                    strlen("Comanda incorecta reply")) <= 0) {
            perror("Eroare la write reply");
          }
        }
      }
    }
    // Comanda on users

    if (strcmp(comanda, "/onusers") == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0)
          perror("Eroare la write verificare logat");
      }
      if (logat == 1) {
        char tmp[200];
        bzero(tmp, 200);
        for (int i = 0; i < nr_utilizatori; i++) {
          if (users[i].online == 1) {
            strcat(tmp, users[i].name);
            strcat(tmp, "\n");
          }
        }
        if (write(t->cl, tmp, strlen(tmp)) <= 0) {
          perror("Eroare la write onusers");
        }
      }
    }
    // Comanda pentru quit
    if (strcmp(comanda, "/quit") == 0) {
      for (int i = 0; i < nr_utilizatori; i++)
        if (strcmp(users[i].name, t->u_name) == 0) {
          users[i].online = 0;
        }
      close(t->cl);
      break;
    }

    // Comanda pentru logout
    if (strcmp(comanda, "/logout") == 0) {
      if (logat == 0) {
        if (write(t->cl, "Nu sunteti logat", strlen("Nu sunteti logat")) <= 0)
          perror("Eroare la write verificare logat");
      }
      if (logat == 1) { 
        for (int i = 0; i < nr_utilizatori; i++)
          if (strcmp(t->u_name, users[i].name) == 0)
            users[i].online = 0;
            logat = 0;
            bzero(t->u_name,100);
        if (write(t->cl, "Deconectare reusita",
                  strlen("Deconectare reusita")) <= 0) {
          perror("Eroare la write Login");
          
        }
      }
    }
    // end if conectat

    // Comanda invalida
    else if (!(strcmp(comanda, "/menu") == 0 ||
               strncmp(comanda, "/send ", 6) == 0 ||
               strcmp(comanda, "/new") == 0 ||
               strncmp(comanda, "/history ", 9) == 0 ||
               strcmp(comanda, "/onusers") == 0 ||
               strcmp(comanda, "/quit") == 0 ||
               strcmp(comanda, "/login") == 0 ||
               strcmp(comanda, "/register") == 0 ||
               strncmp(comanda, "/reply ", 7) == 0 ||
               strcmp(comanda, "/logout") == 0)) {
      if (write(t->cl, "Comanda invalida", strlen("Comanda invalida")) <= 0) {
        perror("Eroare la write Login");
      }
    }
  } // end while
}

void Send(struct thData *tdL, char name[], char message[]) {
  char f_name[100], buff[2000];
  if (strcmp(name, tdL->u_name) < 0)
    sprintf(f_name, "%s_%s_conversatie.txt", name, tdL->u_name);
  else
    sprintf(f_name, "%s_%s_conversatie.txt", tdL->u_name, name);
  FILE *fp = fopen(f_name, "a");
  sprintf(buff, "[%s]%s\n", tdL->u_name, message);
  fputs(buff, fp);
  fclose(fp);

  // Cream fisierul cu mesajele necitite in cazul in care utilizatorul este
  // offline

  for (int i = 0; i < nr_utilizatori; i++) {
    if (strcmp(users[i].name, name) == 0) {
      char tmp_buf[100];
      sprintf(tmp_buf, "%s_unread.txt", name);
      FILE *f_new = fopen(tmp_buf, "a");
      fputs(buff, f_new);
      fclose(f_new);
    }
  }
}

void History(struct thData *tdL, char name[]) {
  char f_name[100], buf[5000];
  if (strcmp(name, tdL->u_name) < 0)
    sprintf(f_name, "%s_%s_conversatie.txt", name, tdL->u_name);
  else
    sprintf(f_name, "%s_%s_conversatie.txt", tdL->u_name, name);
  FILE *fp = fopen(f_name, "r");
  fread(buf, sizeof(buf), 1, fp);
  if (write(tdL->cl, buf, strlen(buf)) <= 0) {
    perror("Eroare la write client [History]");
  }
  fclose(fp);
}

void Reply(struct thData *tdL, char name[], char message[]) {
  char f_name[100];
  int ok = 0;
  char buf[1000];
  char tmp[1010];
  if (strcmp(name, tdL->u_name) < 0)
    sprintf(f_name, "%s_%s_conversatie.txt", name, tdL->u_name);
  else
    sprintf(f_name, "%s_%s_conversatie.txt", tdL->u_name, name);
  FILE *fp = fopen(f_name, "r");
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    buf[strlen(buf) - 1] = '\0';
    if (strncmp(message, buf + strlen(name) + 2,
                strlen(buf) - strlen(name) - 2) ==
        0) { // numele userului + parantezele []
      bzero(tmp, 1010);
      sprintf(tmp, "[%s](%s)%s\n", tdL->u_name, buf + strlen(name) + 2,
              message + (strlen(buf) - strlen(name) - 2));
      FILE *fp_2 = fopen(f_name, "a");
      fputs(tmp, fp_2);
      fclose(fp_2);
      ok = 1;
      break;
    }
    bzero(buf, 1000);
  }
  if (ok == 0) {
    if (write(tdL->cl, "Nu am gasit mesajul pentru reply!",
              strlen("Nu am gasit mesajul pentru reply!")) <= 0) {
      perror("Eroare la write Login");
    }
  }
  if (ok == 1) {
    if (write(tdL->cl, "Ati raspuns la mesaj",
              strlen("Ati raspuns la mesaj")) <= 0) {
      perror("Eroare la write Login");
    }
  }
  fclose(fp);
}

void Login(struct thData *t) {

  char buf[100];
  bzero(buf, 100);
  if (write(t->cl, "Introduceti Userul:", strlen("Introduceti Userul:")) <= 0) {
    perror("Eroare la write Login");
  }
  if (read(t->cl, buf, sizeof(buf)) <= 0) {
    perror("Eroare la read Login");
  }
  int i = 0, ok = 0;
  for (i = 0; i < nr_utilizatori; i++)
    if (strcmp(buf, users[i].name) == 0) {
      ok = 1;
      if (write(t->cl, "Introduceti Parola:", strlen("Introduceti Parola:")) <=
          0) {
        perror("Eroare la write Login");
      }
      bzero(buf, 100);
      if (read(t->cl, buf, sizeof(buf)) <= 0) {
        perror("Eroare la read Login");
      }
      if (strcmp(buf, users[i].parola) == 0) {
        users[i].online = 1;
        bzero(t->u_name, 100);
        strcpy(t->u_name, users[i].name);

        //verificam numarul de mesaje necitite 
        
        char f_name[100];
        int nr_mesaje = 0;
        char c;
        sprintf(f_name, "%s_unread.txt", t->u_name);
        FILE *fp = fopen(f_name, "a+");
        for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
        nr_mesaje++;
        fclose(fp);
        char tmp[100];
        sprintf(tmp, "Sunteti conectat,aveti %d mesaje necitite", nr_mesaje);
        if (write(t->cl, tmp, strlen(tmp)) <= 0) {
          perror("Eroare la write Login");
        }
      } else if (strcmp(buf, users[i].parola) != 0) {
        if (write(t->cl, "Parola Incorecta", strlen("Parola Incorecta")) <= 0) {
          perror("Eroare la write Login");
        }
      }
    }

  if (ok == 0) {
    if (write(t->cl, "User incorect", strlen("User incorect")) <= 0)
      perror("Eroare la write Login");
  }
  
}
void Register(struct thData *t) {
  char name[50], parola[50], buf[110];
  bzero(name, 50);
  bzero(parola, 50);
  bzero(buf, 110);
  if (write(t->cl, "Introduceti Userul:", strlen("Introduceti Userul:")) <= 0) {
    perror("Eroare la write Register");
  }
  if (read(t->cl, name, sizeof(name)) <= 0) {
    perror("Eroare la read Register");
  }
  if (write(t->cl, "Introduceti Parola:", strlen("Introduceti Parola:")) <= 0) {
    perror("Eroare la write Register");
  }
  if (read(t->cl, parola, sizeof(parola)) <= 0) {
    perror("Eroare la read Register");
  }
  strcpy(users[nr_utilizatori].name, name);
  strcpy(users[nr_utilizatori].parola, parola);
  strcpy(t->u_name,name);
  users[nr_utilizatori].online=1;
  nr_utilizatori++;
  FILE *fp = fopen("users.txt", "a");
  sprintf(buf, "%s %s\n", name, parola);
  fputs(buf, fp);
  fclose(fp);
  if (write(t->cl, "Crearea contului a avut succes, acum va puteti conecta",
            strlen("Crearea contului a avut succes, acum va puteti conecta")) <=
      0) {
    perror("Eroare la write Register");
  }
}