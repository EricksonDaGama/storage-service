/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

/* Função handler que é invocada quando o programa cliente recebe o sinal 
 * SIGINT ou SIGTSTP, termina o programa libertando os recursos alocados.
 */
void client_signal_handler();

/* Função que executa a operação put
*/
void op_put(int *terminate);

/* Função que executa a operação get
*/
void op_get(int *terminate);

/* Função que executa a operação del
*/
void op_del(int *terminate);

/* Função que executa a operação size
*/
void op_size(int *terminate);

/* Função que executa a operação height
*/
void op_height(int *terminate);

/* Função que executa a operação getkeys
*/
void op_getkeys(int *terminate);

/* Função que executa a operação getvalues
*/
void op_getvalues(int *terminate);

/* Função que executa a operação verify
*/
void op_verify(int *terminate);
