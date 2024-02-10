/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <zookeeper/zookeeper.h>
#include "entry.h"

/* Remote tree.
*/
struct rtree_t {
    char* address, *port;
    int sockfd;
};


/**
* Watcher function for connection state change events
*/
void client_connection_watcher(zhandle_t *zh, int type, int state, const char *path, void *context);

/**
* Child Watcher function for /chain node
*/
void client_child_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcher_ctx);

/**
 * Cliente liga ao Zookeeper
 */
int client_connect_zookeeper(const char* ip_port);

/**
 * Desliga ZooKeeper
*/
int client_disconnect_zookeeper();

/**
 * Ligar à cabeça da cadeia de replicação
 */
int connect_head_server();

/**
 * Ligar à cauda da cadeia de replicação
 */
int connect_tail_server();

/**
 * Obter o <IP>:<port> do servidor head
 */
int update_ip_port_head_server();

/**
 * Obter o <IP>:<port> do servidor tail
 */
int update_ip_port_tail_server();

/**
 * Deep copy a struct rtree_t
 */
struct rtree_t * rtree_copy(struct rtree_t* rtree);

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int client_rtree_put(struct entry_t *entry);

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *client_rtree_get(char *key);

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int client_rtree_del(char *key);

/* Devolve o número de elementos contidos na árvore.
 */
int client_rtree_size();

/* Função que devolve a altura da árvore.
 */
int client_rtree_height();

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **client_rtree_get_keys();

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **client_rtree_get_values();

/* Verifica se a operação identificada por op_n foi executada.
*/
int client_rtree_verify(int op_n);