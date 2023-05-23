#include "message.h"
#include "tcp.h"
#include "utils.h"
#include "puissance.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int fd;
static char MYPseudo[65], ADPseudo[65];
void viderBuffer()
{
    char c = 0;
    while (c != '\n'  && c != EOF)
        c = getchar();
}
/*  fonction qui envoie un message d'abondon au serveur elle est appelée quand il y'a un signal SIGINT Ctrl+c */
void monExit() {
  sendConcede(fd);
  handle_error(close(fd), "close()");
  printf("Connexion fermée.\n");
  exit(0);
}

int main(int argc, char **argv) {
  /* Gestion du signal SIGINT */
  signal(SIGINT, monExit);
  if (argc < 1 + 2 + 1) {
    printf("Usage: %s <ip> <port> <pseudo>\n", argv[0]);
    exit(-1);
  }
  const in_port_t port = atoi(argv[2]);
  char *ip6 = argv[1];
  int rc;
  char id;
  jeu game;
  message msg;
  int Iplay;

  fd = install_client(ip6, port);
  handle_error(fd, "install_client()");
  //  envoi du pseudo directement après la connection 
  sendPseudo(fd, argv[3]);
  
  system("clear");

  fprintf(stdout, "EN ATTENTE D'ADVERSAIRE  \n");
  //  lecture du message start si tout c'est bien passé sinon on lit un message Discon
  readMessage(fd, &msg, INFINI);
  if(msg.type == DISCON)
  {
    fprintf(stdout, "\n VOTRE ADVERSSAIRE S'EST DECONNECTÉ  \n");
    exit(-1);
  }
  id = msg.body[0];
  strcpy(MYPseudo, msg.body + 1);
  strcpy(ADPseudo, msg.body + 67);
  Iplay = 0;
  while (1) {
    /* si le joueur a envoyé un message MOVE au serveur (Iplay == 1) donc le waittime == 1 car il attend un moveAck sinon c'est à l'autre de jouer*/ 
    rc = readMessage(fd, &msg, (Iplay == 1) ? 1 : CLIENT_WAIT_TIME);
    if (rc < 0) {
      fprintf(stdout, "PARTIE TERMINÉE \n");
      exit(0);
    }
    switch (msg.type) {
    case GRID:
      // mettre à jour la grille.
      for (int i = 0; i < 42; ++i)
        game.grille[i % 6][i / 6] = msg.body[2 + i];
      
      system("clear");
      fprintf(stdout, "\n\t%s\tvs\t%s\n", MYPseudo, ADPseudo);
      afficher(game);

      switch (msg.body[0]) {
      case 0:
        // le cas où c'est son tour de jouer
        if (msg.body[1] == id) {
          char col;
          Iplay = 1;
          fprintf(stderr, "donner la colonne : ");
          do{
            rc = read(0, &col, 1);
            if (rc < 0) {
              close(fd);
              exit(0);
            }
          }while(rc == 0);
          sendMove(fd, col - 48/*conversion du caractère ascii */ );
          viderBuffer();
        }
        break;
      case 1:
        //  Le cas ou l'un des joueur a gagné
        if (msg.body[1] == id)
          fprintf(stdout, "TU AS GAGNÉ\n");
        else
          fprintf(stdout, "TU AS PÉRDU\n");
        close(fd);
        goto FIN;
      case 2:
        //  Le cas où il ya un match null
        fprintf(stdout, "MATCH NUL\n");
        close(fd);
        goto FIN;
      }
      break;

    case MOVEACK:
      //  Le cas pour confirmer ou refuter un coup 
      if (msg.body[1] == 1) {
        Iplay = 0;
        jouer(&game, msg.body[0]);
      }
      break;

    case CONCEDE:
      // Le cas où l'advairssaire a abondoné 
      fprintf(stdout, "\n VOTRE ADVERSSAIRE A ABONDONNÉ \n");
      goto FIN;

    case DISCON:
      // Le cas où  advairssaire a mis trop de temps à repondre
      fprintf(stdout, "\n VOTRE ADVERSSAIRE S'EST DECONNECTÉ  \n");
      goto FIN;
    }
  }
FIN:
  close(fd);

  return 0;
}
