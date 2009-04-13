/*
*  Universidade Federal do Rio Grande do Sul - UFRGS
*  PRAV - Projetos de Audio e V¡deo / Projeto SAM
*  ALMTF++: Adaptative Layered Multicast TCP-Friendly
*
*  Copyright(C) 2008 - Andrea Collin Krob <ackrob@inf.ufrgs.br>
*                               Alberto Santos Junior <ascjunior@inf.ufrgs.br>
*                               Felipe Freitag Vargas <ffvargas@inf.ufrgs.br>
*                               Joao Portal <jvportal@inf.ufrgs.br>
*                               Jonas Hartmann <jonashartmann@inf.ufrgs.br>
*                               Valter Roesler <roesler@inf.ufrgs.br>
*
* Recv_Almtf.cpp: Implementacao do Algoritmo ALMTF - Versao Linux
*
*/
#ifndef RECV_ALMTF_H_
#define RECV_ALMTF_H_

#include <ifaddrs.h>
#include <math.h>
#include "../Almtf.h"
#include "Recv_Core.h"
#include "../Utils/Utils.h"

#define _LOG_BWMAX
#define _LOG_EP
#define _LOG_NUMLOSS
//#define _LOG_LOG
#define _LOG_RECV_TESTE
#define _LOG_BW
#define _LOG_EI
#define _LOG_BWJANELA
#define _LOG_CWND
#define _LOG_TIMEOUT

/*
 * defines do mecanismo de sincronismo
 */
#ifdef LEARNING
#define _LOG_FAILURES
#endif
#ifdef NETSTATE
#define _LOG_NETSTATE
#endif
#ifdef GRAPH_NETWORK
#define _LOG_LAYERS
#endif
#ifdef READ_CONFIG
#define FILENAME_LEARNING "inconfig.txt"
#endif

#define round(x) (x<0?ceil((x)-0.5):floor((x)+0.5))

typedef enum {ALM_STATE_START, ALM_STATE_STEADY} ALMstate;


// Inicializa as variáveis globais do ALMTF
void ALMTF_init();

// Inicializa vars usadas no cálculo da banda por equação
void ALMTF_inicializa_eqn();

// Intervalo de execução do ALM. Executa praticamente todo algoritmo.
void ALMTF_EI(double *Cwnd, double *Bwjanela, double *Bweq, ALMstate *ALM_state);

// transforma a taxa de kbits/s para o valor da janela
double ALMTF_rate2win(double *Bwjanela, double *Cwnd, double CwndMax);

// transforma o valor da janela para a taxa em kbits/s
double ALMTF_win2rate(double *Cwnd);

// Retorna true se deu timeout.
bool ALMTF_timeout();

// Reduz valor de Cwnd, zera perdas e calcula banda equivalente.
void ALMTF_reduzbanda(double *Cwnd, double *Bwjanela);

// Estima banda equivalente.
double ALMTF_estimabanda(struct timeval timestamp, double *Bweq, struct timeval actRTT);

// Estima perdas para utilizacao na equacao de banda.
void ALMTF_estimaperdas(struct timeval* timestamp, bool instant_loss);

// Calculo da equação TCP, calcula perdas para banda equivalente no receptor.
double ALMTF_perdas2banda(double p, struct timeval actRTT);

// Calcula a media dos ultimos 8 intervalos de perdas.
double ALMTF_media_ult_intervalos();

#ifdef _WIN32_

void ALMTF_calcbwmax();

#else

void *ALMTF_calcbwmax(void *);

#endif

// Função chamada sempre que um pacote é recebido
void ALMTF_recvPkt(ALMTFRecLayer *layer, transm_packet* pkt, struct timeval *timepkt);

int ALMTF_addLayer (double *Bweq, double *novabw, double *Cwnd);

void ALMTF_sendPkt();

#ifdef _LOG_LOG

void log_printAll(ALMstate *ALM_state, double *Bwjanela, double *Bweq, double *Cwnd);

#endif

#ifdef _LOG_BW

void *ALMTF_threadLogBw(void *);

#endif

struct timeval ALMTF_calculaRTT (long lseqno, transm_packet* pkt, struct timeval* timepkt, struct timeval pRTT);

struct timeval ALMTF_getRTT();

int ALMTF_getNumloss();

void ALMTF_setNumloss(int tpNloss);

#ifdef GRAPH_JAVAINTERFACE

void GRAPH_getInfo(GRAPHInfo* i);

#endif

bool ALMTF_setIP(char* IP);

unsigned int ALMTF_getIP(char* IP);

/*Inserido por Alberto 15/01/09: alteração no mecanismo de perdas e valores iniciais para Bwmax e Bweq*/
void ALMTF_SetBw(double banda);
/*Inserido por Alberto 09/02/09: segunda correção no mecanismo de estimativa de perdas*/
void ALMTF_InitTimeEI();
void ALMTF_SetTimeEI(long int t, ALMTF_STAB n);
bool ALMTF_GetTimeEI();
bool ALMTF_GetTimeEI(ALMTF_STAB n);

/*
 *tipos, defines e procedimentos definidos para o mecanismo de sincronismo
*/

#ifdef NETSTATE
typedef struct{
	double rtt; // rtt
	double perdas; // porcentagem de perdas
//Variáveis para guardar o estado da rede no momento de um join.
	struct timeval expireTime;
//instante no qual as variáveis foram armazenadas.
	bool falhou;					
//true se ocorreu uma falha, false caso contrario.

}NETSTATEvars;

#define NUM_ELEM 3

void init_netvars(NETSTATEvars *net);
void save_netvars(NETSTATEvars *net);
#endif

#ifdef GRAPH_JAVAINTERFACE

void GRAPH_getInfo(GRAPHInfo* i);

#endif

#ifdef READ_CONFIG
void learning_init();
#endif

#endif /*RECV_ALMTF_H_*/
