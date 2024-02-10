/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdlib.h>
#include <string.h>
#include "data.h"

/* Função que cria um novo elemento de dados data_t, reservando a memória
 * necessária para armazenar os dados, especificada pelo parâmetro size 
 */
struct data_t *data_create(int size) {
    if (0 >= size) {
        return NULL;
    }
    struct data_t* elem;
    if (NULL == (elem = malloc(sizeof(struct data_t)))) {
        return NULL;
    }
    if (NULL == (elem->data = malloc(size))) {
        free(elem);
        return NULL;
    }
    elem->datasize = size;
    return elem;
}

/* Função que cria um novo elemento de dados data_t, inicializando o campo
 * data com o valor passado no parâmetro data, sem necessidade de reservar
 * memória para os dados.
 */
struct data_t *data_create2(int size, void *data) {
    if (0 >= size || NULL == data) {
        return NULL;
    }
    struct data_t* elem;
    if (NULL == (elem = malloc(sizeof(struct data_t)))) {
        return NULL;
    }
    elem->datasize = size;
    elem->data = data;
    return elem;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data) {
    if (NULL != data) {
        free(data->data);
        free(data);
    }
}

/* Função que duplica uma estrutura data_t, reservando toda a memória
 * necessária para a nova estrutura, inclusivamente dados.
 */
struct data_t *data_dup(struct data_t *data) {
    if (NULL == data || NULL == data->data || 0 >= data->datasize) {
        return NULL;
    }
    struct data_t* dup;
    if (NULL == (dup = data_create(data->datasize))) {
        return NULL;
    }
    memcpy(dup->data, data->data, data->datasize);
    return dup;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data) {
    if (NULL == data || 0 >= new_size || NULL == new_data) {
        return;
    }
    data->datasize = new_size;
    free(data->data);
    data->data = new_data;
}
