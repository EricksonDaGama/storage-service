/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "tree_cliente-private.h"

#define OP_TIMEOUT 5

const char* delim = " \t\r\n\v\f";

/* Programa cliente 
*/
int main(int argc, char const *argv[]) {
    // verifica o número de argumentos
    if (2 != argc) {
        printf("Uso: tree_client <IP>:<port>\n");
        return -1;
    }

    printf("Programa cliente inicia...\n");

    // ignorar o SIGPIPE
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        perror("Erro ao invocar signal");
        return -1;
    }
    // tratar o sinal SIGINT
    if (SIG_ERR == signal(SIGINT, client_signal_handler)) {
        perror("Erro ao invocar signal: tipo SIGINT");
        return -1;
    }
    // tratar o sinal SIGTSTP
    if (SIG_ERR == signal(SIGTSTP, client_signal_handler)) {
        perror("Erro ao invocar signal: tipo SIGTSTP");
        return -1;
    }

    char buf[BUFSIZ];
    int terminate = 0;

    if (0 > client_connect_zookeeper(argv[1])) {
        fprintf(stderr, "Erro ao ligar ao ZooKeeper\n");
        return -1;
    }

    do {
        char* token;
        do {
            // lê comando
            if (NULL == fgets(buf, BUFSIZ, stdin)) {
                perror("Erro ao ler comando"); 
                terminate = 1;               
                break;
            }
            token = strtok(buf, delim);
        } while (NULL == token);

        // ocorreu erro quando ler comando
        if (terminate) break;

        if (0 == strcmp(token, "put"))             op_put(&terminate);
        else if (0 == strcmp(token, "get"))        op_get(&terminate);
        else if (0 == strcmp(token, "del"))        op_del(&terminate);
        else if (0 == strcmp(token, "size"))       op_size(&terminate);
        else if (0 == strcmp(token, "height"))     op_height(&terminate);
        else if (0 == strcmp(token, "getkeys"))    op_getkeys(&terminate);
        else if (0 == strcmp(token, "getvalues"))  op_getvalues(&terminate);
        else if (0 == strcmp(token, "verify"))     op_verify(&terminate);
        else if (0 == strcmp(token, "quit"))       terminate = 1;
        else                                       printf("Comando inválido.\n");
        
    } while (!terminate);

    // fecha a ligação
    if (0 > client_disconnect_zookeeper()) {
        fprintf(stderr, "Erro ao fechar a ligação com o ZooKeeper\n");
        return -1;
    }

    return 0;
}

/* Função handler que é invocada quando o programa cliente recebe o sinal 
 * SIGINT ou SIGTSTP, termina o programa libertando os recursos alocados.
 */
void client_signal_handler() {
    if (0 > client_disconnect_zookeeper()) {
        fprintf(stderr, "Erro ao fechar a ligação com o ZooKeeper\n");
        exit(-1);
    }
    exit(0);
}

/* Função que executa a operação put
*/
void op_put(int *terminate) {
    char *key, *value;
    if (NULL == (key = strtok(NULL, delim)) || NULL == (value = strtok(NULL, delim))) {
        printf("Uso: put <key> <data>\n");
    } else {
        struct data_t data;
        data.datasize = strlen(value) + 1;
        data.data = value;

        struct entry_t entry;
        entry.key = key;
        entry.value = &data;

        int result;
        do {
            if (-1 == (result = client_rtree_put(&entry))) {
                printf("Operação put falhou.\n");
                *terminate = 1;
            } else {
                printf("O servidor recebeu o pedido.\nId da operação = %d\n", result);
                sleep(OP_TIMEOUT);
                result = client_rtree_verify(result);
            }
        } while (!terminate && 0 == result);
    }
}

/* Função que executa a operação get
*/
void op_get(int *terminate) {
    char *key;
    if (NULL == (key = strtok(NULL, delim))) {
        printf("Uso: get <key>\n");
    } else {
        struct data_t* result = NULL;
        if ((result = client_rtree_get(key))) {
            if (0 == result->datasize) {
                printf("Key not found.\n");
            } else {
                printf("Value = %s\n", (char*)result->data);
            }
            data_destroy(result);
        } else {
            printf("Operação get falhou.\n");
            *terminate = 1;
        }
    }
}

/* Função que executa a operação del
*/
void op_del(int *terminate) {
    char *key;
    if (NULL == (key = strtok(NULL, delim))) {
        printf("Uso: del <key>\n");
        return;
    } 
    
    int result;
    do {
        if (-1 == (result = client_rtree_del(key))) {
            printf("Operação del falhou.\n");
            *terminate = 1;
        } else {
            printf("O servidor recebeu o pedido.\nId da operação = %d\n", result);
            sleep(OP_TIMEOUT);
            result = client_rtree_verify(result);
        }
    } while (!terminate && 0 == result);
}

/* Função que executa a operação size
*/
void op_size(int *terminate) {
    int size;
    if (0 > (size = client_rtree_size())) {
        printf("Operação size falhou.\n");
        *terminate = 1;
    } else {
        printf("Size = %d\n", size);
    }
}

/* Função que executa a operação height
*/
void op_height(int *terminate) {
    int height;
    if (0 > (height = client_rtree_height())) {
        printf("Operação height falhou.\n");
        *terminate = 1;
    } else {
        printf("Height = %d\n", height);
    }
}

/* Função que executa a operação getkeys
*/
void op_getkeys(int *terminate) {
    char **keys;
    if ((keys = client_rtree_get_keys())) {
        if (NULL == keys[0]) {
            printf("No keys exist.\n");
        } else {
            printf("Keys:\n");
            int i = 0;
            while (NULL != keys[i]) {
                printf("%s\n", keys[i]);
                free(keys[i++]);
            }
        }
        free(keys);
    } else {
        printf("Operação getkeys falhou.\n");
        *terminate = 1;
    }
}

/* Função que executa a operação getvalues
*/
void op_getvalues(int *terminate) {
    void **values;
    if ((values = client_rtree_get_values())) {
        if (NULL == values[0]) {
            printf("No values exist.\n");
        } else {
            printf("Values:\n");
            int i = 0;
            while (NULL != values[i]) {
                printf("%s\n", (char*) values[i]);
                free(values[i++]);
            }
        }
        free(values);
    } else {
        printf("Operação getvalues falhou.\n");
        *terminate = 1;
    }
}

/* Função que executa a operação verify
*/
void op_verify(int *terminate) {
    char *op_n_char;
    if (NULL == (op_n_char = strtok(NULL, delim))) {
        printf("Uso: verify <op_n>\n");
        return;
    }
    int op_n;
    if (0 >= (op_n = atoi(op_n_char))) {
        printf("<op_n> tem de ser um inteiro e maior que 0.\n");
        return;
    }
    int result = client_rtree_verify(op_n);
    if (-1 == result) {
        printf("Operação verify falhou.\n");
        *terminate = 1;
    } else if (0 == result) {
        printf("A operação ainda não foi executada.\n");
    } else {
        printf("A operação já foi executada.\n");
    }
}