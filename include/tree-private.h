/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"
#include "entry.h"

struct tree_t {
	struct entry_t *entry;
	struct tree_t *left, *right;
};

/* Função que devolve o maior de dois parâmetros do tipo int.
 */
int max(int, int);

/* Função que elimina um elemento do tipo struct tree_t, 
 * libertando a memória por ele ocupado.
 */
void treeElement_destroy(struct tree_t*);

#endif

