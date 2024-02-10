/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>
#include "tree.h"
#include "entry.h"
#include "tree_skel.h"
#include "tree_skel-private.h"
#include "client_stub.h"
#include "client_stub-private.h"

/* Árvore binária patilhada
*/
struct tree_t* tree;

/* Threads info
*/
int n_threads;
pthread_t* thread;
int *thread_param;

int last_assigned = 1;

struct {
    int max_proc;
    int* in_progress;
} op_proc;

request_t* queue_head = NULL;

/* mutex para controlar o acesso à árvore
*/
pthread_mutex_t m_tree = PTHREAD_MUTEX_INITIALIZER;

/* mutex para controlar o acesso à estrutura op_proc
*/
pthread_mutex_t m_op_proc = PTHREAD_MUTEX_INITIALIZER;

/* mutex e variável de condição para a fila de pedidos
*/
pthread_mutex_t m_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  c_queue = PTHREAD_COND_INITIALIZER;

/* Variável que indica a intenção de terminar o servidor
*/
int terminate = 0;

zhandle_t *server_zh;
int server_is_connected = 0;

char *server_root_path = "/chain";
char *server_node_path = "/chain/node";

static int ID_LEN = 1024;
char *id_server, *id_next_server;

struct String_vector *server_children_list;
char *server_watcher_ctx = "ZooKeeper Child Watcher";

struct rtree_t *next_server;

/**
* Watcher function for connection state change events
*/
void server_connection_watcher(zhandle_t *zh, int type, int state, const char *path, void *context) {
    if (type == ZOO_SESSION_EVENT) {
        server_is_connected = state == ZOO_CONNECTED_STATE;
    }
}

/**
* Child Watcher function for /chain node
*/
void server_child_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcher_ctx) {
    if (state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
        if (ZOK != zoo_wget_children(zh, server_root_path, server_child_watcher, watcher_ctx, server_children_list)) {
            printf("Erro ao fazer watch aos filhos de %s\n", server_root_path);
            return;
        }
        printf("\n=========== Znode List [ %s ] ===========\n", server_root_path);
        for (int i = 0; i < server_children_list->count; i++) {
            printf("%s\n", server_children_list->data[i]);
        }
        printf("=========================================\n\n");
        
        // esperar que o próximo servidor conclua a preparação para receber pedidos
        sleep(3);
        if (0 > connect_next_server()) {
            fprintf(stderr, "Erro ao ligar ao próximo servidor\n");
        }
        deallocate_String_vector(server_children_list);
    }
}

/**
 * Servidor liga ao Zookeeper
 */
