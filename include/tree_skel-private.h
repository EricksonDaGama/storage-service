/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include "sdmessage.pb-c.h"
#include "tree_skel.h"
#include <zookeeper/zookeeper.h>

/* A função invoke() chamará esta função se houver erro
 */
void invoke_error(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_SIZE e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_size(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_HEIGHT e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_height(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_DEL e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_del(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GET e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_PUT e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_put(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GETKEYS e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get_keys(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_GETVALUES e esta coloca o resultado na estrutura msg 
 */
void invoke_tree_get_values(MessageT *msg);

/* A função invoke() chamará esta função se o opcode for 
 * MESSAGE_T__OPCODE__OP_VERIFY e esta coloca o resultado na estrutura msg 
 */
void invoke_verify(MessageT *msg);

/* Função que coloca o novo pedido na fila
*/
void produce_request(request_t* request);

/* Função que remove um pedido da fila e devolve este pedido
*/
request_t *consume_request();

/* Função que liberta a memória ocupada por request
*/
void destroy_request(request_t *request);

/**
* Watcher function for connection state change events
*/
void server_connection_watcher(zhandle_t *zh, int type, int state, const char *path, void *context);

/**
* Child Watcher function for /chain node
*/
void server_child_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcher_ctx);

/**
 * Servidor liga ao Zookeeper
 */
int server_connect_zookeeper(const char* zk_ip_port, char* server_ip_port);

/**
 * Desliga Zookeeper
 */
int server_disconnect_zookeeper();

/**
 * Ligar ao próximo servidor da cadeia de replicação
 */
int connect_next_server();