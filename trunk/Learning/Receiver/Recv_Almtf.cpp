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

#include "Recv_Almtf.h"


#ifdef _LOG_LOG
	ofstream ofs_log;
#endif
#ifdef _LOG_BWMAX
	ofstream ofs_logBwmax;
#endif
#ifdef _LOG_NUMLOSS
	ofstream ofs_logNumloss;
#endif
#ifdef _LOG_RECV_TESTE
	ofstream ofs_logRecvTeste;
#endif
#ifdef _LOG_BW
	ofstream ofs_logBw;
#endif
#ifdef _LOG_EI
	ofstream ofs_logEI;
#endif
#ifdef _LOG_EP
	ofstream ofs_logEp;
#endif	
#ifdef _LOG_BWJANELA
	ofstream ofs_logBwjanela;
#endif
#ifdef _LOG_CWND
	ofstream ofs_logCwnd;
#endif

#ifdef LOG_PERDAS
	ofstream ofs_logPerdas;
#endif

#ifdef _LOG_TIMEOUT
	ofstream ofs_logTimeout;
#endif

/*
 * definição dos logs do mecanismo de aprendizado
 */
#ifdef _LOG_FAILURES
	ofstream ofs_logFailures;
#endif
#ifdef _LOG_NETSTATE
	ofstream ofs_logNet;
#endif
#ifdef _LOG_LAYERS
	ofstream ofs_logLayers;
#endif

// ALTERACAO_LOG_P
#define _LOG_P
#ifdef _LOG_P
	ofstream ofs_logP;
#endif


//Variável RTT utiizada em várias funções, mas alterada apenas na CalculaRTT
struct timeval RTT;		// RTT em milisegundos
int Numloss;			// número de perdas
int Sincjoin_recv;		// variável para aprendizado de join dos receptores
int Sincjoin_recvCp;	// copia (não zerada) variável para aprendizado de join dos receptores

//Cálculo do RTT: Variáveis utilizadas apenas no cálculo do RTT: Apenas 1 thread tem acesso
bool FiltraRTT;
bool first_rtt = true;
unsigned int Numrec;
struct timeval LastReportSent;
long IntFeedback;
struct timeval t_vinda;
struct timeval t_assim;


/*
 * declaração das variáveis utilizadas no mecanismo de aprendizado
 */
/* Variaveis utilizadas na tecnica de Aprendizado do Receptor */
#ifdef LEARNING
static int fail_time;			// tempo x para considerar como uma falha (em ms)
static int mult_limit;			// limite que o multiplicador pode atingir
static int mult_inc;			// incrementador do multiplicador
int t_stab_mult = 1;			// Multiplicador do T_STAB
int join_failures = 0;			// Numero de falhas de join realizadas
struct timeval t_failure;
int stable_layer = 0;

// Abaixo sao declaracoes so para nao dar conflito com o NETSTATE
int storage_time;			// tempo maximo de armazenamento de um estado de falha.
double similarity;			// porcentagem de semelhanca
#endif

#ifdef NETSTATE
static int fail_time;			// tempo x para considerar como uma falha (em ms)
int join_failures = 0;
struct timeval t_failure;
static int storage_time;		// tempo maximo de armazenamento de um estado de falha.
static double similarity;		// porcentagem de semelhanca*/

int posAtual;			// indica a posicao a ser inserido o elemento na lista de estados.
typedef vector< vector<NETSTATEvars> > NETSTATElist;

/* Lista para guardar os variados estados da rede.
 * Quando um receptor sobe para a camada i,
 * ele guarda o estado da rede na posicao i da lista.
 */
NETSTATElist netList;

// Abaixo sao declaracoes so para nao dar conflito com o LEARNING
static int mult_limit;			// limite que o multiplicador pode atingir
static int mult_inc;			// incrementador do multiplicador
int stable_layer = 0;
#endif


//Cálculo da Banda Máxima: alterado apenas na thread que realiza o cálculo
double Bwmax;				 // Banda máxima pelos pares de pacotes
deque<ALMTFBwmax> BwmaxList; // Lista que armazena os valores de pares de pacotes

//IP do Receptor capturado no início da execução
char* ReceptorIP;

/*Inserido por Alberto 15/0109: alteração na estrutura da estimativa de perdas*/
ALMTFLossEvent Ep;
ALMTFTimeEI TimeEI;  //ALMTFLossEvent, ALMTFTimeEI(09/02/09) antes declaradas em ALMTF_init agora são globais
pthread_mutex_t EventLoss_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t TimeEI_mutex = PTHREAD_MUTEX_INITIALIZER;
double Bwmin;
/********************/

//ALTERAÇÂO_PERDAS_VETOR
//int Num_perdas  = 0;


pthread_t IDThread_Bwmax;
pthread_t IDThread_logBw;
pthread_mutex_t cptRTT_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cptNumloss_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef GRAPH_JAVAINTERFACE
GRAPHInfo Info;
#endif

//FIM DA VARIÁVEIS GLOBAIS

#ifdef _LOG_LOG
void log_printAll(ALMstate *ALM_state, double *Bwjanela, double *Bweq, double *Cwnd)
{
	stringstream ss("");
	ss << "====================================" <<endl;
	ss << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\t" <<endl;
	ss << "\t Bwmax = " << Bwmax <<endl;
	ss << "\t Bwjanela = " << *Bwjanela <<endl;
	ss << "\t Bweq = " << *Bweq <<endl;
	ss << "\t Cwnd = " << *Cwnd <<endl;
	unsigned int bwatual = core_getRatesCumCurrent();
	ss << "\t Bwatual = "<<bwatual<<endl;
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);
	ss << "\t RTT = " << dbRTT <<endl;
	ss << "\t Numrec = " << Numrec <<endl;
	ss << "\t IntFeedback = " << IntFeedback <<endl;
	ss << "\t Numloss = " << ALMTF_getNumloss() <<endl;
	ss << "\t Sincjoin = " << Sincjoin_recvCp << endl;
	if (*ALM_state == ALM_STATE_START)
		ss << "\t ALM_state = ALM_STATE_START" <<endl;
	else
		ss << "\t ALM_state = ALM_STATE_STEADY" <<endl;
	ss << "====================================" <<endl;
	ofs_log << ss.str();
}
#endif

#ifdef _LOG_BW
void *ALMTF_threadLogBw(void *)
{
	struct timeval next;
	next.tv_sec = 0;
	next.tv_usec = 0;
	struct timeval now;
	now.tv_sec = 0;
	now.tv_usec = 0;
	char b[20];
	while(true) {		
		time_getus(&now);
		if (now.tv_sec > next.tv_sec ||
			(now.tv_sec == next.tv_sec && now.tv_usec > next.tv_usec)) {
			double  dbRTT;
			struct timeval tpRTT = ALMTF_getRTT(); 
			dbRTT = time_timeval2double(&tpRTT);			
			int btmp = now.tv_usec;
			memset(b, 0, sizeof(b));			
			sprintf(b, "%.6d", btmp);			
			ofs_logBw << now.tv_sec << "," << b;
			unsigned int Bwatual = core_getRatesCumCurrent();
			ofs_logBw<<"\tBwmax = "<<Bwmax<<"\tBwatual = "<<Bwatual<<"\tRTT = "<<dbRTT<<"\tNumloss = "<<ALMTF_getNumloss()<<endl;
			next.tv_usec += 200000;
			if (next.tv_usec > 1000000) {
				next.tv_usec -= 1000000;
				next.tv_sec++;
			}
		} else 
			usleep(25000);							
	}
}
#endif

void ALMTF_InitTimeEI(){
	pthread_mutex_lock(&TimeEI_mutex);
		TimeEI.NoColateral.tv_sec = 0;
		TimeEI.NoColateral.tv_usec = 0;
		TimeEI.AddLayerWait.tv_sec = 0;
		TimeEI.AddLayerWait.tv_usec = 0;

		struct timeval time_lastPkt;
		core_getLastPktTime(&time_lastPkt);
		TimeEI.AddLayerWait = time_lastPkt;
		time_add_ms(&TimeEI.AddLayerWait, ALMTF_T_STAB, &TimeEI.AddLayerWait);

		struct timeval now;
		time_getus(&now);
		TimeEI.Stab.tv_sec = 0;
		TimeEI.Stab.tv_usec = 0;
		TimeEI.Stab = now;
		struct timeval tpRTT = ALMTF_getRTT();
		time_add(&TimeEI.Stab, &tpRTT, &TimeEI.Stab);
		//Time_stab = espera um RTT com cwnd em "1", tal qual o TCP.
	pthread_mutex_unlock(&TimeEI_mutex);
}

void ALMTF_SetTimeEI(long int t, ALMTF_STAB n){
	struct timeval now;
	time_getus(&now);
	pthread_mutex_lock(&TimeEI_mutex);
		switch(n){
			case STAB:	TimeEI.Stab = now;
						time_add_ms(&TimeEI.Stab, t, &TimeEI.Stab);
				break;
			case NOCL:  TimeEI.NoColateral = now;
						time_add_ms(&TimeEI.NoColateral, t, &TimeEI.NoColateral);
				break;
			case ADDL:  TimeEI.AddLayerWait = now;
						time_add_ms(&TimeEI.AddLayerWait, t, &TimeEI.AddLayerWait);
				break;
		}
	pthread_mutex_unlock(&TimeEI_mutex);
}

bool ALMTF_GetTimeEI(){
	struct timeval now;
	time_getus(&now);
	bool b = false;
	pthread_mutex_lock(&TimeEI_mutex);
		if((time_compare(&(TimeEI.Stab), &now) > 0)
#ifdef SINCRONISMO_JOIN
		   || (time_compare(&(TimeEI.NoColateral), &now) > 0)
#endif
		   )
			b = true;
	pthread_mutex_unlock(&TimeEI_mutex);
	return b;
}

bool ALMTF_GetTimeEI(ALMTF_STAB n){
	struct timeval now;
	time_getus(&now);
	bool b = false;
	pthread_mutex_lock(&TimeEI_mutex);
		switch(n){
			case STAB:  if (time_compare(&(TimeEI.Stab), &now) > 0)
							b = true;
				break;
			case NOCL:  if (time_compare(&(TimeEI.NoColateral), &now) > 0)
							b = true;
				break;
			case ADDL:  if (time_compare(&(TimeEI.AddLayerWait), &now) > 0)
							b = true;
				break;
		}
	pthread_mutex_unlock(&TimeEI_mutex);
	return b;
}

