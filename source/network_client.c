/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include "inet.h"
#include "client_stub-private.h"
#include "network_client.h"
#include "message-private.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree) {  
    int sockfd;
    // cria socket TCP
    if (0 > (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("Erro ao criar socket TCP");
        return -1;
    }
    struct sockaddr_in server;
    // preenche estrutura server para connect
    server.sin_family = AF_INET;
    if (1 > inet_pton(AF_INET, rtree->address, &server.sin_addr)) {
        perror("Erro ao converter IP");
        close(sockfd);
        return -1;
    }
    int port;
    if (0 == (port = atoi(rtree->port))) {
        perror("Erro ao converter port");
        close(sockfd);
        return -1;
    }
    server.sin_port = htons(port);
    
    // estabelece conexão com o servidor
    if (0 > connect(sockfd, (struct sockaddr *) &server, sizeof(server))) {
        fprintf(stderr, "Erro ao conectar-se ao servidor %s:%s\n", rtree->address, rtree->port);
        close(sockfd);
        return -1;
    }
    rtree->sockfd = sockfd;
    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtree_t * rtree, MessageT *msg) {
    // Serializar a mensagem contida em msg
    int len = message_t__get_packed_size(msg);
    uint8_t buf[len];
    message_t__pack(msg, buf);

    // enviar a mensagem serializada para o servidor
    if (len != write_all(rtree->sockfd, buf, len)) {
        fprintf(stderr, "Erro ao enviar dados ao servidor\n");
        return NULL;
    }
    // obter a resposta do servidor
    uint8_t *ret_buf;
    if (0 > (len = read_all(rtree->sockfd, &ret_buf))) {
        fprintf(stderr, "Erro ao receber dados do servidor\n");
        free(ret_buf);
        return NULL;
    }
    // desempacotar a mensagem
    MessageT* ret_msg;
    if (NULL == (ret_msg = message_t__unpack(NULL, len, ret_buf))) {
        perror("Erro ao desempacotar a mensagem");
        free(ret_buf);
        return NULL;
    }

    // liberta a memória do ret_buf alocada no read_all()
    free(ret_buf);

    return ret_msg;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t * rtree) {
    return close(rtree->sockfd);
}