#ifndef _PUISSANCE_H
#define _PUISSANCE_H
typedef unsigned char Uchar;
typedef enum {
  ROUGE = 0,
  JAUNE = 1,
  VIDE = 2,
} Case;

typedef struct {
  Uchar id_joueur; /* idantifiant du prochain joueur dont c'est le tour*/
  Case grille[6][7];
} jeu;

extern jeu *init(void);
extern int jouer(jeu *monjeu, char colomne);
extern void afficher(jeu monjeu);
extern int gagner(jeu monjeu, char colonne);
extern int plein(jeu monjeu);

#endif