void ALMTF_init()
{

// ALTERACAO_LOG_P
#ifdef _LOG_P
	ofs_logP.open("LOG_P.txt");
#endif

#ifdef _LOG_LOG
	ofs_log.open("LOG.txt");	
	ofs_log << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\t" << "Inicializando o ALMTF"<<endl;
	ofs_log.setf(ios::fixed, ios::floatfield);
	ofs_log.precision(2);
#endif
#ifdef _LOG_NUMLOSS
	ofs_logNumloss.open("LOG_Numloss.txt");
#endif
#ifdef _LOG_RECV_TESTE
	ofs_logRecvTeste.open("LOG_Recv_Teste.txt");
#endif
#ifdef _LOG_BW
	ofs_logBw.open("LOG_Bw.txt");
	ofs_logBw.setf(ios::fixed, ios::floatfield);
	ofs_logBw.precision(6);	
#endif
#ifdef _LOG_TIMEOUT
	ofs_logTimeout.open("LOG_Timeout.txt");
	ofs_logTimeout.setf(ios::fixed, ios::floatfield);
	ofs_logTimeout.precision(2);
#endif
#ifdef _LOG_EI
	ofs_logEI.open("LOG_EI.txt");
	ofs_logEI.setf(ios::fixed, ios::floatfield);
	ofs_logEI.precision(2);
#endif
#ifdef _LOG_EP
	ofs_logEp.open("LOG_Ep.txt");
#endif
#ifdef _LOG_BWJANELA
	ofs_logBwjanela.open("LOG_Bwjanela.txt");
	ofs_logBwjanela.setf(ios::fixed, ios::floatfield);
	ofs_logBwjanela.precision(2);
#endif
#ifdef _LOG_CWND
	ofs_logCwnd.open("LOG_Cwnd.txt");
	ofs_logCwnd.setf(ios::fixed, ios::floatfield);
	ofs_logCwnd.precision(2);
#endif

#ifdef LOG_PERDAS
	ofs_logPerdas.open("LOG_Perdas.txt");
#endif

/*
 * inicialização dos logs do mecanismo de aprendizado
*/
#ifdef _LOG_FAILURES
	ofs_logFailures.open("LOG_Failures.txt");
	ofs_logFailures.setf(ios::fixed, ios::floatfield);
	ofs_logFailures.precision(2);
#endif
#ifdef _LOG_NETSTATE
	ofs_logNet.open("LOG_Netstate.txt");
	ofs_logNet.setf(ios::fixed, ios::floatfield);
	ofs_logNet.precision(2);
#endif
#ifdef _LOG_LAYERS
	ofs_logLayers.open("layersData.dat");
	ofs_logLayers.setf(ios::fixed, ios::floatfield);
	ofs_logLayers.precision(2);
#endif

	ALMstate ALM_state;		// Estado do algoritmo (Start ou Steady)
	//ALMTFTimeEI TimeEI;
	// Tempo para estabilização quando sai de uma camada ou dá timeout
	// Tempo para controle dos efeitos colaterais de subida de camada em outros receptores (? conferir)
	// Tempo que deve ser esperado em addLayer para se cadastrar em uma nova camada
	double Cwnd;				// tamanho da janela
	double Bweq;				// Banda máxima pela equação				// TODO verificar o tipo de dado
	double Bwjanela;		// Banda controlada por Bwmax e Bweq		// TODO verificar o tipo de dado
	t_vinda.tv_sec = 0;
	t_vinda.tv_usec = 0;
	t_assim.tv_sec = 0;
	t_assim.tv_usec = 0;
	Cwnd = 1.0;		// Decisão de implementação: manter cwnd em pacotes e começando em "1"
    	Bwjanela = 0;	// Valor qualquer menor que taxa da primeira camada
	// ALTERACAO_STEADY
    	ALM_state = ALM_STATE_STEADY; 
	ReceptorIP = new char[16];
	bzero(ReceptorIP, sizeof(ReceptorIP));
	ALMTF_setIP(ReceptorIP);
	cerr << "Receptor IP : " << ReceptorIP << endl;
	ALMTF_InitTimeEI();		//inserido em função de modificações na estimativa de perdas 09/02/09
	ALMTF_setNumloss(0);
	LastReportSent.tv_sec = 0;
	LastReportSent.tv_usec = 0;
	IntFeedback = 0;
	Sincjoin_recv = 0;
	Sincjoin_recvCp = 0;
	RTT.tv_sec = 0;
	RTT.tv_usec = ALMTF_RTT*1000;
    Numrec = 0;
    FiltraRTT = false; // avisa que eh a primeirissima medida de RTT e deve atribuir RTT e nao filtrar
   	// Inicializa ALMTF_eqn, responsavel por determinar bunsigned int Bwatual = core_getRatesCumCurrent();anda via equacao do TCP
	ALMTF_inicializa_eqn();
	Bweq = 1000.0;	// valor inicial para funcionar o start-state
	core_addLayer(); // se registra na primeira camada
	if (pthread_create(&IDThread_Bwmax, NULL, ALMTF_calcbwmax, (void *)NULL) != 0) {
		cerr << "Error Creating Thread!" << endl;	
	}	

/*
 * inicio de bloco de código do mecanismo de aprendizado
*/
#ifdef _LOG_LAYERS
	stringstream logSS("");
	char _b[20];
	memset(_b,0,sizeof(_b));
	struct timeval _timenow;
	time_getus(&_timenow);
	sprintf(_b, "%.6d", (int)_timenow.tv_usec);
	logSS << _timenow.tv_sec << "," << _b << "\t ";
	logSS << core_getCurrentLayer() << endl;
	ofs_logLayers << "# Arquivo com dados para o Gnuplot" << endl;
	ofs_logLayers << "# Tempo\t\tCamada" << endl;
	ofs_logLayers << logSS.str();
#endif
#ifdef NETSTATE
	netList.resize(core_getTotalLayers());
	for (int i = 0; i < netList.size();i++){		
		netList[i].resize(NUM_ELEM);
		for (int j = 0; j < NUM_ELEM; j++)
			init_netvars(&netList[i][j]);
	}
#ifdef _LOG_NETSTATE	
	ofs_logNet << "netList.size() = " << netList.size() << endl;
#endif	
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado
*/

#ifdef _LOG_BW
	// thread pra fazer o log de Bwjanela, bweq e bwmax
	if (pthread_create(&IDThread_logBw, NULL, ALMTF_threadLogBw, (void *)NULL) != 0) {
		cerr << "Error Creating Thread!" << endl;	
	}
#endif
#ifdef _LOG_EI
	char b[20];
	memset(b, 0,sizeof(b));	
	struct timeval log_time_now;
	time_getus(&log_time_now);
	stringstream log_ss("");
	sprintf(b, "%.6d", (int)log_time_now.tv_usec);
	log_ss << "AAL_V130_090209 - ALMTF alterado por Alberto, ALMTF base fase 2(Threads e banda por pacotes por segundo em Kbits/s, com correções na estimativa de perdas e banda da equação, correções dos limites de Cwnd, RTT e Bwjanela 27/01/09 e limites de Bweq vinculado a Bwmax),\n com estimativa de banda exclusivamente por equação, sem mecanismo de aprendizado\n";
	log_ss << "===================================================================================================================================================================================" << endl;	
	log_ss << "================================================================================ Valores Iniciais =================================================================================" << endl;
	log_ss << "Tempo\t" << log_time_now.tv_sec << "," << b << "\t";
	unsigned int Bwatual = core_getRatesCumCurrent();
	log_ss << "Bwatual\t" << Bwatual << "\t";
	memset(b, 0,sizeof(b));
	sprintf(b, "%.6d", (int)RTT.tv_usec);
	log_ss << "RTT\t" << RTT.tv_sec << "," << b << "\t";
	log_ss << "Bweq\t" << utils_log_double2string(Bweq) << "\t";
	log_ss << "Fase\t" << ALM_state << "\t";
	log_ss << "Sincjoin\t" << Sincjoin_recvCp << "\t";	
	log_ss << "Bwmax\t" << Bwmax << "\t";	
	log_ss << "Cwnd_A\t" << Cwnd << "\t";
	log_ss << "Bwjanela_A\t" << Bwjanela << "\t";
	log_ss << "Bwup\t" << core_getRatesCum(core_getCurrentLayer()+1) << "\t";
	log_ss << "Cwnd_B\t" << Cwnd << "\t";
	log_ss << "Bwjanela_B\t" << Bwjanela << endl;
	log_ss << "===================================================================================================================================================================================" << endl;	
	ofs_logEI << log_ss.str();
	ofs_logEI.flush();
#endif
	// inicia a execução do algoritmo
	struct timeval Twait;
	while (1) {
#ifdef _LOG_LOG
		ofs_log << "========== Inicio ALMTF_EI(): =========="<< endl << "Now: " << time_getnow_sec() <<","<< time_getnow_usec() <<"us"<< endl;
		log_printAll(&ALM_state, &Bwjanela, &Bweq, &Cwnd);
#endif
#ifdef _LOG_CWND
		ofs_logCwnd << "========== Inicio ALMTF_EI(): ==========" << endl;
		ofs_logCwnd << "Now: " << time_getnow_sec() <<","<< time_getnow_usec() <<"us"<< endl;		
		ofs_logCwnd << "Inicial Cwnd=" << Cwnd << endl;
#endif
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "========== Inicio ALMTF_EI(): ==========" << endl;
		ofs_logBwjanela << "Now: " << time_getnow_sec() <<","<< time_getnow_usec() <<"us"<< endl;
		ofs_logBwjanela << "Inicial Bwjanela=" << Bwjanela << endl;
#endif
		ALMTF_EI(&Cwnd,&Bwjanela,&Bweq,&ALM_state);

/*
 * inicio de bloco de código do mecanismo de aprendizado
*/
#ifdef _LOG_LAYERS
		ofs_logLayers.flush();
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado
*/

#ifdef GRAPH_JAVAINTERFACE
		Info.Bweq = Bweq;
		Info.Bwjanela = Bwjanela;
#endif	
#ifdef _LOG_EI
		ofs_logEI << "Cwnd_B\t" << utils_log_double2string(Cwnd, 5, 2) << "\t";
		ofs_logEI << "Bwjanela_B\t" << utils_log_double2string(Bwjanela) << endl;
		ofs_logEI.flush();
#endif
#ifdef _LOG_CWND
		ofs_logCwnd << "Final Cwnd=" << Cwnd << endl;
		ofs_logCwnd << "========== Fim ALMTF_EI(): ==========" << endl;		
#endif
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "Final Bwjanela=" << Bwjanela << endl;
		ofs_logBwjanela << "========== Fim ALMTF_EI(): ==========" << endl;		
#endif
		Twait.tv_sec=0;
		Twait.tv_usec=0;
		struct timeval tpRTT = ALMTF_getRTT(); 
		time_mul(&tpRTT, 1, &Twait);	//TESTANDO - EXECUTAR EI A CADA RTT
		//time_mul(&tpRTT, Cwnd, &Twait);
		//time_mul(&Twait, ALMTF_EIMULTIPLIER, &Twait);
#ifdef _LOG_LOG
		log_printAll(&ALM_state, &Bwjanela, &Bweq, &Cwnd);
		//ofs_log << "Now: " << time_getnow_sec() <<","<<time_getnow_usec()<<"us\t Vai dormir RTT*Cwnd*N = "<<(Twait.tv_sec*1000000+Twait.tv_usec)<<"us"<<endl;
		ofs_log << "Now: " << time_getnow_sec() <<","<<time_getnow_usec()<<"us\t Vai dormir RTT = "<<(Twait.tv_sec*1000000+Twait.tv_usec)<<"us"<<endl;	//TESTANDO - EXECUTAR EI A CADA RTT
		ofs_log << "========== Fim ALMTF_EI(): ==========" << endl;
#endif
		time_usleep(&Twait);	
	}
#ifdef _LOG_LOG
	ofs_log.close();
#endif
#ifdef _LOG_BW
	ofs_logBw.close();
#endif
#ifdef _LOG_NUMLOSS
	ofs_logNumloss.close();
#endif
#ifdef _LOG_RECV_TESTE
	ofs_logRecvTeste.close();
#endif
#ifdef _LOG_EI
	ofs_logEI.close();
#endif
#ifdef _LOG_EP
	ofs_logEp.close();
#endif
#ifdef _LOG_BWJANELA
	ofs_logBwjanela.close();
#endif
#ifdef _LOG_CWND
	ofs_logCwnd.close();
#endif

#ifdef LOG_PERDAS
	ofs_logPerdas.close();
#endif

#ifdef _LOG_TIMEOUT
	ofs_logTimeout.close();
#endif


/*
 *encerramento dos arquivos de log do mecanismo de aprendizado
*/
#ifdef _LOG_FAILURES
	ofs_logFailures.close();
#endif
#ifdef _LOG_NETSTATE
	ofs_logNet.close();
#endif
#ifdef _LOG_LAYERS
	ofs_logLayers.close();
#endif
}

