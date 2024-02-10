/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <poll.h>
#include "inet.h"
#include "tree_skel.h"
#include "network_server.h"
#include "message-private.h"

#define NFDESC 1024

// descritores dos sockets
struct pollfd connections[NFDESC];

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port) {
    int sockfd;
    // cria socket TCP
    if (0 > (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("Erro ao criar socket TCP");
        return -1;
    }
    int enable = 1;
    if (0 > setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        perror("Erro ao invocar setsockopt");
        close(sockfd);
        return -1;
    }
    struct sockaddr_in server;
    // preenche estrutura server para bind
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // faz bind
    if (0 > bind(sockfd, (struct sockaddr *) &server, sizeof(server))) { 
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }
    // faz listen
    if (0 > listen(sockfd, 0)) {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket) {
    struct sockaddr_in client;
    socklen_t size_client = 0;

    // posições da connections libertadas pelos clientes que fecharam o socket
    int freed_pos[NFDESC];
    // número de posições libertadas
    int n_freed_pos = 0;

    for (int i = 0; i < NFDESC; i++) {
        connections[i].fd = -1;
    }
    connections[0].fd = listening_socket;
    connections[0].events = POLLIN;

    int nfds = 1, kfds;

    while (1) {
        if (0 > (kfds = poll(connections, nfds, -1))) {
            perror("Erro ao invocar poll");
        } else if (0 < kfds) {
            if ((connections[0].revents & POLLIN) && (nfds < NFDESC)) {
                // indice da posição para inserir fd
                int i_insert;
                // se houver posições libertadas
                if (0 < n_freed_pos) {
                    i_insert = freed_pos[--n_freed_pos];
                } else {
                    i_insert = nfds++;
                }
                if (0 < (connections[i_insert].fd = accept(listening_socket, (struct sockaddr *) &client, &size_client))) {
                    connections[i_insert].events = POLLIN;
                    kfds--;
                }
            }
            
            for (int i = 1; i < nfds && 0 < kfds; i++) {
                if (-1 == connections[i].fd) continue;

                if (connections[i].revents & POLLIN) {
                    MessageT* msg;
                    if (NULL == (msg = network_receive(connections[i].fd)) ||
                        MESSAGE_T__OPCODE__OP_BAD == msg->opcode) {
                        if (0 == errno) {
                            printf("Um cliente fechou a ligação.\n");
                        } else {
                            perror("Erro ao receber mensagem do cliente");
                        }
                        if (NULL != msg) {
                            message_t__free_unpacked(msg, NULL);
                        }
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        freed_pos[n_freed_pos++] = i;
                    } else {
                        if (0 > invoke(msg)) {
                            fprintf(stderr, "Erro ao invocar invoke\n");
                            message_t__free_unpacked(msg, NULL);
                            return -1;
                        }
                        if (0 > network_send(connections[i].fd, msg)) {
                            fprintf(stderr, "Erro ao enviar mensagem ao cliente\n");
                            return -1;
                        }
                    }
                    kfds--;
                } else if ((connections[i].revents & POLLERR) || (connections[i].revents & POLLHUP)) {
                    close(connections[i].fd);
                    connections[i].fd = -1;
                    freed_pos[n_freed_pos++] = i;
                    kfds--;
                }
            }
        }
    }    
    return -1;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT *network_receive(int client_socket) {
    int len;
    uint8_t* buf;
    if (0 > (len = read_all(client_socket, &buf))) {
        fprintf(stderr, "Erro ao ler dados do cliente\n");
        free(buf);
        return NULL;
    }
    MessageT *ret_msg;
    if (NULL == (ret_msg = message_t__unpack(NULL, len, buf))) {
        perror("Erro ao desempacotar a mensagem");
        free(buf);
        return NULL;
    }
    // liberta a memória do buf alocada no read_all()
    free(buf);
    return ret_msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg) {
    // Serializar a mensagem de resposta
    int len = message_t__get_packed_size(msg);
    uint8_t buf[len];
    message_t__pack(msg, buf);

    // liberta a memória ocupada por msg 
    message_t__free_unpacked(msg, NULL);

    // Enviar a mensagem serializada
    if (0 > write_all(client_socket, buf, len)) {
        fprintf(stderr, "Erro ao enviar dados ao cliente\n");
        return -1;
    }
    return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close() {
    int result = 0;
    // fecha sockets
    for (int i = 0; i < NFDESC; i++) {
        if (-1 != connections[i].fd) {
            if (0 > close(connections[i].fd)) {
                perror("Erro ao fechar socket");
                result = -1;
            }
        }
    }
    return result;
}