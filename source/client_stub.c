/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"


/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t;

struct rtree_t *head, *tail;

static int ZDATA_LEN = 1024 * 1024;
char *ip_port_head, *ip_port_tail;

zhandle_t *client_zh;
int client_is_connected;

char *client_root_path = "/chain";

struct String_vector *client_children_list;
char *client_watcher_ctx = "ZooKeeper Child Watcher";

/**
* Watcher function for connection state change events
*/
void client_connection_watcher(zhandle_t *zh, int type, int state, const char *path, void *context) {
    if (type == ZOO_SESSION_EVENT) {
        client_is_connected = state == ZOO_CONNECTED_STATE;
    }
}

/**
* Child Watcher function for /chain node
*/
void client_child_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcher_ctx) {
    if (state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
        if (ZOK != zoo_wget_children(zh, client_root_path, client_child_watcher, watcher_ctx, client_children_list)) {
            printf("Erro ao fazer watch aos filhos de %s\n", client_root_path);
            return;
        }
        printf("\n=========== Znode List [ %s ] ===========\n", client_root_path);
        for (int i = 0; i < client_children_list->count; i++) {
            printf("%s\n", client_children_list->data[i]);
        }
        printf("=========================================\n\n");

        if (0 > connect_head_server()) {
            fprintf(stderr, "Erro ao ligar ao servidor head\n");
            deallocate_String_vector(client_children_list);
            return;
        }
        if (0 > connect_tail_server()) {
            fprintf(stderr, "Erro ao ligar ao servidor tail\n");
            deallocate_String_vector(client_children_list);
            return;
        }
        deallocate_String_vector(client_children_list);
    }
}

/**
 * Cliente liga ao Zookeeper
 */
int client_connect_zookeeper(const char* ip_port) {
    // Ligar ao ZooKeeper
    if (NULL == (client_zh = zookeeper_init(ip_port, client_connection_watcher, 2000, 0, 0, 0))) {
        perror("Erro ao ligar ao ZooKeeper");
        return -1;
    }
    // esperar que o cliente ligue ao ZooKeeper
    sleep(3);
    if (client_is_connected) {
        printf("Ligar ao ZooKeeper com sucesso.\n");
        client_children_list = malloc(sizeof(struct String_vector));
        // Obter e fazer watch aos filhos de /chain
        if (ZOK != zoo_wget_children(client_zh, client_root_path, client_child_watcher, client_watcher_ctx, client_children_list)) {
            printf("Erro ao fazer watch aos filhos de %s\n", client_root_path);
            return -1;
        }
        printf("Watch aos filhos de %s com sucesso.\n", client_root_path);

        printf("\n=========== Znode List [ %s ] ===========\n", client_root_path);
        for (int i = 0; i < client_children_list->count; i++) {
            printf("%s\n", client_children_list->data[i]);
        }
        printf("=============================================\n\n");

        // Ligar ao servidor head
        ip_port_head = malloc(ZDATA_LEN);
        if (0 > update_ip_port_head_server()) {
            fprintf(stderr, "Erro ao obter <IP>:<port> do servidor head\n");
            return -1;
        }
        if (NULL == (head = rtree_connect(ip_port_head))) {
            fprintf(stderr, "Erro ao estabelecer a ligação com o servidor head %s\n", ip_port_head);
            return -1;
        }
        printf("Ligar ao servidor head %s com sucesso.\n", ip_port_head);

        // Ligar ao servidor tail
        ip_port_tail = malloc(ZDATA_LEN);
        if (0 > update_ip_port_tail_server()) {
            fprintf(stderr, "Erro ao obter <IP>:<port> do servidor tail\n");
            return -1;
        }
        // se os servidores head e tail forem iguais
        if (0 == strcmp(ip_port_head, ip_port_tail)) {
            tail = rtree_copy(head);
        } else {
            if (NULL == (tail = rtree_connect(ip_port_tail))) {
                fprintf(stderr, "Erro ao estabelecer a ligação com o servidor tail %s\n", ip_port_tail);
                return -1;
            }
        }
        printf("Ligar ao servidor tail %s com sucesso.\n", ip_port_tail);

        deallocate_String_vector(client_children_list);
        return 0;
    }
    return -1;
}