int server_connect_zookeeper(const char* zk_ip_port, char* server_ip_port) {
    if (NULL == (server_zh = zookeeper_init(zk_ip_port, server_connection_watcher, 2000, 0, 0, 0))) {
        perror("Erro ao ligar ao ZooKeeper");
        return -1;
    }
    // esperar que o servidor ligue ao ZooKeeper
    sleep(3);
    if (server_is_connected) {
        printf("Ligar ao ZooKeeper com sucesso.\n");
        if (ZNONODE == zoo_exists(server_zh, server_root_path, 0, NULL)) {
            if (ZOK != zoo_create(server_zh, server_root_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
                printf("Erro ao criar %s\n", server_root_path);
                return -1;
            }
            printf("Criou-se o znode %s\n", server_root_path);
        }
        id_server = calloc(ID_LEN, sizeof(char));
        // Criar um ZNode efémero sequencial no ZooKeeper, filho de /chain
        // Guardar o id atribuído ao ZNode pelo ZooKeeper
        if (ZOK != zoo_create(server_zh, server_node_path, server_ip_port, strlen(server_ip_port) + 1, &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_EPHEMERAL | ZOO_SEQUENCE, id_server, ID_LEN)) {
            printf("Erro ao criar znode no %s\n", server_node_path);
            return -1;
        }
        printf("Id do servidor no ZooKeeper: %s\n", id_server);

        server_children_list = malloc(sizeof(struct String_vector));
        // Obter e fazer watch aos filhos de /chain
        if (ZOK != zoo_wget_children(server_zh, server_root_path, server_child_watcher, server_watcher_ctx, server_children_list)) {
            printf("Erro ao fazer watch aos filhos de %s\n", server_root_path);
            return -1;
        }
        printf("Watch aos filhos de %s com sucesso.\n", server_root_path);

        printf("\n=========== Znode List [ %s ] ===========\n", server_root_path);
        for (int i = 0; i < server_children_list->count; i++) {
            printf("%s\n", server_children_list->data[i]);
        }
        printf("=========================================\n\n");

        id_next_server = calloc(ID_LEN, sizeof(char));
        if (0 > connect_next_server()) {
            fprintf(stderr, "Erro ao ligar ao próximo servidor\n");
            deallocate_String_vector(server_children_list);
            return -1;
        }

        deallocate_String_vector(server_children_list);

        return 0;
    }
    return -1;
}

/**
 * Desliga Zookeeper
 */
int server_disconnect_zookeeper() {
    int result = 0;
    free(id_server);
    free(id_next_server);
    free(server_children_list);
    if (0 > rtree_disconnect(next_server)) {
        result = -1;
    }
    if (ZOK != zookeeper_close(server_zh)) {
        result = -1;
    }
    return result;
}

/**
 * Obter o id do próximo servidor no ZooKeeper
 */
char *get_id_next_server() {
    char id_next_server_temp[1024] = {0};
    char data_temp[1024] = {0};
    // Ver qual é o servidor com id mais alto a seguir ao nosso, de entre os filhos de /chain
    for (int i = 0; i < server_children_list->count; i++) {
        sprintf(data_temp, "%s/%s", server_root_path, server_children_list->data[i]);
        if (0 > strcmp(id_server, data_temp)) {
            if (0 == strcmp(id_next_server_temp, "") || 0 < strcmp(id_next_server_temp, data_temp)) {
                strcpy(id_next_server_temp, data_temp);
            }
        }
    }
    // se next server mudou
    if (0 != strcmp(id_next_server, id_next_server_temp)) {
        strcpy(id_next_server, id_next_server_temp);
    }
    return id_next_server;
}

/**
 * Ligar ao próximo servidor da cadeia de replicação
 */
int connect_next_server() {
    id_next_server = get_id_next_server();

    // se existir o próximo servidor
    if (0 != strcmp(id_next_server, "")) {
        printf("Id do servidor no ZooKeeper: %s\n", id_server);
        printf("Id do próximo servidor no ZooKeeper: %s\n", id_next_server);

        int zdata_len = 1024 * 1024;
        char ip_port_next_server [zdata_len];

        // obter ip do próximo servidor na cadeia
        if (ZOK != zoo_get(server_zh, id_next_server, 0, ip_port_next_server, &zdata_len, 0)) {
            fprintf(stderr, "Erro ao obter dado no %s\n", id_next_server);
            return -1;
        }
        printf("<IP>:<port> do próximo servidor: %s\n", ip_port_next_server);

        if (NULL == next_server) {
            // Guardar e ligar a esse servidor como next_server
            if (NULL == (next_server = rtree_connect(ip_port_next_server))) {
                fprintf(stderr, "Erro ao estabelecer a ligação com o próximo servidor %s\n", ip_port_next_server);
                return -1;
            }
            printf("Ligar ao próximo servidor %s com sucesso.\n", ip_port_next_server);
        } else {
            // obter o ip do next_server anterior
            char ip_port_next_server_before[1024] = {0};
            sprintf(ip_port_next_server_before, "%s:%s", next_server->address, next_server->port);

            // se o next_server mudou
            if (0 != strcmp(ip_port_next_server_before, ip_port_next_server)) {
                rtree_disconnect(next_server);
                // Guardar e ligar a esse servidor como next_server
                if (NULL == (next_server = rtree_connect(ip_port_next_server))) {
                    fprintf(stderr, "Erro ao estabelecer a ligação com o próximo servidor %s\n", ip_port_next_server);
                    return -1;
                }
                printf("Ligar ao próximo servidor %s com sucesso.\n", ip_port_next_server);
            }
        }
    } else {
        rtree_disconnect(next_server);
        next_server = NULL;
        printf("Id do servidor no ZooKeeper: %s\n", id_server);
        printf("Sou a cauda da cadeia.\n");
    }
    return 0;
}

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * A função deve lançar N threads secundárias responsáveis por atender
 * pedidos de escrita na árvore.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
*/
int tree_skel_init(int N) {
    if (NULL == (tree = tree_create())) {
        fprintf(stderr, "Erro ao criar a árvore\n");
        return -1;
    }
    n_threads = N;
    if (NULL == (op_proc.in_progress = calloc(n_threads, sizeof(int)))) {
        tree_destroy(tree);
        perror("Erro ao invocar calloc");
        return -1;
    }
    if (NULL == (thread = malloc(sizeof(pthread_t) * n_threads))) {
        free(op_proc.in_progress);
        tree_destroy(tree);
        perror("Erro ao invocar malloc");
        return -1;
    }
    if (NULL == (thread_param = malloc(sizeof(int) * n_threads))) {
        free(thread);
        free(op_proc.in_progress);
        tree_destroy(tree);
        perror("Erro ao invocar malloc");
        return -1;
    }
    for (int i = 0; i < n_threads; i++) {
        thread_param[i] = i;
        if (0 != pthread_create(&thread[i], NULL, &process_request, (void*) &thread_param[i])) {
            tree_skel_destroy();
            perror("Erro ao lançar a thread");
            return -1;
        }
    }
    return 0;
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy() {
    terminate = 1;
    pthread_cond_broadcast(&c_queue);
    for (int i = 0; i < n_threads; i++) {
        if (0 != pthread_join(thread[i], NULL)) {
            perror("Erro no join");
        }
    }
    free(thread_param);
    free(thread);
    free(op_proc.in_progress);
    tree_destroy(tree);
    if (NULL != queue_head) {
        while (NULL != queue_head->next) {
            request_t* temp = queue_head;
            queue_head = queue_head->next;
            destroy_request(temp);
        }
        destroy_request(queue_head);
    }
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(MessageT *msg) {
    if (NULL == tree) {
        invoke_error(msg);
        return -1;
    }
    switch (msg->opcode) {
        case MESSAGE_T__OPCODE__OP_SIZE:      invoke_tree_size(msg);       break;
        case MESSAGE_T__OPCODE__OP_HEIGHT:    invoke_tree_height(msg);     break;
        case MESSAGE_T__OPCODE__OP_DEL:       invoke_tree_del(msg);        break;
        case MESSAGE_T__OPCODE__OP_GET:       invoke_tree_get(msg);        break;
        case MESSAGE_T__OPCODE__OP_PUT:       invoke_tree_put(msg);        break;
        case MESSAGE_T__OPCODE__OP_GETKEYS:   invoke_tree_get_keys(msg);   break;
        case MESSAGE_T__OPCODE__OP_GETVALUES: invoke_tree_get_values(msg); break;
        case MESSAGE_T__OPCODE__OP_VERIFY:    invoke_verify(msg);          break;
        default: return -1;
    }
    return 0;
}

/* A função invoke() chamará esta função se houver erro
 */
void invoke_error(MessageT *msg) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_SIZE e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_size(MessageT *msg) {
    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

    pthread_mutex_lock(&m_tree);
    msg->result = tree_size(tree);
    pthread_mutex_unlock(&m_tree);
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_HEIGHT e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_height(MessageT *msg) {
    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

    pthread_mutex_lock(&m_tree);
    msg->result = tree_height(tree);
    pthread_mutex_unlock(&m_tree);
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_DEL e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_del(MessageT *msg) {
    request_t* new_request;
    if (0 >= msg->n_keys || NULL == (new_request = malloc(sizeof(request_t)))
        || NULL == (new_request->key = malloc(strlen(msg->keys[0]) + 1))) {
        invoke_error(msg);
        free(new_request);
    } else {
        // criar novo pedido
        new_request->op_n = last_assigned;
        new_request->op = 0;
        strcpy(new_request->key, msg->keys[0]);
        new_request->data = NULL;
        new_request->next = NULL;

        // colocar o pedido na fila
        produce_request(new_request);

        // construir a mensagem de retorno
        msg->opcode++;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = last_assigned++;
    }
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GET e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get(MessageT *msg) {
    if (0 >= msg->n_keys) {
        invoke_error(msg);
        return;
    }

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

    pthread_mutex_lock(&m_tree);
    struct data_t* result = tree_get(tree, msg->keys[0]);
    pthread_mutex_unlock(&m_tree);

    if (result) {
        ProtobufCBinaryData *data = malloc(sizeof(ProtobufCBinaryData));
        data->len = result->datasize;
        data->data = malloc(data->len);
        memcpy(data->data, result->data, data->len);

        msg->n_values = 1;
        msg->values = data;

        data_destroy(result);
    } else {
        msg->n_values = 0;
        msg->values = NULL;
    }
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_PUT e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_put(MessageT *msg) {
    struct data_t *data;
    request_t* new_request;
    if (NULL == msg->entry || 
        NULL == (data = data_create(msg->entry->data.len)) ||
        NULL == (new_request = malloc(sizeof(request_t))) || 
        NULL == (new_request->key = malloc(strlen(msg->entry->key) + 1))) {
        invoke_error(msg);
        free(data);
        free(new_request);
    } else {
        // criar novo pedido
        new_request->op_n = last_assigned;
        new_request->op = 1;
        strcpy(new_request->key, msg->entry->key);
        memcpy(data->data, msg->entry->data.data, data->datasize);
        new_request->data = data;
        new_request->next = NULL;

        // colocar o pedido na fila
        produce_request(new_request);

        // construir a mensagem de retorno
        msg->opcode++;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = last_assigned++;
    }
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GETKEYS e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get_keys(MessageT *msg) {
    pthread_mutex_lock(&m_tree);
    char **keys = tree_get_keys(tree);
    pthread_mutex_unlock(&m_tree);
    int i;
    for (i = 0; NULL != keys[i]; i++);
    
    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
    msg->n_keys = i; 
    msg->keys = malloc(i * sizeof(char*));

    for (i = 0; NULL != keys[i]; i++) {
        msg->keys[i] = malloc(strlen(keys[i]) + 1);
        memcpy(msg->keys[i], keys[i], strlen(keys[i]) + 1);
    }
    tree_free_keys(keys);
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GETVALUES e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get_values(MessageT *msg) {
    pthread_mutex_lock(&m_tree);
    void **values = tree_get_values(tree);
    pthread_mutex_unlock(&m_tree);
    int i;
    for (i = 0; NULL != values[i]; i++);

    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_VALUES;
    msg->n_values = i; 
    msg->values = malloc(i * sizeof(ProtobufCBinaryData));

    for (i = 0; NULL != values[i]; i++) {
        struct data_t *data_temp = values[i];
        msg->values[i].len = data_temp->datasize;
        msg->values[i].data = malloc(data_temp->datasize);
        memcpy(msg->values[i].data, data_temp->data, data_temp->datasize);
    }
    tree_free_values(values);
}

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_VERIFY e esta coloca o resultado na estrutura msg 
 */
void invoke_verify(MessageT *msg) {
    if (0 >= msg->result) {
        invoke_error(msg);
    } else {
        msg->opcode++;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        pthread_mutex_lock(&m_op_proc);
        msg->result = verify(msg->result);
        pthread_mutex_unlock(&m_op_proc);
    }
}

/* Verifica se a operação identificada por op_n foi executada.
*/
int verify(int op_n) {
    if (op_n <= 0 || op_proc.max_proc < op_n) {
        return 0;
    } else if (op_proc.max_proc == op_n) {
        return 1;
    } else {
        // verifica se o pedido está a ser executado pela thread secundária
        for (int i = 0; i < n_threads; i++) {
            if (op_proc.in_progress[i] == op_n) {
                return 0;
            }
        }
        return 1;
    }
}

/* Função da thread secundária que vai processar pedidos de escrita.
*/
void * process_request (void *params) {
    int* thread_id = (int*) params;
    request_t* request;
    
    // obter o pedido da fila
    while (!terminate && NULL != (request = consume_request())) {
        // atualiza in_progress
        pthread_mutex_lock(&m_op_proc);
        op_proc.in_progress[*thread_id] = request->op_n;
        pthread_mutex_unlock(&m_op_proc);
        
        // executa operação de escrita
        if (0 == request->op) { // executa delete
            pthread_mutex_lock(&m_tree);
            tree_del(tree, request->key);
            pthread_mutex_unlock(&m_tree);
        } else { // executa put
            pthread_mutex_lock(&m_tree);
            tree_put(tree, request->key, request->data);
            pthread_mutex_unlock(&m_tree);
        }
        // operação concluída
        
        // atualiza a estrutura op_proc
        pthread_mutex_lock(&m_op_proc);
        if (op_proc.max_proc < request->op_n) {
            op_proc.max_proc = request->op_n;
        }
        // retira op_n da fila
        op_proc.in_progress[*thread_id] = 0;
        pthread_mutex_unlock(&m_op_proc);

        if (NULL != next_server) {
            // enviar pedido para o próximo servidor
            int result;
            if (0 == request->op) {
                do {
                    result = rtree_del(next_server, request->key);
                } while (0 > result && NULL != next_server);
            } else {
                struct entry_t entry;
                entry.key = request->key;
                entry.value = request->data;
                do {
                    result = rtree_put(next_server, &entry);
                } while (0 > result && NULL != next_server);
            }
        }
        
        // liberta request
        destroy_request(request);
    }
    
    return NULL;
}

/* Função que coloca o novo pedido na fila
*/
void produce_request(request_t* request) {
    pthread_mutex_lock(&m_queue);
    if (NULL == queue_head) {
        queue_head = request;
    } else {
        request_t *curr = queue_head;
        while (NULL != curr->next) {
            curr = curr->next;
        }
        curr->next = request;
    }
    pthread_cond_signal(&c_queue);
    pthread_mutex_unlock(&m_queue);
}

/* Função que remove um pedido da fila e devolve este pedido
*/
request_t *consume_request() {
    pthread_mutex_lock(&m_queue);
    while (NULL == queue_head && !terminate) {
        pthread_cond_wait(&c_queue, &m_queue);
    }
    request_t *request = NULL;
    if (!terminate) {
        // obter um pedido da fila
        request = queue_head;
        queue_head = queue_head->next;
    }
    pthread_mutex_unlock(&m_queue);
    return request;
}

/* Função que liberta a memória ocupada por request
*/
void destroy_request(request_t *request) {
    free(request->key);
    data_destroy(request->data);
    free(request);
}