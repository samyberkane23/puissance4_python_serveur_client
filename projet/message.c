#include "message.h"
#include "tcp.h"
#include "utils.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*  cette fonction envoie un message msg sur un descripteur fd elle returne 0 si
 * tout c'est bien passé et -1 sinon */
int sendMessage(int fd, message msg) {
  int rc;
  rc = write_all(fd, &msg.type, sizeof(msg.type));
  if (rc <= 0) {
    return -1;
  }
  rc = write_all(fd, &msg.lenght, sizeof(msg.lenght));
  if (rc <= 0) {
    return -1;
  }
  if (msg.lenght != 0 && msg.body != NULL) {
    rc = write_all(fd, msg.body, msg.lenght);
    if (rc <= 0) {
      return -1;
    }
    free(msg.body);
  }

  return 0;
}

/*
  cette fonction tente de lire un message depuis un descripteur fd pendant un
  certain temps WAITTIME , elle retourne 0 si tou c'est bien passer , -1 s'il
  y'a eu un problème lors de la lecture et -2 si le WAITTIME et ecoulé
*/
int readMessage(int fd, message *msg, int WAITTIME) {
  int rc;
  msg->body = NULL;
  int flags = fcntl(fd, F_GETFL);
  rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  time_t t0 = time(NULL);
  while (1) {
    rc = read(fd, &msg->type, sizeof(unsigned char));
    if (rc >= 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
      break;
    }
    if (time(NULL) > t0 + WAITTIME) {
      fcntl(fd, F_SETFL, flags);
      return -2;
    }
  }
  if (rc <= 0) 
    return -1;

  t0 = time(NULL);
  while (1) {
    rc = read(fd, &msg->lenght, sizeof(unsigned char));
    if (rc >= 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
      break;
    }
    if (time(NULL) > t0 + WAITTIME) {
      fcntl(fd, F_SETFL, flags);
      return -2;
    }
  }

  if (rc <= 0) 
    return -1;
  

  if (msg->lenght != 0) {
    if(msg->body)
    {
        free(msg->body);
        msg->body = NULL;
    }
    msg->body = malloc(sizeof(char) * msg->lenght);
    assert(msg->body);
    t0 = time(NULL);
    while (1) {
      rc = read(fd, msg->body, sizeof(char) * msg->lenght);
      if (rc >= 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
        break;
      }
      if (time(NULL) > t0 + WAITTIME) {
        fcntl(fd, F_SETFL, flags);
        return -2;
      }
    }
    if (rc <= 0) 
      return -1;
  }

  return 0;
}

/*  cette fonction permet d'envoyer le TLV PSEUDO au descripteur fd     */
int sendPseudo(int fd, char *pseudo) {
  if(strlen(pseudo) > 64)
    pseudo[64] = '\0';
  message msg = {1, 65, NULL};
  char *body = (char *)calloc(sizeof(char), 65);
  assert(body);
  body[0] = strlen(pseudo);
  strcpy(body + 1, pseudo);
  msg.body = body;
  return (sendMessage(fd, msg));
}

/*  cette fonction permet d'envoyer le TLV START au descripteur fd     */
int sendStart(int fd, char *pseudo1, char *pseudo2, char pcol) {
  message msg = {2, 131, NULL};
  char *body = (char *)calloc(sizeof(char), 131);
  assert(body);
  body[0] = pcol;
  body[1] = strlen(pseudo1);
  strcpy(body + 2, pseudo1);
  body[66] = strlen(pseudo2);
  strcpy(body + 67, pseudo2);
  msg.body = body;
  return sendMessage(fd, msg);
}

/*  cette fonction permet d'envoyer le TLV GRID au descripteur fd     */
int sendGrid(int fd, jeu jeu, char state1, char state2) {
  message msg = {3, 44, NULL};
  char *body = (char *)malloc(sizeof(char) * 44);
  assert(body);
  body[0] = state1;
  body[1] = state2;
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 7; ++j)
      body[2 + (j * 6 + i)] = jeu.grille[i][j];
  msg.body = body;
  return sendMessage(fd, msg);
}

/*  cette fonction permet d'envoyer le TLV MOVE au descripteur fd     */
int sendMove(int fd, char col) {
  message msg = {4, 1, NULL};
  char *body = (char *)malloc(sizeof(char));
  assert(body);
  body[0] = col;
  msg.body = body;
  return sendMessage(fd, msg);
}

/*  cette fonction permet d'envoyer le TLV MOVEACK au descripteur fd     */
int sendMoveAck(int fd, char col, char ok) {
  message msg = {5, 2, NULL};
  char *body = (char *)malloc(sizeof(char) * 2);
  assert(body);
  body[0] = col;
  body[1] = ok;
  msg.body = body;
  return sendMessage(fd, msg);
}

/*  cette fonction permet d'envoyer le TLV CONCEDE au descripteur fd     */
int sendConcede(int fd) {
  message msg = {6, 0, NULL};
  return sendMessage(fd, msg);
}

/*  cette fonction permet d'envoyer le TLV DISCON au descripteur fd     */
int sendDiscon(int fd) {
  message msg = {7, 0, NULL};
  return sendMessage(fd, msg);
}

/* cette fonction renvoi 1 si le client s'est simplement deconecter */
/* et 0 si le client a envoyer un CONCEDE avant de quité la partie */
static int testDeconnection(int fd) {
  message msg;
  int rc = readMessage(fd, &msg, SERVER_WAIT_TIME);
  if (rc < 0) {
    return 1;
  }
  if (msg.type == CONCEDE)
    return 0;
  return 1;
}

int handleDisconect(int fd1, int fd2, int rc) {
  if (rc < 0) {
    if (testDeconnection(fd1))
      sendDiscon(fd2);
    else
      sendConcede(fd2);
    close(fd2);

    exit(0);
  }

  return 0;
}