/**
 * Desliga ZooKeeper
*/
int client_disconnect_zookeeper() {
    int result = 0;
    if (0 > rtree_disconnect(head)) {
        result = -1;
    }
    if (0 == strcmp(ip_port_head, ip_port_tail)) {
        free(tail->address);
        free(tail->port);
        free(tail);
    } else if (0 > rtree_disconnect(tail)) {
        result = -1;
    }
    free(ip_port_head);
    free(ip_port_tail);
    free(client_children_list);
    if (ZOK != zookeeper_close(client_zh)) {
        result = -1;
    }
    return result;
}

/**
 * Obter o <IP>:<port> do servidor head
 */
int update_ip_port_head_server() {
    if (NULL != client_children_list && 0 < client_children_list->count) {
        char *id_head_server = client_children_list->data[0];
        // obter do ZooKeeper o id do znode mais baixo
        for (int i = 1; i < client_children_list->count; i++) {
            if (0 < strcmp(id_head_server, client_children_list->data[i])) {
                id_head_server = client_children_list->data[i];
            }
        }
        // obter o caminho do servidor head
        char head_path[120] = {0};
        sprintf(head_path, "%s/%s", client_root_path, id_head_server);
        printf("Id do servidor head: %s\n", head_path);

        // obter ip do servidor head
        if (ZOK != zoo_get(client_zh, head_path, 0, ip_port_head, &ZDATA_LEN, 0)) {
            fprintf(stderr, "Erro ao obter dado no %s\n", head_path);
            return -1;
        }
        printf("<IP>:<port> do servidor head: %s\n", ip_port_head);
        return 0;
    }
    return -1;
}

/**
 * Obter o <IP>:<port> do servidor tail
 */
int update_ip_port_tail_server() {
    if (NULL != client_children_list && 0 < client_children_list->count) {
        char *id_tail_server = client_children_list->data[0];
        // obter do ZooKeeper o id do znode mais alto
        for (int i = 1; i < client_children_list->count; i++) {
            if (0 > strcmp(id_tail_server, client_children_list->data[i])) {
                id_tail_server = client_children_list->data[i];
            }
        }
        // obter o caminho do servidor tail
        char tail_path[120] = {0};
        sprintf(tail_path, "%s/%s", client_root_path, id_tail_server);
        printf("Id do servidor tail: %s\n", tail_path);

        // obter ip do servidor tail
        if (ZOK != zoo_get(client_zh, tail_path, 0, ip_port_tail, &ZDATA_LEN, 0)) {
            fprintf(stderr, "Erro ao obter dado no %s\n", tail_path);
            return -1;
        }
        printf("<IP>:<port> do servidor tail: %s\n", ip_port_tail);
        return 0;
    }
    return -1;
}

/**
 * Ligar à cabeça da cadeia de replicação
 */
int connect_head_server() {
    if (0 > update_ip_port_head_server()) {
        fprintf(stderr, "Erro ao obter <IP>:<port> do servidor head\n");
        return -1;
    }
    char ip_port_head_before[1024] = {0};
    sprintf(ip_port_head_before, "%s:%s", head->address, head->port);
        
    // se o servidor head mudou
    if (0 != strcmp(ip_port_head_before, ip_port_head)) {
        // se os servidores head e tail atuais forem iguais
        if (0 == strcmp(ip_port_head_before, ip_port_tail)) {
            free(head->address);
            free(head->port);
            free(head);
        } else {
            rtree_disconnect(head);
        }
        // se o servidor head a que vai ligar e o tail forem iguais
        if (0 == strcmp(ip_port_head, ip_port_tail)) {
            head = rtree_copy(tail);
        } else {
            // preencher os campos da estrutura rtree e ligar ao servidor head
            if (NULL == (head = rtree_connect(ip_port_head))) {
                fprintf(stderr, "Erro ao estabelecer a ligação com o servidor head %s\n", ip_port_head);
                return -1;
            }
        }
    }
    return 0;
}

