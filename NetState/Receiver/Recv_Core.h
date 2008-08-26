#ifndef RECV_CORE_H_
#define RECV_CORE_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <deque>
#include <vector>

#include "../Almtf.h"
#include "Recv_Almtf.h"
#include "../Utils/Socket_Utils.h"
#include "../Utils/Utils.h"

//#define _LOG_COREDEBUG
#define _LOG_STATUS
//#define _LOG_LAYERS

#define FILENAME_IN_GROUPS		"inalm.txt"
#define FILENAME_OUT_PACK		"LOG_OutPackets.txt"
#define FILENAME_OUT_SPEED		"LOG_OutSpeed.txt"
#define LOG_TIME_LIMIT			30000
#define LOG_TIME_STEP			1000

// Inicializa as variáveis globais do núcleo
void core_init();

// Lê o arquivo de configurações com os grupos multicast.
bool core_readConfigs();

// Se cadastra em mais uma camada (e apenas uma).
// Cria a thread de recepção de dados, onde será criada a conexão multicast.
void core_addLayer();

// Se desconecta da camada atual, desabilitando a thread para que ela se encerre
// e encerre a conexão.
void core_leaveLayer();

// Retorna true quando todas as camadas encerraram a execução
bool core_allLayersFinished();

#ifdef _LOG_SPEED_AND_PACKETS

// Thread que a cada intervalo de tempo busca os dados das camadas,
// calcula e salva os resultados nos vetores
void *core_logSaveResultsThread(void *);

// Função que salva os resultados dos vetores de log em arquivos
void core_logSaveResults();

#endif

// Thread para receber os dados
// Parâmetro é a camada que esta sendo utilizada
#ifdef _WIN32_

void core_recvThread(ALMTFRecLayer* layer);

void core_recvPacket(ALMTFRecLayer* layer);

#else

void *core_recvThread(void *layerc);

#ifdef _PACKETS_BUFFER

void *core_recvPacket(void *layerc);

#endif

#endif

// Envia o pacote pkt para o transmissor
void core_sendPacket(transm_packet *pkt);

int core_getLogTime();

void core_setLogTime(int value);

int core_getLogStep();

void core_setLogStep(int value);

int core_getRatesCumActual();

int core_getRatesCum(int layer);

int core_getTotalLayers();

int core_getActualLayer();

void core_getLastPktTime(struct timeval *t);

void core_getLastPktTimeLayerZero(struct timeval *t);

void core_getLastTimestamp(struct timeval *t);

#ifdef _WIN32_

DWORD core_printThread(list<string> *l);

#else

void *core_printThread(void *lc);

#endif

void core_sendPrintList(string s);

#ifdef GRAPH_JAVAINTERFACE

void GRAPH_frame(GRAPHInfo num, int* n, char* res);

void *GRAPH_sendMsg(void *);

#endif


#endif /*RECV_CORE_H_*/