void ALMTF_SetBw(double banda){
	if (ALMTF_PP == true) { // Com pares de pacotes
		Bwmax = banda;
    } else {		
		Bwmax = 10000000; // coloca valor alto para nao mexer muito no codigo ??
		//Bwmax = 3000; // teste
    }
	Bwmin = banda;
}

void ALMTF_EI(double *Cwnd, double *Bwjanela, double *Bweq, ALMstate *ALM_state)
{
	// tempo de início do algoritmo
	struct timeval tstart;
	time_getus(&tstart);
	char b[20];
	unsigned int Bwatual = core_getRatesCumCurrent();
	double cwnd_max;	
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);
	cwnd_max = (Bwmax * dbRTT*1000)/(ALMTF_PACKETSIZE * 8);
	if (cwnd_max < 1.0)
		cwnd_max = 1.0;

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef _LOG_LAYERS
		stringstream logSS("");
		char _b[20];
		memset(_b,0,sizeof(_b));
		struct timeval _timenow;
		time_getus(&_timenow);
		sprintf(_b, "%.6d", (int)_timenow.tv_usec);
		logSS << _timenow.tv_sec << "," << _b << "\t ";
		logSS << core_getCurrentLayer() << endl;
		ofs_logLayers << logSS.str();
#endif
#ifdef LEARNING
	if (time_compare(&tstart,&t_failure) > 0)
		stable_layer = core_getCurrentLayer();
#endif
/*
 * fim de código do mecanismo de aprendizado 
*/

#ifdef _LOG_EI
	stringstream logEI_ss("");
	memset(b, 0,sizeof(b));
	struct timeval log_time;
	time_getus(&log_time);
	sprintf(b, "%.6d", (int)log_time.tv_usec);	
	logEI_ss << "Tempo\t" << log_time.tv_sec << "," << b << "\t";
	logEI_ss << "Bwatual\t" << Bwatual << "\t";
#endif		
	struct timeval TimePkt;	
	core_getLastPktTime(&TimePkt);
#ifdef _LOG_EI
	memset(b, 0,sizeof(b));	
	sprintf(b, "%.6d", (int)tpRTT.tv_usec);
	logEI_ss << "RTT\t" << tpRTT.tv_sec << "," << b << "\t";
#endif
	// ALTERACAO_BWEQ_T
	*Bweq = (ALMTF_estimabanda(TimePkt,Bweq,tpRTT))*1.086 ;
#ifdef _LOG_EI
	// ALTERACAO_BWEQ_T
	logEI_ss << "BWEQ_T\t" << utils_log_double2string(*Bweq) << "\t";
	ofs_logEI << logEI_ss.str();
#endif
	// Efetua estimativa de banda atraves da equacao do TCP, armazenando na variavel banda_eqn
	double cwndant;
	//if (time_compare(&TimeEI->Stab, &tstart) > 0) { //alterado em função de modificações na estimativa de perdas
	if (ALMTF_GetTimeEI(STAB)) {
#ifdef _LOG_CWND
		ofs_logCwnd << "if (time_compare(&TimeEI->Stab, &tstart) > 0)" << endl;
#endif
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd, cwnd_max);	//TEMP função removida, a princípio não é pra ter.
		ALMTF_setNumloss(0);
		// Sincjoin_recv=0;
#ifdef _LOG_LOG
		ofs_log << time_getnow_sec() <<","<<time_getnow_usec()<<"\t Time_stab > tstart, vai esperar..."<<endl;
		ofs_log << "\t Time_stab = "<<TimeEI->Stab.tv_sec<<","<<TimeEI->Stab.tv_usec<<endl;
		ofs_log << "\t Tstart = "<<tstart.tv_sec<<","<<tstart.tv_usec<<endl;
#endif
		return;
	}	
	// se está estabilizando pois outro receptor se cadastrou em uma camada,
	// ignora as perdas
	//if (time_compare(&TimeEI->NoColateral, &tstart) > 0) { alterado em função de modificações na estimativa de perdas 09/02/09
#ifdef SINCRONISMO_JOIN
	if (ALMTF_GetTimeEI(NOCL)) {
		ALMTF_setNumloss(0);
	}
#endif

	if (Bwmax == 0) return;

	unsigned int bwup;
	if (core_getCurrentLayer() == core_getTotalLayers()-1)
		bwup = Bwatual;
	else {
		bwup = core_getRatesCum(core_getCurrentLayer()+1);
	}

	cwndant = *Cwnd;
#ifdef _LOG_CWND
		ofs_logCwnd << "--> cwndant=" << cwndant << endl;
#endif

   // Timeout retirado para versão somente com equação
	/*if (ALMTF_timeout()) {
#ifdef _LOG_CWND
		ofs_logCwnd << "if (ALMTF_timeout())\t\t Cwnd=" << *Cwnd << endl;
#endif
		*Cwnd = 1.0;
		ALMTF_reduzbanda(Cwnd,Bwjanela);
		//TimeEI->Stab = tstart;
		//time_add_ms(&TimeEI->Stab, ALMTF_T_STAB, &TimeEI->Stab);  //alterado em função de modificações na estimativa de perdas 09/02/09
		ALMTF_SetTimeEI(ALMTF_T_STAB, STAB);
	}*/

	switch (*ALM_state) {
		case ALM_STATE_START:
#ifdef _LOG_EI
			ofs_logEI << "Fase\t" << *ALM_state << "\t";
#endif
#ifdef _LOG_LOG
			ofs_log <<"\t ALMTF_STATE = START"<<endl;
#endif
			if (ALMTF_getNumloss() == 0) {
#ifdef _LOG_LOG
				ofs_log << "\t NUMLOSS == " << ALMTF_getNumloss() <<endl;
#endif
				*Cwnd *= 1.5;
#ifdef _LOG_CWND
				ofs_logCwnd << "Cwnd *= 1.5\t\tCwnd=" << *Cwnd << endl;
#endif
				
				if (*Cwnd > cwnd_max) {
					*Cwnd = cwnd_max;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = cwnd_max\t\tCwnd=" << *Cwnd << endl;
#endif					
				}	

				/*if (*Cwnd > ALMTF_WINDOWSIZE) {
					*Cwnd = ALMTF_WINDOWSIZE;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = ALMTF_WINDOWSIZE\t\tCwnd=" << *Cwnd << endl;
#endif					
				}*/	
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "case ALM_STATE_START\tif (Numloss == 0)" << endl;
#endif	
				*Bwjanela = ALMTF_win2rate(Cwnd); // lixo
#ifdef _LOG_LOG
				ofs_log << "\t Cwnd foi para "<<*Cwnd<<endl;
				ofs_log << "\t Bwjanela foi para "<<*Bwjanela<<endl;
#endif
			} else {
#ifdef _LOG_LOG
				ofs_log << "NUMLOSS == "<< ALMTF_getNumloss() << endl;
#endif
				*Cwnd *= 0.7;
#ifdef _LOG_CWND
				ofs_logCwnd << "Cwnd *= 0.7\t\tCwnd=" << *Cwnd << endl;
#endif
				ALMTF_reduzbanda(Cwnd, Bwjanela);
				*ALM_state = ALM_STATE_STEADY;
			}
#ifdef _LOG_LOG
			log_printAll(ALM_state, Bwjanela, Bweq, Cwnd);
			ofs_log << "\t FIM START" << endl;	
#endif
			break;
		case ALM_STATE_STEADY:
#ifdef _LOG_EI
			ofs_logEI << "Fase\t" << *ALM_state << "\t";
#endif
#ifdef _LOG_LOG
			ofs_log << "\t ALMTF_STATE = STEADY"<<endl;
#endif
			if (ALMTF_getNumloss() == 0) {
#ifdef _LOG_LOG
				ofs_log << "\t NUMLOSS == " << ALMTF_getNumloss() <<endl;
#endif
				*Cwnd += 0.1;
#ifdef _LOG_CWND
				ofs_logCwnd << "Cwnd += 0.1\t\tCwnd=" << *Cwnd << endl;
#endif

				if (*Cwnd > cwnd_max) {
					*Cwnd = cwnd_max;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = cwnd_max\t\tCwnd=" << *Cwnd << endl;
#endif					
				}	
				/*if (*Cwnd > ALMTF_WINDOWSIZE) {
					*Cwnd = ALMTF_WINDOWSIZE;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = ALMTF_WINDOWSIZE\t\tCwnd=" << *Cwnd << endl;
#endif					
				}*/
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "case ALM_STATE_STEADY\tif (Numloss == 0)" << endl;
#endif	
				*Bwjanela = ALMTF_win2rate(Cwnd);				
			} else {
#ifdef _LOG_LOG
				ofs_log << "\t NUMLOSS == "<<ALMTF_getNumloss()<<endl;
#endif
				*Cwnd -= (*Cwnd)*0.05;
#ifdef _LOG_CWND
				ofs_logCwnd << "Cwnd -= Cwnd*0.05\t\tCwnd=" << *Cwnd << endl;
#endif				
				ALMTF_reduzbanda(Cwnd, Bwjanela);
			}
#ifdef _LOG_LOG
			log_printAll(ALM_state, Bwjanela, Bweq, Cwnd);
			ofs_log << "\t FIM STEADY" << endl;
#endif
			break;
	}

	if (*Bwjanela > 2*(*Bweq)) {
#ifdef _LOG_LOG
		ofs_log << "\t Bwjanela > 2*Bweq, limitando conforme Bweq"<<endl;
#endif
		*Bwjanela = 2*(*Bweq);	//TEMP removida proteção de bweq
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "if (Bwjanela > 2*Bweq)\t\tBwjanela=" << *Bwjanela << endl;
#endif	
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd, cwnd_max);
	} else if (*Bwjanela < (*Bweq)/2) {
#ifdef _LOG_LOG
				ofs_log << "\t Bwjanela < Bweq/2, limitando conforme Bweq"<<endl;
#endif
				*Bwjanela = (*Bweq)/2;  //TEMP removida proteção de bweq
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "if (Bwjanela < Bweq/2)\t\tBwjanela=" << *Bwjanela << endl;
#endif				
				*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd, cwnd_max);
			}
