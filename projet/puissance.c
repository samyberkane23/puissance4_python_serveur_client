#include "puissance.h"
#include <malloc.h>
#include <stdio.h>

/* cette fonction retourne une grille  dont toutes les cases sont  VIDE */
jeu *init(void) {
  jeu *puissance = malloc(sizeof(*puissance));
  puissance->id_joueur = 0;
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 7; ++j)
      puissance->grille[i][j] = VIDE;

  return puissance;
}

/* cette fonction retour vrai si un coup est valide et faux sinon */
static int coup_valide(jeu monjeu, char colomne) {
  return colomne >= 0 && colomne < 7 && monjeu.grille[5][(int)colomne] == VIDE;
}

/*cette fonction permet d'ajouter un jeton dans une colonne ,elle retourne vrai
 * si l'ajout s'est effectué et faux sinon */
int jouer(jeu *monjeu, char colomne) {
  if (!coup_valide(*monjeu, colomne))
    return 0;

  int i = 0;
  while (monjeu->grille[i][(int)colomne] != VIDE)
    ++i;

  monjeu->grille[i][(int)colomne] = (monjeu->id_joueur == 0) ? ROUGE : JAUNE;
  monjeu->id_joueur = (monjeu->id_joueur == 0) ? 1 : 0;
  return 1;
}

/*cette fonction permet d'afficher la grille sur la sortie standard */
void afficher(jeu momjeu) {
  fprintf(stdout, "\n     -----------------------------\n");
  for (int i = 5; i > -1; --i) {
    fprintf(stdout, " %d   |", i);
    for (int j = 0; j < 7; ++j)
      if (momjeu.grille[i][j] == VIDE)

        fprintf(stdout, "   |");
      else
        fprintf(stdout, " %d |", momjeu.grille[i][j]);

    fprintf(stdout, "\n     -----------------------------\n");
  }
  fprintf(stdout, "\n      ");
  for (int j = 0; j < 7; ++j)
    fprintf(stdout, " %d  ", j);
  fprintf(stdout, "\n");
}

/*  cette fonction prend en parametre la dernière colomne joué et retourne vrai
 * si l'un des jouer a gagné */
int gagner(jeu monjeu, char colonne) {
  Case couleur = (monjeu.id_joueur == 0) ? JAUNE : ROUGE;
  int l = 0; /* dernière ligne jouer */
  while (l < 6 && monjeu.grille[l][(int)colonne] != VIDE)
    l++;
  l--;
  int j, i = l;

  /*Test vertical */
  while (i >= 0 && monjeu.grille[i][(int)colonne] == couleur)
    --i;

  if (l - i == 4)
    return 1;

  int nb = 0;
  j = colonne;

  /*Test horizontal gauche */
  while (j >= 0 && monjeu.grille[l][j] == couleur) {
    j--;
    nb++;
  }
  if (nb == 4)
    return 1;

  j = colonne;
  nb--;
  /*Test horizontal droite */
  while (j < 7 && monjeu.grille[l][j] == couleur) {
    j++;
    nb++;
  }
  if (nb >= 4)
    return 1;

  /*première diagonale haut*/
  nb = 0;
  i = l;
  j = colonne;
  while (j < 7 && i < 6 && monjeu.grille[i][j] == couleur) {
    ++i;
    ++j;
    ++nb;
  }
  if (nb >= 4)
    return 1;
  /*première diagonale bas*/
  nb--;
  i = l;
  j = colonne;
  while (j >= 0 && i >= 0 && monjeu.grille[i][j] == couleur) {
    --i;
    --j;
    ++nb;
  }
  if (nb >= 4)
    return 1;
  /*deuxième diagonale haut*/
  nb = 0;
  i = l;
  j = colonne;
  while (j < 7 && i >= 0 && monjeu.grille[i][j] == couleur) {
    --i;
    ++j;
    ++nb;
  }
  if (nb >= 4)
    return 1;

  /*deuxième diagonale bas*/
  nb--;
  i = l;
  j = colonne;
  while (j >= 0 && i < 6 && monjeu.grille[i][j] == couleur) {
    ++i;
    --j;
    ++nb;
  }
  if (nb >= 4)
    return 1;

  return 0;
}

/* cette fonction retourne vrai si la grille est plien */
int plein(jeu monjeu) {
  for (int i = 0; i < 7; ++i)
    if (monjeu.grille[5][i] == VIDE)
      return 0;

  return 1;
}