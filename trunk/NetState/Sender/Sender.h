#ifndef SENDER_H_
#define SENDER_H_

#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include "../Almtf.h"
using namespace std;

#define FILENAME_IN_GROUPS		"inalm.txt"				// arquivo com os grupos multicast
#define FILENAME_DATA			"bootlog.txt"				// arquivo com os dados a serem enviados

#define TIME_LIMIT		600000			// tempo limite de execução (ms)
#define TIME_LIMITED	true					// tem limite de tempo?

#define _LOG_SENDER_INFO
//#define _LOG_DEBUG


void get_args(void);

bool create_layers();

void start_layers();

void layer_start(ALMTFSenderLayer &layer);

int layer_calc_num_rec(ALMTFSenderLayer &layer, unsigned int argRec, struct timeval *argTime);

#ifdef _WIN32_

DWORD printf_thread(list<string> *l);

DWORD recvThread(ALMTFSenderLayer *layer);

DWORD sendThread(ALMTFSenderLayer *layer);

#else

void *printf_thread(void *lc);

void *recvThread(void *layerc);

void *sendThread(void *layerc);

#endif
#ifdef SINCRONISMO_JOIN
bool sendpkt(ALMTFSenderLayer *layer, transm_packet &tpkt, ALMTFSincVar *vrSinc);
#else
bool sendpkt(ALMTFSenderLayer *layer, transm_packet &tpkt);
#endif

bool all_threads_finished(void);

bool finish_execution(void);

void getrecvIP(unsigned int intip, unsigned int *r);

#ifdef SINCRONISMO_JOIN

void init_sincvars(ALMTFSincVar *vrSinc);

unsigned int set_sincvar(ALMTFSincVar *vrSinc);

#endif

bool read_config_file();


#endif /*SENDER_H_*/
