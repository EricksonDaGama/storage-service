/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "entry.h"

/* Função que cria uma entry, reservando a memória necessária para a
 * estrutura e inicializando os campos key e value, respetivamente, com a
 * string e o bloco de dados passados como parâmetros, sem reservar
 * memória para estes campos.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {
    // make sure that the key of the created entry is always a valid string
    if (NULL == key) {
        return NULL;
    }
    struct entry_t* entry;
    if (NULL == (entry = malloc(sizeof(struct entry_t)))) {
        return NULL;
    }
    entry->key = key;
    entry->value = data;
    return entry;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry) {
    if (NULL != entry) {
        free(entry->key);
        data_destroy(entry->value);
        free(entry);
    }
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry) {
    return NULL == entry || NULL == entry->key ? NULL : entry_create(strdup(entry->key), data_dup(entry->value));
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {
    if (NULL != entry && NULL != new_key) {
        free(entry->key);
        entry->key = new_key;
        data_destroy(entry->value);
        entry->value = new_value;
    }
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2) {
    int ret;
    if (0 == (ret = strcmp(entry1->key, entry2->key))) {
        return 0;
    } else {
        return 0 > ret ? -1 : 1;
    }
}