/**
 * Ligar à cauda da cadeia de replicação
 */
int connect_tail_server() {
    if (0 > update_ip_port_tail_server()) {
        fprintf(stderr, "Erro ao obter <IP>:<port> do servidor tail\n");
        return -1;
    }
    char ip_port_tail_before[1024] = {0};
    sprintf(ip_port_tail_before, "%s:%s", tail->address, tail->port);

    // o servidor tail mudou
    if (0 != strcmp(ip_port_tail_before, ip_port_tail)) {
        // se servidores atuais head e tail forem iguais
        if (0 == strcmp(ip_port_tail_before, ip_port_head)) {
            free(tail->address);
            free(tail->port);
            free(tail);
        } else {
            rtree_disconnect(tail);
        }
        // se o servidor tail a que vai ligar e o head forem iguais
        if (0 == strcmp(ip_port_head, ip_port_tail)) {
            tail = rtree_copy(head);
        } else {
            // preencher os campos da estrutura rtree e ligar ao servidor tail
            if (NULL == (tail = rtree_connect(ip_port_tail))) {
                fprintf(stderr, "Erro ao estabelecer a ligação com o servidor tail %s\n", ip_port_tail);
                return -1;
            }
        }
    }
    return 0;
}

/**
 * Deep copy a struct rtree_t
 */
struct rtree_t * rtree_copy(struct rtree_t* rtree) {
    struct rtree_t* copy = malloc(sizeof(struct rtree_t));
    copy->address = malloc(strlen(rtree->address) + 1);
    strcpy(copy->address, rtree->address);

    copy->port = malloc(strlen(rtree->port) + 1);
    strcpy(copy->port, rtree->port);

    copy->sockfd = rtree->sockfd;
    return copy;
}

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port) {
    if (NULL == address_port) {
        return NULL;
    }
    // i armazena o index do ":"
    int i = 0, arg_len = strlen(address_port);
    while (i < arg_len && ':' != address_port[i]) i++;

    // caso ":" não foi encontrado
    if (i >= arg_len) {
        printf("Formato do arg <IP>:<port> inválido.\n");
        return NULL;
    }
    // preenche os campos de rtree
    struct rtree_t* rtree = malloc(sizeof(struct rtree_t));
    rtree->address = calloc(i + 1, sizeof(char));
    rtree->port = malloc(arg_len - i);
    memcpy(rtree->address, address_port, i);
    strcpy(rtree->port, address_port + i + 1);

    // conectar ao servidor
    if (0 > network_connect(rtree)) {
        fprintf(stderr, "Erro ao estabelecer a ligação com o servidor %s:%s\n", rtree->address, rtree->port);
        free(rtree->address);
        free(rtree->port);
        free(rtree);
        return NULL;
    }
    return rtree;
}

