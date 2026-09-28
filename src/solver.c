//juste la logique de recherche de mot dans la grille
//
#include "../include/solver.h"
#include <string.h>
#include <ctype.h>

// les 8 directions possibles : (dx, dy)
static const int DIRECTIONS[8][2] = {
    { 1,  0},   // droite
    {-1,  0},   // gauche
    { 0,  1},   // bas
    { 0, -1},   // haut
    { 1,  1},   // diagonale bas-droite
    {-1, -1},   // diagonale haut-gauche
    { 1, -1},   // diagonale haut-droite
    {-1,  1},   // diagonale bas-gauche
};


