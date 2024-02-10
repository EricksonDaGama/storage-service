/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

/* Função handler que é invocada quando o programa servidor recebe o sinal 
 * SIGINT ou SIGTSTP, termina o programa libertando os recursos alocados.
 */
void server_signal_handler();

/**
 * Obter o endereço IP da máquina
 */
char *get_server_ip();
