/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdint.h>

/* Receber todos os bytes do servidor/cliente
*/
int read_all(int sock, uint8_t** buf);


/* Enviar todos os bytes ao servidor/cliente
*/
int write_all(int sock, uint8_t *buf, int len);
