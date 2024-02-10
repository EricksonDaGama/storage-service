/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include "inet.h"
#include "message-private.h"

/* Receber todos os bytes do servidor/cliente
*/
int read_all(int sock, uint8_t** buf) {
    int bufsize = 0;
    if (0 > read(sock, &bufsize, sizeof(int))) {
        perror("read size failed");
        return -1;
    }
    bufsize = ntohl(bufsize);
    if (NULL == (*buf = malloc(bufsize))) {
        perror("Erro ao invocar malloc");
        return -1;
    }
    int len = 0, n;
    while (bufsize > len) {
        if (0 > (n = read(sock, *buf + len, bufsize))) {
            if (EINTR == errno) continue;
            perror("read failed");
            return -1;
        }
        len += n;
    }
    return bufsize;
}

/* Enviar todos os bytes ao servidor/cliente
*/
int write_all(int sock, uint8_t *buf, int len) {
    int bufsize = len, n;
    // write data len
    int len_nl = htonl(len);
    if (sizeof(len) != write(sock, &len_nl, sizeof(len))) {
        perror("write size failed");
        return -1;
    }
    // write data
    while (0 < len) {
        if (0 > (n = write(sock, buf, len))) {
            if (EINTR == errno) continue;
            perror("write failed");
            return -1;
        }
        buf += n;
        len -= n;
    }
    return bufsize;
}