#ifdef _LOG_LOG
	log_printAll(ALM_state, Bwjanela, Bweq, Cwnd);	
#endif
#ifdef _LOG_EI
	ofs_logEI << "Sincjoin\t" << Sincjoin_recvCp << "\t";
	ofs_logEI << "Bwmax\t" << utils_log_double2string(Bwmax,1,2) << "\t";
#endif	
#ifdef _LOG_LOG
			ofs_log << "\t Bwjanela = "<<*Bwjanela<<"\t Bwup = "<<bwup<<endl;;
#endif
	// TRATA SUBIR CAMADA
#ifdef _LOG_EI
		ofs_logEI << "Cwnd_A\t" << utils_log_double2string(*Cwnd, 5, 2) << "\t";
		ofs_logEI << "Bwjanela_A\t" << utils_log_double2string(*Bwjanela) << "\t";
		ofs_logEI << "Bwup\t" << right << setw(4) << bwup << "\t";
#endif
	//if (*Bwjanela > bwup) {		anterior
	if (*Bweq > bwup){
		if (Sincjoin_recv > core_getCurrentLayer() || *ALM_state == ALM_STATE_START) {
#ifdef _LOG_LOG
			ofs_log << "if (Sincjoin_recv > core_getCurrentLayer() || *ALM_state == ALM_STATE_START)" << endl;
			ofs_log << "Sincjoin = " << Sincjoin_recv << "\tCamada Atual = " << core_getCurrentLayer() << endl;
#endif
/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef NETSTATE
			NETSTATEvars netVars;		
			save_netvars(&netVars);
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado 
*/
			int r = ALMTF_addLayer(Bweq, Bwjanela, Cwnd);
			if (r == 1) {	
/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef NETSTATE
				time_getus(&t_failure);
				time_add_ms(&t_failure, 120000, &t_failure); // 120s = 2min
				netVars.expireTime = t_failure;	//Guarda o tempo do estado.
				netVars.falhou = false;			//Seta como false, pois pode nao falhar.
				netList[core_getCurrentLayer()][posAtual] = netVars;
				posAtual++;
				if (posAtual == NUM_ELEM)
					posAtual = 0;
#ifdef _LOG_NETSTATE
				ofs_logNet << "Subiu de camada! " << core_getCurrentLayer() << endl;
				ofs_logNet << "Guardou o estado da rede!" << endl;
				char auxb[20];
				memset(auxb, 0,sizeof(auxb));				
				sprintf(auxb, "%.6d", (int)t_failure.tv_usec);	
				ofs_logNet << "Tempo atual =              \t" << t_failure.tv_sec << "," << auxb << endl;
#endif
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado 
*/	
#ifdef _LOG_LOG
				ofs_log << "Camada Adicionada = " << core_getCurrentLayer() << endl;
#endif
				Sincjoin_recv = 0;

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef LEARNING
				time_getus(&t_failure);
#ifdef _LOG_FAILURES
				ofs_logFailures << "Subiu de camada!" << endl;
				char auxb[20];
				memset(auxb, 0,sizeof(auxb));				
				sprintf(auxb, "%.6d", (int)t_failure.tv_usec);	
				ofs_logFailures << "Tempo atual =              \t" << t_failure.tv_sec << "," << auxb << endl;
#endif

				time_add_ms(&t_failure, fail_time, &t_failure);
				// Tempo atual + x segundos
				// Se descer de camada x segundos apos a subida, considera  como uma falha

#ifdef _LOG_FAILURES				
				memset(auxb, 0,sizeof(auxb));				
				sprintf(auxb, "%.6d", (int)t_failure.tv_usec);
				ofs_logFailures << "Tempo atual + 2 segundos = \t" << t_failure.tv_sec << "," << auxb << endl;
				ofs_logFailures << endl;
#endif
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado 
*/			
			} else 
				if (r == -1) {
					*Cwnd = cwndant;	
#ifdef _LOG_LOG
					ofs_log << "Camada NAO Adicionada = " << "Cwnd = " << *Cwnd << endl;
#endif
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = cwndant\t\tCwnd=" << *Cwnd << endl;
#endif	
					*Bwjanela = ALMTF_win2rate(Cwnd); 
				}
		}
	} else {
#ifdef SINCRONISMO_JOIN
		if (Sincjoin_recv > core_getCurrentLayer()+1 && *ALM_state == ALM_STATE_STEADY) {
#ifdef _LOG_LOG
			ofs_log << "if (Sincjoin_recv > core_getCurrentLayer()+1 && *ALM_state == ALM_STATE_STEADY)" << endl;
			ofs_log << "Sincjoin = " << Sincjoin_recv << "\tCamada Atual = " << core_getCurrentLayer() << endl;
#endif
			//TimeEI->NoColateral = tstart;
			//time_add_ms(&TimeEI->NoColateral, 500, &TimeEI->NoColateral);  //alterado em função de modificações na estimativa de perdas 09/02/09
			ALMTF_SetTimeEI(500, NOCL);

#ifdef _LOG_LOG
			ofs_log << "TimeStart = " << tstart.tv_sec << "," << tstart.tv_usec << "\tTimeEI->NoColateral = " << TimeEI->NoColateral.tv_sec << "," <<  TimeEI->NoColateral.tv_usec << endl; 
#endif
		} 
#endif

	}
	//Faz um único join por vez
	//Sincjoin_recv = 0;
	// TRATA DESCER CAMADA
	if (core_getCurrentLayer() <= 0) return;

	//while (*Bwjanela < Bwatual) anterior
	while (*Bweq < Bwatual) {
#ifdef _LOG_LOG
		ofs_log << "while (Bweq < Bwatual)" << " Bwjanela = " << *Bwjanela << " Bwatual = " << Bwatual << " Bweq = " << *Bweq << endl;
		ofs_log << "if (Bweq >= Bwatual) RETURN";
#endif
		//if (*Bweq >= Bwatual) return;		//TEMP removida proteção de bweq
#ifdef _LOG_LOG
		ofs_log << "\t Vai DEIXAR camada! "<<core_getCurrentLayer()<<endl;
#endif
		core_leaveLayer();

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef _LOG_LAYERS
		stringstream logSS("");
		char _b[20];
		memset(_b,0,sizeof(_b));
		struct timeval _timenow;
		time_getus(&_timenow);
		sprintf(_b, "%.6d", (int)_timenow.tv_usec);
		logSS << _timenow.tv_sec << "," << _b << "\t ";
		logSS << core_getCurrentLayer() << endl;
		ofs_logLayers << logSS.str();
#endif
#ifdef NETSTATE
		struct timeval timenow;		
		time_getus(&timenow);
#ifdef _LOG_NETSTATE
		ofs_logNet << "Desceu de camada!" << endl;
		memset(b, 0,sizeof(b));				
		sprintf(b, "%.6d", (int)timenow.tv_usec);
		ofs_logNet << "Tempo Atual = \t" << timenow.tv_sec << "," << b << "\t" << endl;		
#endif
		// Se esta deixando camada antes do tempo estipulado
		// considera como sendo uma perda
		if (time_compare(&t_failure,&timenow) > 0){
#ifdef _LOG_NETSTATE
			ofs_logNet << "timenow < t_failure !! Falhou na tentativa de join!!" << "\t" << endl;			
#endif
			join_failures++;
			if (posAtual > 0)
				netList[core_getCurrentLayer()+1][posAtual-1].falhou = true;
			else netList[core_getCurrentLayer()+1][NUM_ELEM-1].falhou = true;
			// NUM_ELEM-1 Ã© igual ao Ãºltimo elemento.
#ifdef _LOG_NETSTATE
			ofs_logNet << "Salvou o estado atual da rede!\t" << endl << endl;			
#endif
		}
#endif
		
#ifdef LEARNING
		struct timeval timenow;
		time_getus(&timenow);
#ifdef _LOG_FAILURES		
		ofs_logFailures << "Desceu de camada!" << endl;
		memset(b, 0,sizeof(b));				
		sprintf(b, "%.6d", (int)timenow.tv_usec);
		ofs_logFailures << "Tempo Atual = \t" << timenow.tv_sec << "," << b << "\t" << endl;		
#endif
		// Se esta deixando camada antes do tempo estipulado
		// considera como sendo uma perda
		if (time_compare(&t_failure,&timenow) > 0){
			join_failures++;
#ifdef _LOG_FAILURES
			ofs_logFailures << "timenow < t_failure !! Falhou na tentativa de join!!" << "\t" << endl;			
#endif
		}
		if (stable_layer > core_getCurrentLayer()){
			stable_layer = core_getCurrentLayer();
			t_stab_mult = 1;
		}
#ifdef _LOG_FAILURES
		ofs_logFailures << "join_failures = " << join_failures << "\t" << endl;		
#endif
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado 
*/

#ifdef _LOG_LOG
		ofs_log << "\t DEIXOU A CAMADA! "<<core_getCurrentLayer()+1<<endl;
#endif
		//TimeEI->Stab = tstart;
		//time_add_ms(&TimeEI->Stab, ALMTF_T_STAB, &TimeEI->Stab);  //alterado em função de modificações na estimativa de perdas 09/02/09
		ALMTF_SetTimeEI(ALMTF_T_STAB, STAB);
#ifdef _LOG_LOG
		ofs_log << "TimeStart = " << tstart.tv_sec << "," << tstart.tv_usec << "\tTimeEI->Stab = " << TimeEI->Stab.tv_sec << "," <<  TimeEI->Stab.tv_usec << endl; 
#endif
		//Time_stab = tstart + ALMTF_T_STAB;
		if (core_getCurrentLayer() <= 0) return;
		Bwatual = core_getRatesCumCurrent();
		if (core_getCurrentLayer() == core_getTotalLayers()-1)
			bwup = Bwatual;
		else bwup = core_getRatesCum(core_getCurrentLayer()+1);

		*Bwjanela = (Bwatual+bwup)/2;
		*Bweq =  (Bwatual+bwup)/2;
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd, cwnd_max);
#ifdef _LOG_LOG
		ofs_log << "\t Novo Valor Bwjanela = (Bwatual+bwup)/2;"<< *Bwjanela <<endl;
		ofs_log << "\t Novo Valor Cwnd = "<< *Cwnd <<endl;
