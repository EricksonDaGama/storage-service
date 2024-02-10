/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "tree_skel-private.h"
#include "network_server.h"
#include "tree_server-private.h"

/* Programa servidor
*/
int main(int argc, char const *argv[]) {
    // verfica o número de argumentos
    if (3 != argc) {
        printf("Uso: tree_server <port> <IP>:<port>\n");
        return -1;
    }

    printf("Programa servidor inicia...\n");

    // ignorar o SIGPIPE
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        perror("Erro ao invocar signal: tipo SIGPIPE");
        return -1;
    }
    // tratar o sinal SIGINT
    if (SIG_ERR == signal(SIGINT, server_signal_handler)) {
        perror("Erro ao invocar signal: tipo SIGINT");
        return -1;
    }
    // tratar o sinal SIGTSTP
    if (SIG_ERR == signal(SIGTSTP, server_signal_handler)) {
        perror("Erro ao invocar signal: tipo SIGTSTP");
        return -1;
    }
    
    // verifica a validade do argumento
    int port;
    if (0 == (port = atoi(argv[1]))) {
        fprintf(stderr, "Erro ao converter porto\n");
        return -1;
    }

    int listening_sock;
    // obter socket de escuta
    if (0 > (listening_sock = network_server_init(port))) {
        fprintf(stderr, "Erro ao obter listening socket\n");
        return -1;
    }

    char* server_ip;
    if (NULL == (server_ip = get_server_ip())) {
        fprintf(stderr, "Erro ao obter o IP do servidor\n");
        return -1;
    }
    char server_ip_port[1024] = {0};
    sprintf(server_ip_port, "%s:%d", server_ip, port);
    free(server_ip);

    printf("<IP>:<port> do servidor: %s\n", server_ip_port);

    // Ligar ao ZooKeeper e ao próximo servidor
    if (0 > server_connect_zookeeper(argv[2], server_ip_port)) {
        fprintf(stderr, "Erro ao ligar ao ZooKeeper\n");
        network_server_close();
        return -1;
    }

    // inicializa a árvore binária
    if (0 > tree_skel_init(1)) {
        fprintf(stderr, "Erro ao inicializar a árvore binária\n");
        server_disconnect_zookeeper();
        network_server_close();
        return -1;
    }

    // começa a receber ligações dos clientes
    if (0 > network_main_loop(listening_sock)) {
        fprintf(stderr, "Erro ao aceitar conexão do cliente\n");
        tree_skel_destroy(); 
        server_disconnect_zookeeper();
        network_server_close();
        return -1;
    }

    return 0;
}

/* Função handler que é invocada quando o programa servidor recebe o sinal 
 * SIGINT ou SIGTSTP, termina o programa libertando os recursos alocados.
 */
void server_signal_handler() {
    int result = 0;
    tree_skel_destroy();
    if (0 > server_disconnect_zookeeper()) {
        fprintf(stderr, "Erro ao desligar ZooKeeper\n");
        result = -1;
    }
    if (0 > network_server_close()) {
        fprintf(stderr, "Erro ao fechar socket\n");
        result = -1;
    }
    exit(result);
}

/**
 * Obter o endereço IP da máquina
 */
char *get_server_ip() {
    char* ip_address = malloc(100 * sizeof(char));
    int fd;
    struct ifreq ifr;

    /*AF_INET - to define network interface IPv4*/
    /*Creating soket for it.*/
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /*AF_INET - to define IPv4 Address type.*/
    ifr.ifr_addr.sa_family = AF_INET;

    /*eth0 - define the ifr_name - port name
    where network attached.*/
    memcpy(ifr.ifr_name, "enp0s3", IFNAMSIZ - 1);

    /*Accessing network interface information by
    passing address using ioctl.*/
    if (0 > ioctl(fd, SIOCGIFADDR, &ifr)) {
        perror("ioctl");
        close(fd);
        return NULL;
    }

    /*closing fd*/
    close(fd);

    /*Extract IP Address*/
    strcpy(ip_address, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));

    return ip_address;
}