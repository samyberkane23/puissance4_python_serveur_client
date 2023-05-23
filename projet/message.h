#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "puissance.h"
#define INFINI 999999

#define CLIENT_WAIT_TIME 32
#define SERVER_WAIT_TIME 30

typedef enum {
  PSEUDO = 1,
  START = 2,
  GRID = 3,
  MOVE = 4,
  MOVEACK = 5,
  CONCEDE = 6,
  DISCON = 7,
} message_type;

typedef struct {
  unsigned char type;
  unsigned char lenght;
  char *body;
} message;

extern int readMessage(int fd, message *msg, int WAITTIME);
extern int sendMessage(int fd, message msg);
extern int sendPseudo(int fd, char *pseudo);
extern int sendGrid(int fd, jeu jeu, char state1, char state2);
extern int sendStart(int fd, char *pseudo1, char *pseudo2, char pcol);
extern int sendMove(int fd, char col);
extern int sendMoveAck(int fd, char col, char ok);
extern int sendConcede(int fd);
extern int sendDiscon(int fd);
extern int handleDisconect(int fd1, int fd2, int rc);

#endif