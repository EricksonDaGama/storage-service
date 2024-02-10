/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Serializa todas as keys presentes no array de strings keys
 * para o buffer keys_buf que será alocado dentro da função.
 * O array de keys a passar em argumento pode ser obtido através 
 * da função tree_get_keys. Para além disso, retorna o tamanho do
 * buffer alocado ou -1 em caso de erro.
 */
int keyArray_to_buffer(char **keys, char **keys_buf) {
    if (NULL == keys) {
        return -1;
    }

    // get size of the buffer
    int i, datasize = 0;
    for (i = 0; NULL != keys[i]; i++) {
        datasize += strlen(keys[i]) + 1;
    }
    datasize += (i + 1) * sizeof(int);
    
    // allocate memory to the buffer
    *keys_buf = malloc(datasize);
    // store the number of keys
    memcpy(*keys_buf, &i, sizeof(int));

    // serialize keys
    int curr_index = sizeof(int);
    for (int j = 0; NULL != keys[j]; j++) {
        // serializa the key length
        int len = strlen(keys[j]) + 1;
        memcpy(*keys_buf + curr_index, &len, sizeof(int));
        curr_index += sizeof(int);
        // serialize the key
        memcpy(*keys_buf + curr_index, keys[j], len);
        curr_index += len;
    }
    return datasize;
}

/* De-serializa a mensagem contida em keys_buf, com tamanho
 * keys_buf_size, colocando-a e retornando-a num array char**,
 * cujo espaco em memória deve ser reservado. Devolve NULL
 * em caso de erro.
 */
char** buffer_to_keyArray(char *keys_buf, int keys_buf_size) {
    if (NULL == keys_buf || 0 >= keys_buf_size) {
        return NULL;
    }
    
    // get the number of keys stored in the buffer
    int num_keys;
    memcpy(&num_keys, keys_buf, sizeof(int));

    // allocate memory to the keys array
    char** keys = calloc(num_keys + 1, sizeof(char*));

    // keys array index
    int i = 0, 
    // current initial index of the buffer to store the next key
    curr_index = sizeof(int); 

    while (curr_index < keys_buf_size) {
        // deserialize the next key length
        int len;
        memcpy(&len, keys_buf + curr_index, sizeof(int));
        curr_index += sizeof(int);
        // deserialize the key
        keys[i] = malloc(len);
        memcpy(keys[i++], keys_buf + curr_index, len);
        curr_index += len;
    }

    return keys;
}