#endif
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "Bwjanela = (Bwatual+bwup)/2\t\tBwjanela=" << *Bwjanela << endl;
#endif	
	}
}

void ALMTF_inicializa_eqn()
{
	pthread_mutex_lock(&EventLoss_mutex);
		Ep.Ep = 1;
		Ep.Ep_pesos[0] = 1.0;
		Ep.Ep_pesos[1] = 1.0;
		Ep.Ep_pesos[2] = 1.0;
		Ep.Ep_pesos[3] = 1.0;
		Ep.Ep_pesos[4] = 0.8;
		Ep.Ep_pesos[5] = 0.6;
		Ep.Ep_pesos[6] = 0.4;
		Ep.Ep_pesos[7] = 0.2;
/*VP_CORRIGIDO -  anterior: for (int i = 0; i < 8; i++)*/
		for (int i = 0; i < 9; i++)
			Ep.Ep_pkt_rec[i] = 0;
		Ep.Ep_tstamp_ultperda.tv_sec = 0;
		Ep.Ep_tstamp_ultperda.tv_usec = 0;
	pthread_mutex_unlock(&EventLoss_mutex);
}

/** Transforma a taxa de kbits/s para o valor da janela. 
 *  Funcao que realiza o calculo da inversa da funcao win2rate. Transforma a banda em seu
 *  equivalente em janela de congestionamento, ocorre quando eh necessario adaptar Bwjanela
 *  de acordo com o valor obtido via equação TCP.
 *  Var Globais: Bwjanela, RTT;
 *  Const: PACKETSIZE, WINDOWSIZE;
 */
double ALMTF_rate2win(double *Bwjanela, double *Cwnd, double CwndMax) 
{
	double wnd = 0.0;
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);
	wnd = ((*Bwjanela) * dbRTT*1000)/(ALMTF_PACKETSIZE * 8);
	double aux = wnd;
	bool upd_wnd = 0;	
	//Proteção para Cwnd não variar mais do que 30%
	if ((wnd/(*Cwnd)) < 0.7 || (wnd/(*Cwnd)) > 1.3) {
		wnd = *Cwnd;				//TODO a wnd perde a sincronia com bwjanela!
		upd_wnd = 1;
	}
	if (wnd > CwndMax) {
		wnd = CwndMax;		//TODO a wnd perde a sincronia com bwjanela!
		upd_wnd = 1;		
	}		
	if (wnd < 1.0) {
		wnd = 1.0;					//TODO a wnd perde a sincronia com bwjanela!
		upd_wnd = 1;				
	}
	if(upd_wnd) {
		double* wnd_aux = new double(wnd);
#ifdef _LOG_BWJANELA
	ofs_logBwjanela << "rate2win! sincronizar em win2rate. Original " << *Cwnd << " calculado " << aux << " mudou para: " << wnd << endl;
#endif
		ALMTF_win2rate(wnd_aux); //UNST tenta recuperar a sincronia
		delete wnd_aux;
	}
#ifdef _LOG_CWND
	ofs_logCwnd << "\tALMTF_rate2win()\t\tCwnd=" << wnd << " RTT:=" << dbRTT << endl;
#endif
	return (wnd);
}

/*
 * Transforma o valor da janela para a taxa em kbits/s. 
 *  Funcao que realiza o calculo da banda utilizada em uma conexao TCP equivalente em
 *  um determinado tamanho de janela.
 *  Var Globais: Cwnd, RTT;
 *  Constantes: WINDOWSIZE, PACKETSIZE;
 */
double ALMTF_win2rate(double *Cwnd) 
{	
	double bws;
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);	
	if (dbRTT == 0.0) {
		bws = (((*Cwnd) * ALMTF_PACKETSIZE * 8)/0.000001);
	}
	else 
		bws = (((*Cwnd) * ALMTF_PACKETSIZE * 8)/(dbRTT*1000));
#ifdef _LOG_BWJANELA
	ofs_logBwjanela << "\tALMTF_win2rate()\t\tBwjanela=" << bws << " RTT: " << dbRTT << endl;
#endif
	/*if (bws > Bwmax) {	//UNST TODO tira a sincronia com Cwnd!
		bws = Bwmax;		//TESTANDO nao eh pra precisar mais disso
		//double* bws_aux = new double(bws);
		//ALMTF_rate2win(bws_aux, Cwnd); //UNST tenta recuperar a sincronia
		//delete bws_aux;
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "\tBwjanela = Bwmax\t\tBwjanela=" << bws << endl;
#endif				
	}*/		
	return (bws);
}

/*
 * Caracteriza timeout se nao chegou pacote em 10 vezes o tempo que deveria chegar.
 *  Exibe mensagem de Timeout na tela.
 *  Variaveis: TimeAtual e TimeUltPacote como parametros.
 */
bool ALMTF_timeout() {

	struct timeval now;	
	time_getus(&now);
	unsigned int bw_layer_zero = core_getRatesCum(0);	
	bool b = false;
	struct timeval calc;	
	double dbcalc = 0;
	dbcalc = ((8*10*(double)ALMTF_PACKETSIZE) / ((double)bw_layer_zero*1000)); //Resultado em ms, x0.001 para us
	calc = time_double2timeval(dbcalc);		
	struct timeval time_lastPkt;
	core_getLastPktTime(&time_lastPkt);
	time_add(&calc, &time_lastPkt, &calc);
	if (time_compare(&now,&calc) > 0) {
		core_sendPrintList("\tTimeout!\n");
		b = true;
#ifdef _LOG_TIMEOUT
		ofs_logTimeout << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\tDeu timeout!!"<<endl;
		ofs_logTimeout << "\t\t Bwatual Layer Zero..........: "<<bw_layer_zero<<endl;		
		ofs_logTimeout << "\t\t Time Last Packet Layer Zero.: "<<time_lastPkt.tv_sec<<","<<time_lastPkt.tv_usec<<"us"<<endl;
		ofs_logTimeout << "\t\t Time Now....................: "<<now.tv_sec<<","<<now.tv_usec<<"us"<<endl;
		ofs_logTimeout << "\t\t Time Calculo................: "<<calc.tv_sec<<","<<calc.tv_usec<<"us"<<endl;		
#endif
	}
	return b;
}

/** 
 *	Converte perdas para banda equivalente no receptor (bits/s).
 *  Assume que nao tem "delayed-ack" no TCP, portanto, cada pacote recebe um ACK.
 *  "p" representa a media das perdas.
 *  "RTT" representa o rtt medido pelo sistema.
 */
double ALMTF_perdas2banda(double p, struct timeval curRTT) 
{
// ALTERACAO_LOG_P
#ifdef _LOG_P
			char b[20];
			memset(b, 0, sizeof(b));
			sprintf(b, "%.6d", (int)time_getnow_usec());
			ofs_logP << time_getnow_sec() << "," << b << "\tp == " << p <<endl;
#endif

	double dbRTT = 0;
	dbRTT = time_timeval2double(&curRTT)*1000; //RTT em ms
	if (p < 0 || dbRTT < 0)
		return (-1);
	double p2b = 0;
	double tmp1 = 0;
	double tmp2 = 0;
	tmp1 = dbRTT * sqrt(2. * p/3.);
	tmp2 = 3*sqrt(3. * p/8.);

	//???
	/*if (tmp2 > 1.0)
		tmp2 = 1.0;*/

	tmp1 = tmp1 + tmp2*((4.*dbRTT) * p * (1 + 32.*p*p));
	/* Equacao TCP */
	p2b = (ALMTF_PACKETSIZE*8) / tmp1; 
	return (p2b);
}

/** 
 *	Calcula media dos ultimos 8 intervalos em que houveram perdas.
 *  "ini" inicio da medida no vetor de pacotes recebidos
 *  "end" final da medida no vetor de pacotes recebidos
 *  "vet[]" vetor para calculo da media
 *  "vetp[]" vetor de pesos para calculo da media
 */
double ALMTF_media_ult_intervalos()
{
	double soma = 0.0;
	double media1 = 0.0;
	double media2 = 0.0;
	int i, num_ev;
	pthread_mutex_lock(&EventLoss_mutex);
		if (Ep.Ep > 8) 
			num_ev = 8;
		else num_ev = Ep.Ep;
										
		for (i=0; i < num_ev; i++)
			soma = soma + Ep.Ep_pesos[i];

		for (i=0; i < num_ev; i++)	//considerando evento atual
			media1 = media1 + ((Ep.Ep_pkt_rec[i] * Ep.Ep_pesos[i])/soma);

/*VP_CORRIGIDO - anterior: if(num_ev > 1)
							  for(i=1; i < num_ev; i++) //desconsiderando evento atual
								 media2 = media2 + ((Ep.Ep_pkt_rec[i] * Ep.Ep_pesos[i])/soma);	*/
		if(num_ev > 1){
			if (Ep.Ep > 8) 
				num_ev++;
			for (i=1; i < num_ev; i++) //desconsiderando evento atual
				media2 = media2 + ((Ep.Ep_pkt_rec[i] * Ep.Ep_pesos[i-1])/soma);
		}
		
		if(media2 > media1)
			media1 = media2;

#ifdef _LOG_EP
		stringstream ss("");
		struct timeval now;
		time_getus(&now);
		ss << now.tv_sec <<","<<now.tv_usec<<"us:\t";
/*VP_CORRIGIDO - anterior: for (int i = 0; i < 8; i++)*/
		for (int i = 0; i < 9; i++) {
			ss << Ep.Ep_pkt_rec[i] << " -\t";
		}
			ss <<endl;
		ofs_logEp << ss.str();
#endif

	pthread_mutex_unlock(&EventLoss_mutex);

	return (1.0/media1);
}

/** 
 *	Reduz valor de Cwnd, zera perdas e calcula banda equivalente.
 *  Var Globais: Cwnd, Numloss e Bwjanela;
 */
void ALMTF_reduzbanda(double *Cwnd, double *Bwjanela) 
{
#ifdef _LOG_CWND
	ofs_logCwnd << "ALMTF_reduzbanda()\t\tCwnd=" << *Cwnd << endl;
#endif
	if (*Cwnd < 1.0)
		*Cwnd = 1.0;
#ifdef _LOG_CWND
	ofs_logCwnd << "\tif (*Cwnd < 2)\t\tCwnd=" << *Cwnd << endl; 
#endif
	ALMTF_setNumloss(0);
	*Bwjanela = ALMTF_win2rate(Cwnd);
#ifdef _LOG_BWJANELA
	ofs_logBwjanela<< "ALMTF_reduzbanda()\t\tBwjanela=" << *Bwjanela<< endl;
#endif
}

