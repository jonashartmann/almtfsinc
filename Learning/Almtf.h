#ifndef ALMTF_H_
#define ALMTF_H_

#ifdef _WIN32_

#define <winsock.h>

#else

#include <netinet/in.h>

#endif

#include <list>

/*
 * Definição dos defines para habilitação da transmissão Unicast e para salvar os LOGs
 */
//#define _WIN32_
//#define _ALMTF_DEBUG
//#define GRAPH_JAVAINTERFACE
#define GRAPH_NETWORK
#define _LOG_SPEED_AND_PACKETS
#define SINCRONISMO_JOIN		// REFERENTE AO USO DO VETOR DE SINC_JOIN

/* Definir somente um de cada vez (altera os valores do vetor) */
#define SINCRONISMO_JOIN_1
//#define SINCRONISMO_JOIN_2
//#define SINCRONISMO_JOIN_3
//#define SINCRONISMO_JOIN_4

//#define TIME_SLOTS				// REFERENTE AO USO DE TIME SLOTS
#define LEARNING					// REFERENTE AO USO DE APRENDIZADO PELOS RECEPTORES
//#define _PACKETS_BUFFER

#define FL_INIRAJADA	0x80	// indicador de inicio de rajada (packet-pair ou packet-train)
#define FL_MEDERTT		0x40	// indica que timestampOffset eh valido
#define FL_ADDLAYER		0x20	// indica que pode subir para qualquer camada nesse slot

//#define ALMTF_PACKETSIZE		500				// Tamanho do pacote em bytes [almtfsim.tcl]
//#define ALMTF_PP				true			// true=com pares de pacotes; false=sem pares de pacotes
//#define ALMTF_PP_BURST_LENGTH	2
//#define ALMTF_T_STAB			1000			// Tempo para estabilização quando sai de uma camada ou dá timeout (em ms)
//#define ALMTF_WINDOWSIZE		30.0			// TODO ver valor correto ALMTF_windowsize
//#define ALMTF_RTT				500				// RTT inicial em ms
//#define ALMTF_DTSINC			1.0				// de quanto em quanto tempo receptores podem tentar join
//#define ALMTF_EIMULTIPLIER	1

#ifdef TIME_SLOTS

#define TIME_SLOT 	100						// Time slot em ms
#define N_ADD 		20*TIME_SLOT			// Minimum time slots before layer addition
#define N_DROP 		20*TIME_SLOT			// Threshold for quick drop

#endif

/*
 * ALMTFLayer
 * Definicao da estrutura da tabela de calculo do numero de receptores
 */
struct tbl_rec {
    unsigned int    rec;
    struct timeval time;
    struct tbl_rec *next;
};

/*
 * hdr_almtf
 * Definicao do cabecalho ALMTF
 */
typedef struct {
    long seqno;						// data sequence number
    unsigned int numReceptor;		// duas funcoes: echo do IP do receptor que quer calcular RTT (para evitar confusao entre receptores) e num de receptores
    struct timeval timestamp;		// momento da transmissao (usado para echo e calculo de RTT)
    struct timeval timestampEcho;	// echo do timestamp do receptor para calculo de RTT
    struct timeval timestampOffset;	// offset desde que transmissor recebeu pedido de feedback em ms
    unsigned int sincjoin;			// sincronismo no join (so receptores na camada menor ou igual a sincjoin podem fazer join)
    
    unsigned char flagsALMTF;		// flags do ALMTF
} hdr_almtf;

/*
 * transm_packet
 * Definicao do pacote de transmissão de dados 
 */
typedef struct {
	hdr_almtf header;
	char buffer[ALMTF_PACKETSIZE];
} transm_packet;

#ifdef SINCRONISMO_JOIN
typedef struct {
	double deltasinc;				// randomizacao para fazer sincjoin
	unsigned int sincjoin;					// sincronismo do join - indica até qual camada pode ser dado join
	double div_sinc;
	int ind_sinc;
	struct timeval time_ultsinc;	// ultima vez que foi enviado sincjoin
} ALMTFSincVar;
#endif

/*
 * ALMTFSenderLayer
 * Definicao da struct que define uma camada de transmissão de dados
 */
