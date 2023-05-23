#include "message.h"
#include "tcp.h"
#include "utils.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int sockfd;
static jeu *game;
static message msg;

/*fonction qui est  appelé quand il y'a un exit pour liberer la memoire allouée
 * et ferme la socket*/
void monExit(void) {

  if (game) {
    free(game);
    game = NULL;
  }

  if (msg.body) {
    free(msg.body);
    msg.body = NULL;
  }
  close(sockfd);
}

/* fonction qui contient le déroulement du jeu */
int partie(int fd1, int fd2) {
  jeu *game = init();
  char *s1, *s2;
  char state = 0;
  //  recuperer le premier pseudo
  int rc = readMessage(fd1, &msg, SERVER_WAIT_TIME);
  if (rc < 0) {
    sendDiscon(fd2);
    close(fd1);
    close(fd2);
    exit(0);
  }
  s1 = malloc(sizeof(char) * 65);
  assert(s1);
  strcpy(s1, msg.body + 1);

  //  recuperer le deuxième pseudo
  rc = readMessage(fd2, &msg, SERVER_WAIT_TIME);
  if (rc < 0) {
    sendDiscon(fd1);
    close(fd1);
    close(fd2);
    exit(0);
  }
  s2 = malloc(sizeof(char) * 65);
  assert(s2);
  strcpy(s2, msg.body + 1);

  //  envoyer les message start au deux joueurs
  srand(getpid());
  int random = rand() % 2;
  rc = sendStart(fd1, s1, s2, random);
  handleDisconect(fd1, fd2, rc);
  rc = sendStart(fd2, s2, s1, (random + 1) % 2);
  handleDisconect(fd2, fd1, rc);

  int fd_read = (random == 0) ? fd1 : fd2;
  while (1) {

    // envoyer la grille au deux joueurs
    rc = sendGrid(fd1, *game, state, game->id_joueur);
    handleDisconect(fd1, fd2, rc);

    rc = sendGrid(fd2, *game, state, game->id_joueur);
    handleDisconect(fd2, fd1, rc);

  ETIQUETTE:
    rc = readMessage(fd_read, &msg, SERVER_WAIT_TIME);
    if (rc < 0) {
      // le cas ou le readMessage a echoué
      sendDiscon((fd_read == fd1) ? fd2 : fd1);
      exit(0);
    }
    switch (msg.type) {
    case MOVE:

      if (jouer(game, msg.body[0])) {
        // le coup donner par le client est valide
        sendMoveAck(fd_read, msg.body[0], 1);
        handleDisconect(fd_read, (fd_read == fd1) ? fd2 : fd1, rc);
        if (gagner(*game, msg.body[0])) {
          // le cas où le coup joué est gagnant
          sendGrid(fd1, *game, 1, (fd_read == fd1) ? 0 : 1);
          sendGrid(fd2, *game, 1, (fd_read == fd1) ? 0 : 1);
          close(fd1);
          close(fd2);
          exit(0);
        }

        if (plein(*game)) {
          // le cas où la grille du jeu est pleine suite au coup joué
          sendGrid(fd1, *game, 2, 0);
          sendGrid(fd2, *game, 2, 0);
          close(fd1);
          close(fd2);
          exit(0);
        }

        fd_read = (fd_read == fd1) ? fd2 : fd1;
      } else {
        // le coup donner par le client est non valide
        sendMoveAck(fd_read, msg.body[0], 0);
        handleDisconect(fd_read, (fd_read == fd1) ? fd2 : fd1, rc);
        sendGrid(fd_read, *game, state, game->id_joueur);
        handleDisconect(fd_read, (fd_read == fd1) ? fd2 : fd1, rc);

        // pour refaire un read sur la même sockete sans envoyer la grille à
        // l'autre joueur
        goto ETIQUETTE;
      }
      break;
    case CONCEDE:
      // le cas ou l'un des joueurs abondonne
      sendConcede((fd_read == fd1) ? fd2 : fd1);
      exit(0);
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 1 + 1) {
    printf("Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  //  Gestion du signal sigpipe pour eviter le crash du serveur
  signal(SIGPIPE, SIG_IGN);
  atexit(monExit);
  const in_port_t port = atoi(argv[1]);
  sockfd = install_server(port);
  handle_error(sockfd, "install_server()");
  printf("Ecoute sur le port %d\n", port);

  while (1) {
    //  accepter une première connexion
    int fd_sock_conn1 = accept(sockfd, NULL, NULL);
    if (fd_sock_conn1 < 0) {
      log_error(fd_sock_conn1, "accept()");
      continue;
    } else {
      //  accepter une deuxième connexion
      int fd_sock_conn2 = accept(sockfd, NULL, NULL);
      if (fd_sock_conn2 < 0) {
        log_error(fd_sock_conn1, "accept()");
        continue;
      } else {
        // lancer une partie dans un autre processus
        pid_t pid = fork();
        if (pid == 0) {
          partie(fd_sock_conn1, fd_sock_conn2);
          exit(0);
        }
      }
    }
    // recupérer les zombie
    while (1) {
      pid_t pid = waitpid(-1, NULL, WNOHANG);
      if (pid == 0 || (pid < 0 && errno == ECHILD)) {
        break;
      }

      if (pid < 0) {
        log_error(pid, " waitpid()");
        break;
      }
    }
  }

  handle_error(close(sockfd), "close()");
  return 0;
}