/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree) {
    if (NULL == rtree) {
        return 0;
    }
    int ret = 0;
    if (0 > network_close(rtree)) {
        fprintf(stderr, "Erro ao fechar a ligação com o servidor %s:%s\n", rtree->address, rtree->port);
        ret = -1;
    }
    free(rtree->address);
    free(rtree->port);
    free(rtree);
    return ret;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {
    MessageT__Entry msg_entry;
    message_t__entry__init(&msg_entry);
    msg_entry.key = entry->key;
    msg_entry.data.len = entry->value->datasize;
    msg_entry.data.data = entry->value->data;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
    msg.entry = &msg_entry;

    MessageT *ret_msg = network_send_receive(rtree, &msg);

    // verifica e guarda o resultado
    int result = NULL != ret_msg && MESSAGE_T__OPCODE__OP_PUT + 1 == ret_msg->opcode ? ret_msg->result : -1;
    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.n_keys = 1;
    msg.keys = &key;

    MessageT* ret_msg = network_send_receive(rtree, &msg);
    
    struct data_t* result = NULL;
    if (NULL != ret_msg && MESSAGE_T__OPCODE__OP_GET + 1 == ret_msg->opcode) {
        result = malloc(sizeof(struct data_t));
        if (0 < ret_msg->n_values) {
            result->datasize = ret_msg->values[0].len;
            result->data = malloc(result->datasize);
            memcpy(result->data, ret_msg->values[0].data, result->datasize);
        } else {
            result->datasize = 0;
            result->data = NULL;
        }
    }
    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.n_keys = 1;
    msg.keys = &key;
    
    MessageT* ret_msg = network_send_receive(rtree, &msg);
    // verifica e guarda o resultado
    int result = NULL != ret_msg && MESSAGE_T__OPCODE__OP_DEL + 1 == ret_msg->opcode ? ret_msg->result : -1;
    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret_msg = network_send_receive(rtree, &msg);

    int result = NULL != ret_msg && MESSAGE_T__OPCODE__OP_SIZE + 1 == ret_msg->opcode ? ret_msg->result : -1;

    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret_msg = network_send_receive(rtree, &msg);

    int result = NULL != ret_msg && MESSAGE_T__OPCODE__OP_HEIGHT + 1 == ret_msg->opcode ? ret_msg->result : -1;

    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    MessageT* ret_msg = network_send_receive(rtree, &msg);

    char **result = NULL;
    if (NULL != ret_msg && MESSAGE_T__OPCODE__OP_GETKEYS + 1 == ret_msg->opcode) {
        result = calloc(ret_msg->n_keys + 1, sizeof(char*));
        for (int i = 0; i < ret_msg->n_keys; i++) {
            result[i] = malloc(strlen(ret_msg->keys[i]) + 1);
            strcpy(result[i], ret_msg->keys[i]);
        }
    }
    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);
    
    return result;
}

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values(struct rtree_t *rtree) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETVALUES;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *ret_msg = network_send_receive(rtree, &msg);

    void **result = NULL;
    if (NULL != ret_msg && MESSAGE_T__OPCODE__OP_GETVALUES + 1 == ret_msg->opcode) {
        result = calloc(ret_msg->n_values + 1, sizeof(void*));
        for (int i = 0; i < ret_msg->n_values; i++) {
            result[i] = malloc(ret_msg->values[i].len);
            memcpy(result[i], ret_msg->values[i].data, ret_msg->values[i].len);
        }
    }
    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Verifica se a operação identificada por op_n foi executada.
*/
int rtree_verify(struct rtree_t *rtree, int op_n) {
    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.result = op_n;
    
    MessageT* ret_msg = network_send_receive(rtree, &msg);

    int result = NULL != ret_msg && MESSAGE_T__OPCODE__OP_VERIFY + 1 == ret_msg->opcode ? ret_msg->result : -1;

    // liberta a mensagem
    message_t__free_unpacked(ret_msg, NULL);

    return result;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int client_rtree_put(struct entry_t *entry) {
    return rtree_put(head, entry);
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *client_rtree_get(char *key) {
    return rtree_get(tail, key);
}

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int client_rtree_del(char *key) {
    return rtree_del(head, key);
}

/* Devolve o número de elementos contidos na árvore.
 */
int client_rtree_size() {
    return rtree_size(tail);
}

/* Função que devolve a altura da árvore.
 */
int client_rtree_height() {
    return rtree_height(tail);
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **client_rtree_get_keys() {
    return rtree_get_keys(tail);
}

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **client_rtree_get_values() {
    return rtree_get_values(tail);
}

/* Verifica se a operação identificada por op_n foi executada.
*/
int client_rtree_verify(int op_n) {
    return rtree_verify(tail, op_n);
}