/** 
 *	Estima banda equivalente. Esta funcao organiza os parametros necessarios
 *  para serem utilizados na equacao. Funcao chamada a cada pacote recebido.
 */
double ALMTF_estimabanda(struct timeval timestamp, double *Bweq, struct timeval curRTT) 
{
	double banda = 0.0;
	double perdas = ALMTF_media_ult_intervalos();
	double dbRTT = time_timeval2double(&curRTT);
	banda = ALMTF_perdas2banda(perdas,curRTT);
	//cout << "time: " << time_timeval2double(&timestamp) << "\tbanda: " << banda  << "\tbandamax: " << Bwmax << "\tbandamin: " << Bwmin << endl;

	// ALTERACAO_FBWEQ
 	/*if(banda > Bwmax && Bwmax > 0)
		banda = Bwmax;
	if(banda < Bwmin)
		banda = Bwmin;*/

	return (banda);
}

/** Estima perdas para utilizacao na equacao de banda. A funcao estima duas taxas de perdas,
 *  uma considerando os pacotes do intervalo atual, e outra não.
 *  OBS: esta funcao eh chamada cada pacote recebido.
 *  eventos_perdas = Ep;
 *  tstamp_ultperda = Ep_tstamp_ultperda;
 *  pkt_rec = Ep_pkt_rec;
 */
void ALMTF_estimaperdas(struct timeval* timestamp, bool instant_loss)
{
	int i;
	int num_ev;
	double estim1 = 0;
	double estim2 = 0;	
	struct timeval curRTT = ALMTF_getRTT();

   // Acrescentado para evitar que o vetor seja modificado quando em estabilização
	if (ALMTF_GetTimeEI(STAB)) { 
		return; 
	}

	pthread_mutex_lock(&EventLoss_mutex);
		// ALTERAÇÂO_PERDAS_VETOR
		//if((ALMTF_getNumloss() > 0) && !(ALMTF_GetTimeEI())) {		
		if(instant_loss && !(ALMTF_GetTimeEI())) {
			struct timeval tmpsc;
			tmpsc.tv_sec = 0;
			tmpsc.tv_usec = 0;
			time_subtract(timestamp,&(Ep.Ep_tstamp_ultperda),&tmpsc);
			if (time_compare(&tmpsc,&curRTT) > 0) {
				// ALTERAÇÂO_PERDAS_VETOR
				//if(ALMTF_getNumloss() > Num_perdas){
					Ep.Ep_tstamp_ultperda = *timestamp;
					Ep.Ep++;
/*VP_CORRIGIDO - anterior: for (i=7; i>0; i--)*/
					for (i=8; i>0; i--)  
						Ep.Ep_pkt_rec[i] = Ep.Ep_pkt_rec[i-1];
					Ep.Ep_pkt_rec[0]=0;
#ifdef LOG_PERDAS
					ofs_logPerdas << "Evento de perda " << Ep.Ep - 1 << "\tTime: " << time_timeval2double(timestamp) << "\n\tVetor de Pacotes Recebidos:";
/*VP_CORRIGIDO - anterior: for(int i = 0; i < 8; i++)*/
					for(int i = 0; i < 9; i++)
						ofs_logPerdas << right << setw(6) <<  Ep.Ep_pkt_rec[i];
					ofs_logPerdas << "\n";
#endif
					// ALTERAÇÂO_PERDAS_VETOR
					//Num_perdas = 0;
					//ALMTF_setNumloss(0);
				//}

				// ALTERAÇÂO_PERDAS_VETOR
				/*else{
#ifdef LOG_PERDAS
					ofs_logPerdas << "Perda descartada: Pacotes perdidos em intervalo de RTT após Evento de perdas. ALMTF_getNumloss() = " << ALMTF_getNumloss() << endl;
#endif
				}*/

			}

			// ALTERAÇÂO_PERDAS_VETOR
			//else Num_perdas = ALMTF_getNumloss();
		}
		Ep.Ep_pkt_rec[0]++;
   
	pthread_mutex_unlock(&EventLoss_mutex);

}

//void ALMTF_calcbwmax_dir(double PP_value)
//{
//	if (ALMTF_PP != true)
//		return;
//	
//	if (PP_namostras < 10) {		
//		PP_buf[PP_namostras] = PP_value;
//		PP_somatotal += PP_value;
//		PP_namostras++;
//		if (PP_namostras == 10) {
//			Bwmax = PP_somatotal/10;			
//			PP_indice = 0;
//		}
//		return;
//	}
//	//Se PP_namoastras == 10 então faz a média. 
//	if ((PP_value > (Bwmax*0.7)) && (PP_value < (Bwmax*1.3))) {		
//		PP_descartou = 0; //Zera quantidade de valores fora da media (descartados).
//		PP_somatotal = (PP_somatotal - PP_buf[PP_indice]);
//		PP_somatotal = PP_somatotal + PP_value;
//		PP_buf[PP_indice] = PP_value; //Substitui valor do vetor pelo novo valor obtido.
//		Bwmax = (PP_somatotal/10); //Altera Bw maxima.
//	
//		PP_indice++;
//		if (PP_indice == 10) 
//			PP_indice = 0;
//		
//	}
//	else {
//		PP_descartou++;
//		if (PP_descartou > 10) { //Para recomecar tem que ter 10 descartes seguidos.			
//			PP_namostras = 0; //Recomeca novamente medicoes - ressincroniza.
//			PP_descartou = 0;
//			PP_somatotal = 0;
//		}
//	}
//
//}

void *ALMTF_calcbwmax(void *) 
{
	double PP_buf[10];
	int PP_namostras;
	int PP_indice;
	int PP_descartou;
	double PP_somatotal;
	//inicialização das variáveis para inferência da banda máxima.
	PP_namostras = 0;
	PP_descartou = 0;
	PP_somatotal = 0.0;
	PP_indice = 0;
	for (int i = 0; i < 10; i++)
		PP_buf[i] = 0.0;	

	if (ALMTF_PP != true)
		return(NULL);
	
#ifdef _LOG_BWMAX	
	ofs_logBwmax.open("LOG_Bwmax.txt");

#endif

	while (1) {		
			 while (!BwmaxList.empty()) {
#ifdef _LOG_BWMAX	
					stringstream ss("");
					ss<<time_getnow_sec()<<","<<time_getnow_usec()<<"us:\t ==== Inicio ALMTF_calcbwmax ===="<<endl;
#endif
					ALMTFBwmax PP_Bwmax;	
					PP_Bwmax = BwmaxList.front();
					BwmaxList.pop_front();

					if (PP_namostras < 10) {
						PP_buf[PP_namostras] = PP_Bwmax.PP_value;
						PP_somatotal += PP_Bwmax.PP_value;
						PP_namostras++;
						if (PP_namostras == 10) {
							Bwmax = PP_somatotal/10;							
							PP_indice = 0;
#ifdef _LOG_BWMAX
						ss << "\t Bwmax: "<<Bwmax<<endl;
#endif
						}
#ifdef _LOG_BWMAX
						ss << "\t inatividade: "<<Bwmax<<endl;
#endif
					} else {
						//Se PP_namoastras == 10 então faz a média. 
						if ((PP_Bwmax.PP_value > (Bwmax*0.7)) && (PP_Bwmax.PP_value < (Bwmax*1.3))) {		
#ifdef _LOG_BWMAX
							ss << "\t ADICIONOU PP_value: "<<PP_Bwmax.PP_value<<" Layer: "<<PP_Bwmax.layerID<<endl;
#endif					
							PP_descartou = 0; //Zera quantidade de valores fora da media (descartados).
							PP_somatotal = (PP_somatotal -	PP_buf[PP_indice]);
							PP_somatotal =	PP_somatotal + PP_Bwmax.PP_value;
							PP_buf[PP_indice] = PP_Bwmax.PP_value; //Substitui valor do vetor pelo novo valor obtido.
							Bwmax = (PP_somatotal/10); //Altera Bw maxima.
							PP_indice++;
							if (PP_indice == 10) 
								PP_indice = 0;			
						} else {
#ifdef _LOG_BWMAX
							ss << "\t DESCARTOU PP_value: "<<PP_Bwmax.PP_value<<" Layer: "<<PP_Bwmax.layerID<<endl;
#endif						
							PP_descartou++;
							if (PP_descartou > 10) { //Para recomecar tem que ter 10 descartes seguidos.			
								PP_namostras = 0; //Recomeca novamente medicoes - ressincroniza.
								PP_descartou = 0;
								PP_somatotal = 0;
#ifdef _LOG_BWMAX
								ss << "\t == 10 DESCARTES SEGUIDOS =="<<endl;
#endif							
							}
						}
					}
#ifdef _LOG_BWMAX
					ss << "\t PP_Buffer = ";
					for (int i = 0; i < 10; i++)
						ss << PP_buf[i] << " - ";
					ss<<endl;
					ss << "\t Bwmax Final = "<<Bwmax<<endl;
					ss << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\t ==== FIM ALMTF_calcbwmax ===="<<endl;
					ofs_logBwmax << ss.str();
#endif	
		}
		usleep(10000);
	}
#ifdef _LOG_BWMAX
	ofs_logBwmax.close();
#endif
#ifdef _WIN32_
	return (NULL);
#else 
	pthread_exit(NULL);
#endif
}

/*
 *	ALMTF_add_layer.
 *  Adiciona camada se banda permitir, controla banda e o tempo que pode ou não pode subir. 
 *  novabw = Bwjanela	
 */