typedef struct 
{
	bool active;						// indica se a camada está ativa
	long int numPackets;		// número total de pacotes enviados
	long int speed;				// velocidade de transmissão dos dados (kbits/s)
	char IP[16];						// endereço IP multicast do grupo
	int port;							// porta de destino
	short layerID;					// identificacao do transmissor (camada que este transmissor esta transmitindo)
	long seqno;						// número de sequência do pacote
	int PP_numPkt;				// número do pacote da rajada que deve ser enviado. quando == 0, é o primeiro
	bool sendInfRTT;				// (quando true) flag para avisar que transmissor recebeu msg de um receptor
											// e ainda não enviou o pacote para cálculo de RTT, então bloqueia outras solicitações
	int burstSize;					// número de pacotes de uma rajada
	struct timeval lastReportRecv;				// hora que chegou pacote. Usado para medir offset a fim do receptor calcular RTT
	struct timeval timestampEcho;				// momento que receptor enviou a mensagem de report (para calculo de RTT)
	unsigned int numNodereceptorEcho;	// numero IP (nodo) do receptor que quer calcular RTT (para evitar confusao entre receptores)
	unsigned int totalRecv;						// Total de receptores
#ifdef _WIN32_
	DWORD sendThreadID;				// ID da thread criada para enviar os dados
	HANDLE sendThreadHD;
	SOCKET socketSendMC;				// objeto socket para enviar as msgs multicast
#else
	pthread_t sendThreadID;			// ID da thread criada para enviar os dados
	int socketSendMC;					// objeto socket para enviar as msgs multicast
#endif	
	bool threadFinished;				// indica se a thread que envia os dados foi finalizada ou não
	struct sockaddr_in socketSenderAddrIn;	// endereço usado pelo socket para enviar/receber
} ALMTFSenderLayer;

typedef struct {	
	double PP_value;
	short layerID;
} ALMTFBwmax;

/*
 * Estrutura que guarda um item na lista de pacotes
 * Contém o pacote em si e o timepkt que indica o momento em que o
 * pacote foi recebido
 */
typedef struct {
	transm_packet* pkt;
	struct timeval timepkt;
} ALMTFRecvdPacket;

typedef struct {
	// Eventos de perda
	unsigned int Ep;								// Número de eventos de perda até o momento
	float Ep_pesos[8];								// Pesos para cada evento de perda
	unsigned long Ep_pkt_rec[8];			// Número de pacotes recebidos entre cada evento de perda
	struct timeval Ep_tstamp_ultperda;	// Timestamp de quando ocorreu a última perda
} ALMTFLossEvent;

typedef struct {
	struct timeval Stab;
	struct timeval NoColateral;
	struct timeval AddLayerWait;
} ALMTFTimeEI;

/*
 * ALMTFRecLayer
 * Definicao da struct que define uma camada de RECEPÇÃO de dados
 * É uma struct diferente de ALMTFSenderLayer pois não precisa de tantos atributos
 */
typedef struct 
{
	long int speed;				// velocidade da camada (kbits/s)
	char IP[16];				// endereço IP multicast do grupo
	int port;					// porta para receber os dados
	short layerID;				// identificacao da camada (qual camada está sendo recebida por esta struct)
#ifdef _WIN32_
	DWORD dwRecvThread
	HANDLE haRecvThread;
	SOCKET sock;				// objeto socket para receber os pacotes
#ifdef _PACKETS_BUFFER
	DWORD dwProcThread;
	HANDLE haProcThread;
#endif
#else
	pthread_t IDthread;				// ID da thread criada para enviar os dados
	int sock;				// objeto socket para receber os pacotes
#ifdef _PACKETS_BUFFER
	pthread_t ptProcThread;
#endif
#endif	
#ifdef _PACKETS_BUFFER
	list<ALMTFRecvdPacket *> PktList;	
#endif	
	bool thread_enabled;		// quando false, a thread encerra a execução
	bool thread_running;		// indica se a thread está executando ou não
	struct ip_mreq sock_mreq;	// "solicitation for join and leave"
	long int nlost;
	long int seqno;
	long int seqno_expected;
	long int seqno_next;
	struct timeval first_packet;
	int measure;
	struct timeval start_time;	
	long int num_packets;
#ifdef _LOG_SPEED_AND_PACKETS
	int *speedXtime;
	int *packetsXtime;
#endif
} ALMTFRecLayer;

typedef struct {
	int Bwatual;
	double Bweq;
	double Bwjanela;
	double Bwmax;
	double RTT;
	int Numloss;
} GRAPHInfo;


#endif /*ALMTF_H_*/