int ALMTF_addLayer (double *Bweq, double *novabw, double *Cwnd) 
{	
	//cout << "ALMTF_addLayer chamado!\n";
	struct timeval timenow;
	time_getus(&timenow);
	//if (time_compare(Time_addLayerWait,&timenow) > 0) //Aguarda antes de subir novamente.  
		//alterado em função de modificações na estimativa de perdas 09/02/09

   //Alterado pois ADDL não será usada nesta versão
	//if(ALMTF_GetTimeEI(ADDL))
		//return -1;									   //timenow < Time_addLayerWait

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef NETSTATE
	if ((core_getCurrentLayer() < core_getTotalLayers()-1) && (join_failures > NUM_ELEM)){		
		for (int k = 0 ; k < NUM_ELEM; k++){
			// Se expirou o tempo deste estado passa para o prÃ³ximo
			if (time_compare(&netList[core_getCurrentLayer()+1][k].expireTime,&timenow) < 0){
#ifdef _LOG_NETSTATE
					ofs_logNet << "Tempo Expirado, proximo estado!" << endl;
#endif
					continue;
			}
	
			if (netList[core_getCurrentLayer()+1][k].falhou == true){
				//SÃ³ compara os estados se o estado guardado Ã© de uma falha.
				double failrtt = netList[core_getCurrentLayer()+1][k].rtt;
                double failperdas = netList[core_getCurrentLayer()+1][k].perdas;                
                struct timeval tmpRTT = ALMTF_getRTT();
                double RTTatual = time_timeval2double(&tmpRTT);
                double perdasatual = ALMTF_getPerdas();
#ifdef _LOG_NETSTATE
                ofs_logNet << "***** Estado De Falha *****" << endl;
                ofs_logNet << "RTT*1000000 = " << failrtt*1000000 << endl;
                ofs_logNet << "perdas = " << failperdas << endl;				
                ofs_logNet << "***************************" << endl;
                ofs_logNet << "****** Estado Atual  ******" << endl;
                ofs_logNet << "RTT*1000000 = " << RTTatual*1000000 << endl;
                ofs_logNet << "perdas = " << perdasatual << endl;
                ofs_logNet << "***************************" << endl;
#endif
				/* Testa as novas condicoes da rede.
				 * Se elas forem parecidas com o estado armazenado entao nao sobe de camada.
				 */
				
                if ( (RTTatual >= failrtt) || (perdasatual > failperdas) )
                    {	
#ifdef _LOG_NETSTATE
                       ofs_logNet << "Estado atual pior que um de falha!\t NÃ£o vai subir!" << endl << endl;
#endif
                       return -1;
                    }
#ifdef _LOG_NETSTATE
                ofs_logNet << "Estado atual melhor!" << endl << endl;
#endif
             }
          }
#ifdef _LOG_NETSTATE
          ofs_logNet << "Estado atual melhor que todos armazenados! Vai tentar subir!" << endl << endl;
#endif	
       }
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado
 */

	unsigned int bwup;
	unsigned int Bwatual = core_getRatesCumCurrent();
	if (core_getCurrentLayer() == core_getTotalLayers()-1)
		bwup = Bwatual;
	else
		bwup = core_getRatesCum(core_getCurrentLayer()+1);	
	//while (*novabw > bwup) { anterior
	while (*Bweq > bwup) {
		if (bwup >= Bwmax) {
#ifdef _ALMTF_DEBUG2
			cout << "Banda Maxima Atingida!" << endl;
#endif
			return -1;
		}
		if (core_getCurrentLayer() >= core_getTotalLayers()-1) {
#ifdef _ALMTF_DEBUG2
			cout << "Camada Máxima Atingida!" << endl;
#endif
			return -1;
		}
#ifdef _LOG_LOG
		stringstream ss;
		ss << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\t Bweq > bwup, vai SUBIR camada! "<<core_getCurrentLayer()+1<<endl;
		ss << "\t Bweq = "<<*novabw<<endl;
		ss << "\t bwup = "<<bwup<<endl;
		ofs_log << ss.str();
#endif
		core_addLayer();

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef _LOG_LAYERS
		stringstream logSS("");
		char _b[20];
		memset(_b,0,sizeof(_b));
		struct timeval _timenow;
		time_getus(&_timenow);
		sprintf(_b, "%.6d", (int)_timenow.tv_usec);
		logSS << _timenow.tv_sec << "," << _b << "\t ";
		logSS << core_getCurrentLayer() << endl;
		ofs_logLayers << logSS.str();
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado
 */
		Bwatual = core_getRatesCumCurrent();
		if (core_getCurrentLayer() == core_getTotalLayers()-1) { 
			//Está na última camada,não tem mais o que subir.
			bwup = Bwatual;
			return 1;
		}
		else bwup =  core_getRatesCum(core_getCurrentLayer()+1);
		if (*Bweq > bwup) {
			*novabw = (Bwatual + bwup) / 2;
			*Bweq = (Bwatual + bwup) / 2; //inserido para transição: banda somente por equação
#ifdef _LOG_BWJANELA
			ofs_logBwjanela << "if (*novabw > bwup)\t\tBwjanela=" << *novabw << endl;
#endif
#ifdef _LOG_CWND
			ofs_logCwnd << "if (*novabw > bwup)" << endl;
#endif
			double dbRTT;
			struct timeval tpRTT = ALMTF_getRTT();
			dbRTT = time_timeval2double(&tpRTT);
			double CwndMax= (Bwmax * dbRTT*1000)/(ALMTF_PACKETSIZE * 8);
			*Cwnd = ALMTF_rate2win(novabw, Cwnd, CwndMax);
		}
	}
	//*Time_addLayerWait = timenow; //alterado em função de modificações na estimativa de perdas 09/02/09

/*
 * inicio de bloco de código do mecanismo de aprendizado
 */
#ifdef LEARNING	
	// se teve 3 falhas ao subir
	if (join_failures >= 3){
#ifdef _LOG_FAILURES
		ofs_logFailures << "***************************" << endl;
		ofs_logFailures << "*****  Teve 3 Falhas!! ****" << endl;
		ofs_logFailures << "***************************" << endl;
#endif
		join_failures = 0;
		// incrementa o multiplicador do tempo de estabilizacao
		if (t_stab_mult < mult_limit) // mas nao para mais que o limitador
			t_stab_mult = t_stab_mult * mult_inc; // exponencial -> 2,4,8,16...MULT_LIMIT
#ifdef _LOG_FAILURES
		ofs_logFailures << "Multiplicador = " << t_stab_mult << endl;
#endif
	}
	//time_add_ms(Time_addLayerWait, ALMTF_T_STAB*t_stab_mult, Time_addLayerWait); //alterado em função de modificações na estimativa de perdas 09/02/09

	ALMTF_SetTimeEI(ALMTF_T_STAB*t_stab_mult, ADDL);

#else
	//time_add_ms(Time_addLayerWait, ALMTF_T_STAB, Time_addLayerWait); //alterado em função de modificações na estimativa de perdas 09/02/09
   
   //Alterado pois ADDL não será usada nesta versão
	ALMTF_SetTimeEI(ALMTF_T_STAB, ADDL);

	//Time_addLayerWait=timenow + ALMTF_T_STAB=Evita subir duas camadas menos de Time_stab
#endif	
/*
 * fim de bloco de código do mecanismo de aprendizado
 */

	return 1;
}

struct timeval ALMTF_calculaRTT (long lseqno, transm_packet* pkt, struct timeval* timepkt, struct timeval pRTT)
{
	struct timeval rtt_medido;
	struct timeval ctpRTT = pRTT;
	rtt_medido.tv_sec = 0;
	rtt_medido.tv_usec = 0;
	hdr_almtf* almtf_h = &(pkt->header);	
	// se é o primeiro pacote recebido na camada, envia msg para o transmissor e ignora todo cálculo de RTT
	if (lseqno == -1) {		
		ALMTF_sendPkt();
		time_div(&ctpRTT, 2.0, &t_vinda);
		time_subtract(timepkt, &almtf_h->timestamp, &t_assim);
		time_subtract(&t_assim, &t_vinda, &t_assim);
	// se não é o primeiro pacote recebido, inicia o processo de cálculo do RTT, etc.
	} else {			
		// se já passou o intervalo de feedback, envia mensagem para o transmissor
		struct timeval tmpsb;			
		tmpsb.tv_sec = 0;
		tmpsb.tv_usec = 0;
		time_subtract(timepkt, &LastReportSent, &tmpsb);
		if (time_compare_us(&tmpsb, IntFeedback*1000) > 0)  //Intervalo de feedback em segundos
			ALMTF_sendPkt();
		//if (now-LastReportSent > IntFeedback)
		//////// Inicio do calculo do RTT /////////
		// caso FL_MEDERTT ligada, timestamp_offset eh valido e pode medir RTT
		if ((almtf_h->flagsALMTF & FL_MEDERTT) && (almtf_h->numReceptor == ALMTF_getIP(ReceptorIP)))
		{ // && almtf_h->num_receptor == num_nodereceptor_) {
			// calcula o RTT precisamente, conforme dados vindos do transmissor				
			time_subtract(timepkt, &almtf_h->timestampEcho, &rtt_medido);
			time_subtract(&rtt_medido, &almtf_h->timestampOffset, &rtt_medido);				
			//rtt_medido = (double)now-(double)almtf_h->timestamp_echo-(double)almtf_h->timestamp_offset;
			//cout << "RTT: " << time_timeval2double(&rtt_medido) << "\ttime: " <<  time_timeval2double(timepkt) << endl;
			if (time_compare_us(&rtt_medido, 0) < 0) 
			{//if (rtt_medido < 0)	
				rtt_medido.tv_sec = 0;
				rtt_medido.tv_usec = 0;
			}
			// atualiza o tempo de ida e a assimetria dos relógios conforme novo rtt medido
			time_div(&rtt_medido, 2.0, &t_vinda);				
			//t_vinda = rtt_medido / 2;
			time_subtract(timepkt, &almtf_h->timestamp, &t_assim);
			time_subtract(&t_assim, &t_vinda, &t_assim);
			//t_assim = now - almtf_h->timestamp - t_vinda;
			FiltraRTT = true; // todas outras medidas utiliza o filtro para deixar mais estavel
		}
		else
		{
			time_subtract(timepkt, &almtf_h->timestamp, &rtt_medido);
			time_subtract(&rtt_medido, &t_assim, &rtt_medido);
			time_add(&rtt_medido, &t_vinda, &rtt_medido);
			//rtt_medido = t_vinda + ((double)now-(double)almtf_h->timestamp-t_assim);				
			if (time_compare_us(&rtt_medido, 0) < 0)
			{
				rtt_medido.tv_sec = 0;
				rtt_medido.tv_usec = 0;
			}
		}
		if(FiltraRTT)
		{
			if(first_rtt){
				first_rtt = false;
				ctpRTT = rtt_medido;
				//cout << "Primeiro RTT: " << time_timeval2double(&ctpRTT) << "\ttime: " <<  time_timeval2double(timepkt) << endl;
			}
			else{
				// ALTERACAO_NO25RTT
				//if (time_compare(&ctpRTT,&rtt_medido) > 0)
				//{
					struct timeval tmp1;
					time_mul(&rtt_medido, 0.1, &tmp1);
					time_mul(&ctpRTT, 0.9, &ctpRTT);
					time_add(&ctpRTT, &tmp1, &ctpRTT);
					//RTT.tv_sec = 0.9*RTT.tv_sec + 0.1*rtt_medido.tv_sec;
					//RTT.tv_usec = 0.9*RTT.tv_usec + 0.1*rtt_medido.tv_usec;
				/*}
				else
				{						
					struct timeval tmpRTT;
					time_mul(&ctpRTT, 1.25, &tmpRTT);
					if (time_compare(&rtt_medido, &tmpRTT) > 0)						
						ctpRTT = rtt_medido;
				}*/
			}
			//if (rtt_medido < RTT) // se rtt medido menor => banda maior - filtra para nao subir muito rapido
			//	RTT = 0.9 * RTT + 0.1 * rtt_medido; // medicao do pacote vale apenas 10% do rtt
			//else
			//	if (rtt_medido > RTT*1.25) // se rtt medido maior que 25% do anterior atualiza
			//		RTT = rtt_medido;	
		}
		else
		{
			ctpRTT = rtt_medido;
			//time_mul(&t_vinda, 2.0, &ctpRTT);
			//RTT = 2 * t_vinda; // supoe link simetrico na primeira vez // TODO t_vinda = RTT/2 ali em cima .. entõa aqui o RTT deve continuar mesma coisa, nem precisa esse código
		}		
		//////// Fim do calculo de RTT /////////
		if ((almtf_h->flagsALMTF & FL_MEDERTT) == 0)
		{   // se flag desligada, entao campo significa num receptores
			Numrec = almtf_h->numReceptor;
			if (Numrec > 60) // maximo 60 receptores para calculo de intervalo entre feedbacks
				Numrec = 60;
		}
	}
	//ctpRTT.tv_sec = 0;	//TEMP RTT FIXADO!!
	//ctpRTT.tv_usec = 10000;	
	return(ctpRTT);
}

struct timeval ALMTF_getRTT()
{
	struct timeval tpRTT;
	pthread_mutex_lock(&cptRTT_mutex);
#ifdef RTT_TESTES_CONSTANTE
	tpRTT.tv_sec = 0;
	tpRTT.tv_usec = RTT_TESTES_CONSTANTE*1000;
#else
	tpRTT = RTT;
#endif
	pthread_mutex_unlock(&cptRTT_mutex);
	return tpRTT;
}

int ALMTF_getNumloss()
{
	int tpNloss;
	pthread_mutex_lock(&cptNumloss_mutex);
	tpNloss = Numloss;
	pthread_mutex_unlock(&cptNumloss_mutex);
	return tpNloss;	
}

void ALMTF_setNumloss(int tpNloss)
{
	pthread_mutex_lock(&cptNumloss_mutex);
	Numloss = tpNloss;
	// ALTERAÇÂO_PERDAS_VETOR
	//Num_perdas = 0;
	pthread_mutex_unlock(&cptNumloss_mutex);	
}

void ALMTF_recvPkt(ALMTFRecLayer *layer, transm_packet* pkt, struct timeval *timepkt)
{	// ALTERAÇÂO_PERDAS_VETOR
	bool instant_loss;
	hdr_almtf* almtf_h = &(pkt->header);
	if (layer->layerID == 0) 
	{		
#ifdef SINCRONISMO_JOIN
		if (almtf_h->sincjoin > (unsigned)Sincjoin_recv){
			Sincjoin_recv = almtf_h->sincjoin;
			Sincjoin_recvCp = almtf_h->sincjoin;
		}
#else
		Sincjoin_recv = 100;
		Sincjoin_recvCp = 100;
#endif
		
		pthread_mutex_lock(&cptRTT_mutex);
		RTT = ALMTF_calculaRTT(layer->seqno, pkt, timepkt, RTT);
		pthread_mutex_unlock(&cptRTT_mutex);
	}
	// verifica perda de pacotes
	layer->seqno = almtf_h->seqno;

	// ALTERAÇÂO_PERDAS_VETOR
	instant_loss = false;

    if (layer->seqno_expected >= 0) {
        int loss = layer->seqno - layer->seqno_expected; // loss representa o num de pacotes perdidos em rajada
        if (loss > 0) {

			// ALTERAÇÂO_PERDAS_VETOR
         instant_loss = true;
      
			layer->nlost += loss; // para estatisticas via TCL (almtf-ns.tcl)			
			ALMTF_setNumloss(ALMTF_getNumloss()+loss);
#ifdef _LOG_NUMLOSS
			char b[20];
			memset(b, 0, sizeof(b));
			sprintf(b, "%.6d", (int)time_getnow_usec());
			ofs_logNumloss << time_getnow_sec() <<","<<b<<"\tLayer "<<layer->layerID<<"\tLoss = "<<loss<<" -> Numloss = "<<ALMTF_getNumloss()<<endl;
#endif		
        }
    }
    layer->seqno_expected = layer->seqno + 1;
	ALMTF_estimaperdas(timepkt, instant_loss);
    	
	if (layer->layerID == 0) { 
		// Trata tráfego em pares de pacotes.
		// O transmissor PP envia o primeiro com a flag_PP ligada.	
		if (almtf_h->flagsALMTF & FL_INIRAJADA) {
			layer->measure = 1; //No primeiro da sequência do burst, seta flag.
			layer->seqno_next = layer->seqno+1;		
			layer->first_packet = *timepkt;
		} else { // FL_INIRAJADA NÃO ligada!
			if (layer->measure > -1) {			
				if (layer->seqno == layer->seqno_next) {
					layer->measure++;
					layer->seqno_next = layer->seqno+1;					
					//Se o número de pacotes do burst chegou, efetua o cálculo de banda máxima desta medida.
					if (layer->measure == ALMTF_PP_BURST_LENGTH) {
						struct timeval tv_diff;
						time_subtract(timepkt, &layer->first_packet, &tv_diff);
						double PP_value = 0.0;						
						double timevar;	
						timevar = time_timeval2double(&tv_diff)*1000.0; // *1000 para ficar em ms
						if (timevar > 0) {
							// PP_value em Kbit/s e timevar em ms							
							PP_value = (ALMTF_PACKETSIZE+sizeof(pkt->header))*8.0*(ALMTF_PP_BURST_LENGTH-1)/timevar;					
							ALMTFBwmax PP_Bwmax;
							PP_Bwmax.PP_value = PP_value;
							PP_Bwmax.layerID = layer->layerID;
							BwmaxList.push_back(PP_Bwmax);
						}
						layer->measure = -1;
					}
				} else { //Pacote fora da sequência é ignorado.
					layer->measure = -1;
				}
			}
		}
	}
}

void ALMTF_sendPkt()
{
	struct timeval now;
	time_getus(&now);
	LastReportSent = now;
	if (Numrec == 0) Numrec = 1; // se ainda não atualizou Numrec, será 0
	double r = utils_rand()/RAND_MAX;
	IntFeedback = (long)((r*Numrec*1000) + 1);
	transm_packet *pkt = (transm_packet *)malloc(sizeof(transm_packet));
	hdr_almtf *almtf_h = &(pkt->header);
	almtf_h->seqno = 0;
	almtf_h->sincjoin = 0;
	almtf_h->timestampEcho.tv_sec = 0;
	almtf_h->timestampEcho.tv_usec = 0;
	almtf_h->timestampOffset.tv_sec = 0;
	almtf_h->timestampOffset.tv_usec = 0;
	almtf_h->flagsALMTF = 0;
	almtf_h->timestamp = now;
	unsigned int packIP = ALMTF_getIP(ReceptorIP);
	almtf_h->numReceptor = packIP;
	core_sendPacket(pkt);
	free(pkt);
}

#ifdef GRAPH_JAVAINTERFACE
void GRAPH_getInfo(GRAPHInfo* i)
{	
	i->Bwatual = core_getRatesCumCurrent();
	i->Bweq = Info.Bweq;
	i->Bwjanela = Info.Bwjanela;
	i->Bwmax = Bwmax;
	struct timeval tmpRTT = ALMTF_getRTT();
	double dbRTT = time_timeval2double(&tmpRTT);
	i->RTT = dbRTT*1000000;
	i->Numloss = ALMTF_getNumloss();
}
#endif

bool ALMTF_setIP(char* IP)
{
	struct ifaddrs *list_addr = NULL;
	struct ifaddrs *list_buf = NULL;  
	int count = 0;
	if (getifaddrs (&list_buf) < 0) {
		 freeifaddrs (list_buf);
         return false;
    }    
    for (list_addr = list_buf; list_addr; list_addr = list_addr->ifa_next) {
          socklen_t s_len;
          char ip_addr[16];
          if (!list_addr->ifa_addr)
        	  continue;
          if (list_addr->ifa_addr->sa_family == AF_INET) {
               s_len = sizeof (struct sockaddr_in);
               count++;
          } else if (list_addr->ifa_addr->sa_family == AF_INET6)
        	  		   	  s_len = sizeof (struct sockaddr_in6);
          		  	else
          		  		  continue;
          if (getnameinfo (list_addr->ifa_addr, s_len, ip_addr, sizeof (ip_addr), NULL, 0, NI_NUMERICHOST) < 0) {
              continue;
          }
		  if ((count == 2) && (list_addr->ifa_addr->sa_family == AF_INET)) {
			   strcpy(IP,ip_addr);
			   freeifaddrs (list_buf);
			   return true;
		  }
      }
      freeifaddrs (list_buf);
      return false;
}

unsigned int ALMTF_getIP (char* IP)
{
	unsigned int a,b,c,d;
	sscanf(IP, "%d.%d.%d.%d", &a, &b, &c, &d ); 	
	return( (a*16777216)+(b*65536)+(c*256)+(d) );
}


/*
 * funções inseridas pelo mecanismo de aprendizado
*/
#ifdef NETSTATE
void init_netvars(NETSTATEvars *net)
{
   net->rtt = 0;
   net->perdas = 0;
   net->expireTime.tv_sec = 0;
   net->expireTime.tv_usec = 0;
}

void save_netvars(NETSTATEvars *net)
{
   struct timeval tmpRTT = ALMTF_getRTT();
   double dbRTT = time_timeval2double(&tmpRTT);	
   net->rtt = dbRTT;	
   net->perdas = ALMTF_getPerdas();	
}
#endif
#ifdef READ_CONFIG
void learning_init(){	
	ifstream file(FILENAME_LEARNING);
	if (!file.is_open()){
		cout << "\nError opening configuration file"<<endl;
	    exit(0);
	} else cout << endl << "Succeeded opening configuration file"<<endl;
	
	if (!file.eof()){
		file >> fail_time;
		file >> mult_limit;
		file >> mult_inc;
		file >> storage_time;
		file >> similarity;
	}
	file.close();
		
	cout << "Configuracoes: " << endl;
	cout << "\tfail_time = " << fail_time << endl;
	cout << "\tmult_limit = " << mult_limit << endl;
	cout << "\tmult_inc = " << mult_inc << endl;
	cout << "\tstorage_time = " << storage_time << endl;
	cout << "\tsimilarity = " << similarity << endl << endl;	
}
#endif

int main(int argc, char *argv[])
{
	utils_init();

/*
 * inicio de bloco de código do mecanismo de aprendizado
*/
#ifdef READ_CONFIG
	learning_init();
#endif
/*
 * fim de bloco de código do mecanismo de aprendizado
*/

	if (argc > 1) {
		core_setLogTime(atoi(argv[1]));
		if (argc > 2)
			core_setLogStep(atoi(argv[2]));
	}
	cout << "Vai fazer log por " << core_getLogTime();
	cout << " ms. Step de " << core_getLogStep() << " ms" << endl << endl;	
	core_init();
	ALMTF_init();
	while (!core_allLayersFinished()) 
		   usleep(2000);	
	cout << "Receptor: Conexoes encerradas, fim do programa..."<<endl<<endl;
	return 1;
}
