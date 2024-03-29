/*
 *   Universidade Federal do Rio Grande do Sul - UFRGS
 *   PRAV - Projetos de Audio e Vídeo 
 *   Projeto SAM - Sistema Adaptativo Multimedia
 *
 *	 Copyright(C) 2008 - Andrea Collin Krob <ackrob@inf.ufrgs.br>
 *                       Alberto Junior     <e-mail>
 *                       Athos Fontanari    <aalfontanari@inf.ufrgs.br>
 *                       João Victor Portal <e-mail>
 * 						 Jonas Hartmann		<jonashartmann@inf.ufrgs.br>
 *                       Leonardo Daronco   <e-mail>
 *                       Valter Roesler     <e-mail>
 *
 *
 *     Sender.cpp: Implementação do algoritmo ALMTF versão Windows/Linux.
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
#ifdef _LOG_TIMEOUT
	ofstream ofs_logTimeout;
#endif
#ifdef _LOG_NETSTATE
	ofstream ofs_logNet;
#endif
#ifdef _LOG_LAYERS
	ofstream ofs_logLayers;
#endif
	
//Variável RTT utiizada em várias funções, mas alterada apenas na CalculaRTT
struct timeval RTT;		// RTT em milisegundos
int Numloss;			// número de perdas
int Sincjoin_recv;		// variável para sincronismo de join dos receptores

//Cálculo do RTT: Variáveis utilizadas apenas no cálculo do RTT: Apenas 1 thread tem acesso
bool FiltraRTT;
unsigned int Numrec;
struct timeval LastReportSent;
long IntFeedback;
struct timeval t_vinda;
struct timeval t_assim;

#ifdef NETSTATE
int join_failures = 0;
struct timeval t_failure;

int posAtual;			// indica a posição a ser inserido o elemento na lista de estados.
typedef vector< vector<NETSTATEvars> > NETSTATElist;

/* Lista para guardar os variados estados da rede.
 * Quando um receptor sobe para a camada i,
 * ele guarda o estado da rede na posição i da lista.
 */
NETSTATElist netList;

#endif

//Cálculo da Banda Máxima: alterado apenas na thread que realiza o cálculo
double Bwmax;				 // Banda máxima pelos pares de pacotes
deque<ALMTFBwmax> BwmaxList; // Lista que armazena os valores de pares de pacotes

//IP do Receptor capturado no início da execução
char* ReceptorIP;

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
	unsigned int bwatual = core_getRatesCumActual();
	ss << "\t Bwatual = "<<bwatual<<endl;
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);
	ss << "\t RTT = " << dbRTT <<endl;
	ss << "\t Numrec = " << Numrec <<endl;
	ss << "\t IntFeedback = " << IntFeedback <<endl;
	ss << "\t Numloss = " << ALMTF_getNumloss() <<endl;
	ss << "\t Sincjoin = " << Sincjoin_recv << endl;
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
			unsigned int Bwatual = core_getRatesCumActual();
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

void ALMTF_init()
{
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
	ALMTFTimeEI TimeEI;
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
    ALM_state = ALM_STATE_START; 
	if (ALMTF_PP == true) { // Com pares de pacotes
		Bwmax = 0.0;
    } else {		
		Bwmax = 10000000; // coloca valor alto para nao mexer muito no codigo ??
    }
	ReceptorIP = new char[16];
	bzero(ReceptorIP, sizeof(ReceptorIP));
	ALMTF_setIP(ReceptorIP);
	cerr << "Receptor IP : " << ReceptorIP << endl;
	TimeEI.NoColateral.tv_sec = 0;
	TimeEI.NoColateral.tv_usec = 0;
	TimeEI.AddLayerWait.tv_sec = 0;
	TimeEI.AddLayerWait.tv_usec = 0;
	ALMTF_setNumloss(0);
	LastReportSent.tv_sec = 0;
	LastReportSent.tv_usec = 0;
	IntFeedback = 0;
	Sincjoin_recv = 0;
	RTT.tv_sec = 0;
	RTT.tv_usec = ALMTF_RTT*1000;
    Numrec = 0;
    FiltraRTT = false; // avisa que eh a primeirissima medida de RTT e deve atribuir RTT e nao filtrar
    // Inicializa ALMTF_eqn, responsavel por determinar bunsigned int Bwatual = core_getRatesCumActual();anda via equacao do TCP
    ALMTFLossEvent Ep;	
	ALMTF_inicializa_eqn(&Ep);
    Bweq = 1000.0;	// valor inicial para funcionar o start-state
	core_addLayer(); // se registra na primeira camada
	if (pthread_create(&IDThread_Bwmax, NULL, ALMTF_calcbwmax, (void *)NULL) != 0) {
		cerr << "Error Creating Thread!" << endl;	
	}	
	struct timeval time_lastPkt;
	core_getLastPktTime(&time_lastPkt);
	TimeEI.AddLayerWait = time_lastPkt;
	time_add_ms(&TimeEI.AddLayerWait, ALMTF_T_STAB, &TimeEI.AddLayerWait);
	struct timeval timenow;
	time_getus(&timenow);
	TimeEI.Stab.tv_sec = 0;
	TimeEI.Stab.tv_usec = 0;
	TimeEI.Stab = timenow;
	struct timeval tpRTT = ALMTF_getRTT();
	time_add(&TimeEI.Stab, &tpRTT, &TimeEI.Stab);
	//Time_stab = espera um RTT com cwnd em "1", tal qual o TCP.
	
#ifdef NETSTATE
	// Ajusta o tamanho da matriz e inicializa os elementos.
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
	log_ss << "===========================================================================================================================================" << endl;	
	log_ss << "============================================================  Valores Iniciais ============================================================" << endl;
	log_ss << "Tempo\t" << log_time_now.tv_sec << "," << b << "\t";
	unsigned int Bwatual = core_getRatesCumActual();
	log_ss << "Bwatual\t" << Bwatual << "\t";
	memset(b, 0,sizeof(b));
	sprintf(b, "%.6d", (int)RTT.tv_usec);
	log_ss << "RTT\t" << RTT.tv_sec << "," << b << "\t";
	log_ss << "Bweq\t" << Bweq << "\t";
	log_ss << "Fase\t" << ALM_state << "\t";
	log_ss << "Sincjoin\t" << Sincjoin_recv << "\t";	
	log_ss << "Bwmax\t" << Bwmax << "\t";	
	log_ss << "Cwnd\t" << Cwnd << "\t";
	log_ss << "Bwjanela\t" << Bwjanela << endl;
	log_ss << "===========================================================================================================================================" << endl;
	ofs_logEI << log_ss.str();
	ofs_logEI.flush();
#endif
	// inicia a execução do algoritmo
	struct timeval Twait;
#ifdef _LOG_LAYERS
	stringstream logSS("");
	char _b[20];
	memset(_b,0,sizeof(_b));
	struct timeval _timenow;
	time_getus(&_timenow);
	sprintf(_b, "%.6d", (int)_timenow.tv_usec);
	logSS << _timenow.tv_sec << "," << _b << "\t ";
	logSS << core_getActualLayer() << endl;
	ofs_logLayers << "# Arquivo com dados para o Gnuplot" << endl;
	ofs_logLayers << "# Tempo\t\tCamada" << endl;
	ofs_logLayers << logSS.str();
#endif
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
		ALMTF_EI(&TimeEI,&Cwnd,&Bwjanela,&Bweq,&Ep,&ALM_state);
#ifdef GRAPH_JAVAINTERFACE
		Info.Bweq = Bweq;
		Info.Bwjanela = Bwjanela;
#endif	
#ifdef _LOG_EI
		ofs_logEI << "Cwnd\t" << Cwnd << "\t";
		ofs_logEI << "Bwjanela\t" << Bwjanela << endl;
		ofs_logEI.flush();
#endif
#ifdef _LOG_LAYERS
		ofs_logLayers.flush();
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
		time_mul(&tpRTT, Cwnd, &Twait);
		time_mul(&Twait, ALMTF_EIMULTIPLIER, &Twait);
#ifdef _LOG_LOG
		log_printAll(&ALM_state, &Bwjanela, &Bweq, &Cwnd);
		ofs_log << "Now: " << time_getnow_sec() <<","<<time_getnow_usec()<<"us\t Vai dormir RTT*Cwnd*N = "<<(Twait.tv_sec*1000000+Twait.tv_usec)<<"us"<<endl;
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
#ifdef _LOG_TIMEOUT
	ofs_logTimeout.close();
#endif
#ifdef _LOG_NETSTATE
	ofs_logNet.close();
#endif
#ifdef _LOG_LAYERS
	ofs_logLayers.close();
#endif
}

void ALMTF_EI(ALMTFTimeEI *TimeEI, double *Cwnd, double *Bwjanela, double *Bweq, ALMTFLossEvent *Ep, ALMstate *ALM_state)
{
#ifdef _LOG_LAYERS
	stringstream logSS("");
	char _b[20];
	memset(_b,0,sizeof(_b));
	struct timeval _timenow;
	time_getus(&_timenow);
	sprintf(_b, "%.6d", (int)_timenow.tv_usec);
	logSS << _timenow.tv_sec << "," << _b << "\t ";
	logSS << core_getActualLayer() << endl;
	ofs_logLayers << logSS.str();
#endif
	// tempo de início do algoritmo
	struct timeval tstart;
	time_getus(&tstart);
	char b[20];
	unsigned int Bwatual = core_getRatesCumActual();
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
	struct timeval tpRTT; 
	tpRTT = ALMTF_getRTT();	
#ifdef _LOG_EI
	memset(b, 0,sizeof(b));	
	sprintf(b, "%.6d", (int)tpRTT.tv_usec);
	logEI_ss << "RTT\t" << tpRTT.tv_sec << "," << b << "\t";
#endif
	*Bweq = ALMTF_estimabanda(TimePkt,Bweq,Ep,tpRTT);
#ifdef _LOG_EI
	logEI_ss << "Bweq\t" << *Bweq << "\t";
	ofs_logEI << logEI_ss.str();
#endif
	// Efetua estimativa de banda atraves da equacao do TCP, armazenando na variavel banda_eqn
	double cwndant;
	if (time_compare(&TimeEI->Stab, &tstart) > 0) {
#ifdef _LOG_CWND
		ofs_logCwnd << "if (time_compare(&TimeEI->Stab, &tstart) > 0)" << endl;
#endif
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd);
		ALMTF_setNumloss(0);
		Sincjoin_recv=0;
#ifdef _LOG_LOG
		ofs_log << time_getnow_sec() <<","<<time_getnow_usec()<<"\t Time_stab > tstart, vai esperar..."<<endl;
		ofs_log << "\t Time_stab = "<<TimeEI->Stab.tv_sec<<","<<TimeEI->Stab.tv_usec<<endl;
		ofs_log << "\t Tstart = "<<tstart.tv_sec<<","<<tstart.tv_usec<<endl;
#endif
		return;
	}	
	// se está estabilizando pois outro receptor se cadastrou em uma camada,
	// ignora as perdas
	if (time_compare(&TimeEI->NoColateral, &tstart) > 0) {
		ALMTF_setNumloss(0);
	}

	if (Bwmax == 0) return;

	unsigned int bwup;
	if (core_getActualLayer() == core_getTotalLayers()-1)
		bwup = Bwatual;
	else {
		bwup = core_getRatesCum(core_getActualLayer()+1);
	}

	cwndant = *Cwnd;
#ifdef _LOG_CWND
		ofs_logCwnd << "--> cwndant=" << cwndant << endl;
#endif
	if (ALMTF_timeout()) {
#ifdef _LOG_CWND
		ofs_logCwnd << "if (ALMTF_timeout())\t\t Cwnd=" << *Cwnd << endl;
#endif
		*Cwnd = 1.0;
		ALMTF_reduzbanda(Cwnd,Bwjanela);
		TimeEI->Stab = tstart;
		time_add_ms(&TimeEI->Stab, ALMTF_T_STAB, &TimeEI->Stab);
	}

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
				if (*Cwnd > ALMTF_WINDOWSIZE) {
					*Cwnd = ALMTF_WINDOWSIZE;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = ALMTF_WINDOWSIZE\t\tCwnd=" << *Cwnd << endl;
#endif					
				}	
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "case ALM_STATE_START\tif (Numloss == 0)" << endl;
#endif	
				*Bwjanela = ALMTF_win2rate(Cwnd); // lixo
				if (*Bwjanela > Bwmax) {
					*Bwjanela = Bwmax;
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "\tBwjanela = Bwmax\t\tBwjanela=" << *Bwjanela << endl;
#endif				
				}
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
				if (*Cwnd > ALMTF_WINDOWSIZE) {
					*Cwnd = ALMTF_WINDOWSIZE;
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = ALMTF_WINDOWSIZE\t\tCwnd=" << *Cwnd << endl;
#endif					
				}
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "case ALM_STATE_STEADY\tif (Numloss == 0)" << endl;
#endif	
				*Bwjanela = ALMTF_win2rate(Cwnd);				
				if (*Bwjanela > Bwmax) {
					*Bwjanela = Bwmax; // ?? precisa será
#ifdef _LOG_BWJANELA
					ofs_logBwjanela << "\tBwjanela = Bwmax\t\tBwjanela=" << *Bwjanela << endl;
#endif	
				}
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
		*Bwjanela = 2*(*Bweq);
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "if (Bwjanela > 2*Bweq)\t\tBwjanela=" << *Bwjanela << endl;
#endif	
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd);
	} else if (*Bwjanela < (*Bweq)/2) {
#ifdef _LOG_LOG
				ofs_log << "\t Bwjanela < Bweq/2, limitando conforme Bweq"<<endl;
#endif
				*Bwjanela = (*Bweq)/2;
#ifdef _LOG_BWJANELA
				ofs_logBwjanela << "if (Bwjanela < Bweq/2)\t\tBwjanela=" << *Bwjanela << endl;
#endif				
				*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd);
			}
#ifdef _LOG_LOG
	log_printAll(ALM_state, Bwjanela, Bweq, Cwnd);	
#endif
#ifdef _LOG_EI
	ofs_logEI << "Sincjoin\t" << Sincjoin_recv << "\t";
	ofs_logEI << "Bwmax\t" << Bwmax << "\t";
#endif	
#ifdef _LOG_LOG
			ofs_log << "\t Bwjanela = "<<*Bwjanela<<"\t Bwup = "<<bwup<<endl;;
#endif
	// TRATA SUBIR CAMADA
	if (*Bwjanela > bwup) {		
		if (Sincjoin_recv > core_getActualLayer() || *ALM_state == ALM_STATE_START) {
#ifdef _LOG_LOG
			ofs_log << "if (Sincjoin_recv > core_getActualLayer() || *ALM_state == ALM_STATE_START)" << endl;
			ofs_log << "Sincjoin = " << Sincjoin_recv << "\tCamada Atual = " << core_getActualLayer() << endl;
#endif
#ifdef NETSTATE
			NETSTATEvars netVars;
			save_netvars(&netVars,*Bwjanela,Bwmax,*Cwnd);
			/*
			netVars.Bwjanela = *Bwjanela;
			netVars.Bwmax = Bwmax;
			netVars.Cwnd = *Cwnd;
			*/
#endif
			int r = ALMTF_addLayer(Bwjanela, &TimeEI->AddLayerWait, Cwnd);
			if (r == 1) {		
#ifdef _LOG_LOG
				ofs_log << "Camada Adicionada = " << core_getActualLayer() << endl;
#endif
				Sincjoin_recv = 0;
#ifdef NETSTATE
				time_getus(&t_failure);
				netVars.addTime = t_failure;	//Guarda o tempo em que subiu de camada.
				netVars.falhou = false;			//Seta como false, pois pode não falhar.
				netList[core_getActualLayer()][posAtual] = netVars;
				posAtual++;
				if (posAtual == NUM_ELEM)
					posAtual = 0;
#ifdef _LOG_NETSTATE
				ofs_logNet << "Subiu de camada! " << core_getActualLayer() << endl;
				ofs_logNet << "Guardou o estado da rede!" << endl;
				char auxb[20];
				memset(auxb, 0,sizeof(auxb));				
				sprintf(auxb, "%.6d", (int)t_failure.tv_usec);	
				ofs_logNet << "Tempo atual =              \t" << t_failure.tv_sec << "," << auxb << endl;
#endif
				time_add_ms(&t_failure, FAIL_TIME, &t_failure);
				// Tempo atual + x segundos
				// Se descer de camada x segundos apos a subida, considera como uma falha
#ifdef _LOG_NETSTATE				
				memset(auxb, 0,sizeof(auxb));				
				sprintf(auxb, "%.6d", (int)t_failure.tv_usec);
				ofs_logNet << "Tempo atual + 2 segundos = \t" << t_failure.tv_sec << "," << auxb << endl;
				ofs_logNet << endl;
#endif
#endif
			} else 
				if (r == -1) {
					*Cwnd = cwndant;
#ifdef _LOG_LOG
					ofs_log << "Camada NAO Adicionada = " << "Cwnd = " << *Cwnd << endl;
#endif
#ifdef _LOG_CWND
					ofs_logCwnd << "Cwnd = cwndant\t\tCwnd=" << *Cwnd << endl;
#endif	
				}
		}
	}
#ifdef SINCRONISMO_JOIN
	else {
		if (Sincjoin_recv > core_getActualLayer()+1 && *ALM_state == ALM_STATE_STEADY) {
#ifdef _LOG_LOG
			ofs_log << "if (Sincjoin_recv > core_getActualLayer()+1 && *ALM_state == ALM_STATE_STEADY)" << endl;
			ofs_log << "Sincjoin = " << Sincjoin_recv << "\tCamada Atual = " << core_getActualLayer() << endl;
#endif
			TimeEI->NoColateral = tstart;
			time_add_ms(&TimeEI->NoColateral, 500, &TimeEI->NoColateral);
#ifdef _LOG_LOG
			ofs_log << "TimeStart = " << tstart.tv_sec << "," << tstart.tv_usec << "\tTimeEI->NoColateral = " << TimeEI->NoColateral.tv_sec << "," <<  TimeEI->NoColateral.tv_usec << endl; 
#endif
		} 
	}
	//Faz um único join por vez
	Sincjoin_recv = 0;
#endif
	// TRATA DESCER CAMADA
	if (core_getActualLayer() <= 0) return;

	while (*Bwjanela < Bwatual) {
#ifdef _LOG_LOG
		ofs_log << "while (Bwjanela < Bwatual)" << "Bwjanela = " << *Bwjanela << " Bwatual = " << Bwatual << " Bweq = " << *Bweq << endl;
		ofs_log << "if (Bweq >= Bwatual) RETURN";
#endif
		if (*Bweq >= Bwatual) return;		
#ifdef _LOG_LOG
		ofs_log << "\t Vai DEIXAR camada! "<<core_getActualLayer()<<endl;
#endif
		core_leaveLayer();
		
#ifdef _LOG_LAYERS
		stringstream logSS("");
		char _b[20];
		memset(_b,0,sizeof(_b));
		struct timeval _timenow;
		time_getus(&_timenow);
		sprintf(_b, "%.6d", (int)_timenow.tv_usec);
		logSS << _timenow.tv_sec << "," << _b << "\t ";
		logSS << core_getActualLayer() << endl;
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
				netList[core_getActualLayer()+1][posAtual-1].falhou = true;
			else netList[core_getActualLayer()+1][NUM_ELEM-1].falhou = true;
			// NUM_ELEM-1 é igual ao último elemento.
#ifdef _LOG_NETSTATE
			ofs_logNet << "Salvou o estado atual da rede!\t" << endl << endl;			
#endif
		}
#endif
#ifdef _LOG_LOG
		ofs_log << "\t DEIXOU A CAMADA! "<<core_getActualLayer()+1<<endl;
#endif
		TimeEI->Stab = tstart;
		time_add_ms(&TimeEI->Stab, ALMTF_T_STAB, &TimeEI->Stab);
#ifdef _LOG_LOG
		ofs_log << "TimeStart = " << tstart.tv_sec << "," << tstart.tv_usec << "\tTimeEI->Stab = " << TimeEI->Stab.tv_sec << "," <<  TimeEI->Stab.tv_usec << endl; 
#endif
		//Time_stab = tstart + ALMTF_T_STAB;
		if (core_getActualLayer() <= 0) return;
		Bwatual = core_getRatesCumActual();
		if (core_getActualLayer() == core_getTotalLayers()-1)
			bwup = Bwatual;
		else
			bwup = core_getRatesCum(core_getActualLayer()+1);
		*Bwjanela = (Bwatual+bwup)/2;
		*Cwnd = ALMTF_rate2win(Bwjanela, Cwnd);
#ifdef _LOG_LOG
		ofs_log << "\t Novo Valor Bwjanela = (Bwatual+bwup)/2;"<< *Bwjanela <<endl;
		ofs_log << "\t Novo Valor Cwnd = "<< *Cwnd <<endl;
#endif
#ifdef _LOG_BWJANELA
		ofs_logBwjanela << "Bwjanela = (Bwatual+bwup)/2\t\tBwjanela=" << *Bwjanela << endl;
#endif	
	}
}

void ALMTF_inicializa_eqn(ALMTFLossEvent *Ep)
{
	Ep->Ep = 1;
	Ep->Ep_pesos[0] = 1.0;
	Ep->Ep_pesos[1] = 1.0;
	Ep->Ep_pesos[2] = 1.0;
	Ep->Ep_pesos[3] = 1.0;
	Ep->Ep_pesos[4] = 0.8;
	Ep->Ep_pesos[5] = 0.6;
	Ep->Ep_pesos[6] = 0.4;
	Ep->Ep_pesos[7] = 0.2;
	for (int i = 0; i < 8; i++)
		Ep->Ep_pkt_rec[i] = 0;
	Ep->Ep_tstamp_ultperda.tv_sec = 0;
	Ep->Ep_tstamp_ultperda.tv_usec = 0;
}

/** Transforma a taxa de kbits/s para o valor da janela. 
 *  Funcao que realiza o calculo da inversa da funcao win2rate. Transforma a banda em seu
 *  equivalente em janela de congestionamento, ocorre quando eh necessario adaptar Bwjanela
 *  de acordo com o valor obtido via equação TCP.
 *  Var Globais: Bwjanela, RTT;
 *  Const: PACKETSIZE, WINDOWSIZE;
 */
double ALMTF_rate2win(double *Bwjanela, double *Cwnd) 
{
	double wnd = 0.0;
	double dbRTT;
	struct timeval tpRTT = ALMTF_getRTT();
	dbRTT = time_timeval2double(&tpRTT);
	wnd = ((*Bwjanela) * dbRTT*1000)/(ALMTF_PACKETSIZE * 8);	
	//Proteção para Cwnd não variar mais do que 30%
	if ((wnd/(*Cwnd)) > 0.7*(*Cwnd) && (wnd/(*Cwnd)) < 1.3*(*Cwnd) && wnd < ALMTF_WINDOWSIZE && wnd > 1.0) 
		wnd = *Cwnd;
	if (wnd > ALMTF_WINDOWSIZE) {
		wnd = ALMTF_WINDOWSIZE;
	}		
	if (wnd < 1.0)
		wnd = 1.0;
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
	/* Proteção para Cwnd não ultrapassar o valor máximo. */
	if (*Cwnd > ALMTF_WINDOWSIZE)
		*Cwnd = ALMTF_WINDOWSIZE;
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
	core_getLastPktTimeLayerZero(&time_lastPkt);
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
double ALMTF_perdas2banda(double p, struct timeval actRTT) 
{
	double dbRTT = 0;
	dbRTT = time_timeval2double(&actRTT)*1000; //RTT em ms
	if (p < 0 || dbRTT < 0)
		return (-1);
	double p2b = 0;
	double tmp1 = 0;
	double tmp2 = 0;
	tmp1 = dbRTT * sqrt(2. * p/3.);
	tmp2 = 1.3*sqrt(3. * p/8.);
	if (tmp2 > 1.0)
		tmp2 = 1.0;
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
double ALMTF_media_ult_intervalos(int ini, int fim, unsigned long *vet, float *vetp)
{
	double soma = 0.0;
	double media = 0.0;
	int i;
	for (i=ini; i < fim; i++)
		soma = soma + vetp[i];
	for (i=ini; i < fim; i++)
		media = media + ((vet[i] * vetp[i])/soma);
	return (media);
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
	if (*Cwnd < 2)
		*Cwnd = 2;
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
double ALMTF_estimabanda(struct timeval timestamp, double *Bweq, ALMTFLossEvent *Ep, struct timeval actRTT) 
{
	double banda = 0.0;
	double perdas = 0.0;
	double dbRTT = time_timeval2double(&actRTT);
	perdas = ALMTF_estimaperdas(timestamp,Ep,actRTT);
	if (perdas > 0) {
		banda = ALMTF_perdas2banda(perdas,actRTT);
	} else {
		if (dbRTT == 0.0)
			banda = (*Bweq) + (ALMTF_PACKETSIZE*0.8/(0.000001*1000));
		else
			banda = (*Bweq) + (ALMTF_PACKETSIZE*0.8/(dbRTT*1000)); // BLAH TODO se RTT == 0 ????
	}
	if (banda > Bwmax && Bwmax > 0)
		banda = Bwmax;
	return (banda);
}

/** Estima perdas para utilizacao na equacao de banda. A funcao estima duas taxas de perdas,
 *  uma considerando os pacotes do intervalo atual, e outra não.
 *  OBS: esta funcao eh chamada cada pacote recebido.
 *  eventos_perdas = Ep;
 *  tstamp_ultperda = Ep_tstamp_ultperda;
 *  pkt_rec = Ep_pkt_rec;
 */
double ALMTF_estimaperdas(struct timeval timestamp, ALMTFLossEvent *Ep, struct timeval actRTT)
{
	int i;
	unsigned long pkt_tmp;
	int num_ev;
	double estim1 = 0;
	double estim2 = 0;	
    if (ALMTF_getNumloss() > 0) {		
		struct timeval tmpsc;
		tmpsc.tv_sec = 0;
		tmpsc.tv_usec = 0;
		time_subtract(&timestamp,&Ep->Ep_tstamp_ultperda,&tmpsc);
		if (time_compare(&tmpsc,&actRTT) > 0) {		
			Ep->Ep_tstamp_ultperda = timestamp;
			Ep->Ep++;
			for (i=7; i>0; i--) {
				pkt_tmp = Ep->Ep_pkt_rec[i-1];
				Ep->Ep_pkt_rec[i] = pkt_tmp;
			}
			Ep->Ep_pkt_rec[0]=0;
		}
	}
	Ep->Ep_pkt_rec[0]++;
#ifdef _LOG_EP
	stringstream ss("");
	struct timeval now;
	time_getus(&now);
	ss << now.tv_sec <<","<<now.tv_usec<<"us:\t";
	for (int i = 0; i < 8; i++) {
		ss << Ep->Ep_pkt_rec[i] << " -\t";
	}
		ss <<endl;
	ofs_logEp << ss.str();
#endif
	if (Ep->Ep == 1) {
		return (0.0); /* Não teve perdas. */
	}
	/* 
	 * Estima dois valores: considerando ou nao evento atual (que ainda nao teve perdas). Retorna o maior,
	 * pois o evento atual deve ser utilizado somente para melhorar a estimativa, jah que nao se sabe
	 * quando vai dar perdas
	 */
	if (Ep->Ep > 8) 
		num_ev = 8;
	else
		num_ev = Ep->Ep;
	estim1 = ALMTF_media_ult_intervalos(0,num_ev,(unsigned long *)Ep->Ep_pkt_rec,(float *)Ep->Ep_pesos); /* Considera intervalo atual. */
	estim2 = ALMTF_media_ult_intervalos(1,num_ev,(unsigned long *)Ep->Ep_pkt_rec,(float *)Ep->Ep_pesos); /* Desconsidera o intervalo atual. */
	if (estim2 > estim1)
		estim1 = estim2;
	return (1/estim1);

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
						}
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
					ss << "\t PP_Bufffer = ";
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
}

/*
 *	ALMTF_add_layer.
 *  Adiciona camada se banda permitir, controla banda e o tempo que pode ou não pode subir. 
 *  novabw = Bwjanela	
 */
int ALMTF_addLayer (double *novabw, struct timeval *Time_addLayerWait, double *Cwnd) 
{	
	struct timeval timenow;
	time_getus(&timenow);
	if (time_compare(Time_addLayerWait,&timenow) > 0) //Aguarda antes de subir novamente.
		return -1;									   //timenow < Time_addLayerWait

#ifdef NETSTATE
	if ((core_getActualLayer() < core_getTotalLayers()-1) && (join_failures > NUM_ELEM)){		
		for (int k = 0 ; k < NUM_ELEM; k++){
			if (netList[core_getActualLayer()+1][k].falhou == true){
				//Só compara os estados se o estado guardado é de uma falha.
				double failbw = netList[core_getActualLayer()+1][k].Bwjanela;
				double failbwmax = netList[core_getActualLayer()+1][k].Bwmax;
				double failcwnd = netList[core_getActualLayer()+1][k].Cwnd;
#ifdef _LOG_NETSTATE
				ofs_logNet << "***** Estado De Falha *****" << endl;
				ofs_logNet << "Bwjanela = " << failbw << endl;
				ofs_logNet << "Bwmax = " << failbwmax << endl;
				ofs_logNet << "Cwnd = " << failcwnd << endl;
				ofs_logNet << "***************************" << endl;
				ofs_logNet << "****** Estado Atual  ******" << endl;
				ofs_logNet << "Bwjanela = " << *novabw << endl;
				ofs_logNet << "Bwmax = " << Bwmax << endl;
				ofs_logNet << "Cwnd = " << *Cwnd << endl;
				ofs_logNet << "***************************" << endl;
#endif
				/* Testa as novas condições da rede.
				 * Se elas forem parecidas com o estado armazenado então não sobe de camada.
				 */
				if (   ((failbw*0.95 < *novabw) && (*novabw < failbw*1.05))
					&& ((failbwmax*0.95 < Bwmax) && (Bwmax < failbwmax*1.05))
					/*&& ((failcwnd*0.90 < *Cwnd) && (*Cwnd < failcwnd*1.10))*/		)			
				{	
#ifdef _LOG_NETSTATE
					ofs_logNet << "Estado atual parecido com um de falha!\t Não vai subir!" << endl << endl;
#endif
					return -1;
				}
#ifdef _LOG_NETSTATE
			ofs_logNet << "Estado atual diferente!" << endl << endl;
#endif
			}
		}
#ifdef _LOG_NETSTATE
			ofs_logNet << "Estado atual diferente de todos armazenados! Vai tentar subir!" << endl << endl;
#endif	
	}
#endif
	
	unsigned int bwup;
	unsigned int Bwatual = core_getRatesCumActual();
	if (core_getActualLayer() == core_getTotalLayers()-1)
		bwup = Bwatual;
	else
		bwup = core_getRatesCum(core_getActualLayer()+1);	
	
	while (*novabw > bwup) {
		if (bwup >= Bwmax) {
#ifdef _ALMTF_DEBUG2
			cout << "Banda Maxima Atingida!" << endl;
#endif
			return -1;
		}
		if (core_getActualLayer() >= core_getTotalLayers()-1) {
#ifdef _ALMTF_DEBUG2
			cout << "Camada Máxima Atingida!" << endl;
#endif
			return -1;
		}
#ifdef _LOG_LOG
		stringstream ss;
		ss << time_getnow_sec() <<","<<time_getnow_usec()<<"us:\t Bwjanela > bwup, vai SUBIR camada! "<<core_getActualLayer()+1<<endl;
		ss << "\t Bwjanela = "<<*novabw<<endl;
		ss << "\t bwup = "<<bwup<<endl;
		ofs_log << ss.str();
#endif
		core_addLayer();
		
#ifdef _LOG_LAYERS
		stringstream logSS("");
		char _b[20];
		memset(_b,0,sizeof(_b));
		struct timeval _timenow;
		time_getus(&_timenow);
		sprintf(_b, "%.6d", (int)_timenow.tv_usec);
		logSS << _timenow.tv_sec << "," << _b << "\t ";
		logSS << core_getActualLayer() << endl;
		ofs_logLayers << logSS.str();
#endif
		
		//Bwatual = core_getRatesCumActual();
		if (core_getActualLayer() == core_getTotalLayers()-1) { 
			//Está na última camada,não tem mais o que subir.
			bwup = Bwatual;
			return -1;
		}
		else bwup =  core_getRatesCum(core_getActualLayer());
		if (*novabw > bwup) {
			*novabw = (Bwatual + bwup) / 2;
#ifdef _LOG_BWJANELA
			ofs_logBwjanela << "if (*novabw > bwup)\t\tBwjanela=" << *novabw << endl;
#endif
#ifdef _LOG_CWND
			ofs_logCwnd << "if (*novabw > bwup)" << endl;
#endif
			*Cwnd = ALMTF_rate2win(novabw, Cwnd);
		}
	}
	*Time_addLayerWait = timenow;
	time_add_ms(Time_addLayerWait, ALMTF_T_STAB, Time_addLayerWait);
	//Time_addLayerWait=timenow + ALMTF_T_STAB=Evita subir duas camadas menos de Time_stab
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
		if ((almtf_h->flagsALMTF & FL_MEDERTT)) { // && almtf_h->num_receptor == num_nodereceptor_) {
			// calcula o RTT precisamente, conforme dados vindos do transmissor				
			time_subtract(timepkt, &almtf_h->timestampEcho, &rtt_medido);
			time_subtract(&rtt_medido, &almtf_h->timestampOffset, &rtt_medido);				
			//rtt_medido = (double)now-(double)almtf_h->timestamp_echo-(double)almtf_h->timestamp_offset;
			if (time_compare_us(&rtt_medido, 0) < 0) {
			//if (rtt_medido < 0)	
				rtt_medido.tv_sec = 0;
				rtt_medido.tv_usec = 0;
			}
			// atualiza o tempo de ida e a assimetria dos relógios conforme novo rtt medido
			time_div(&rtt_medido, 2.0, &t_vinda);				
			//t_vinda = rtt_medido / 2;
			time_subtract(timepkt, &almtf_h->timestamp, &t_assim);
			time_subtract(&t_assim, &t_vinda, &t_assim);
			//t_assim = now - almtf_h->timestamp - t_vinda;
			FiltraRTT = true;
		} else {
			time_subtract(timepkt, &almtf_h->timestamp, &rtt_medido);
			time_subtract(&rtt_medido, &t_assim, &rtt_medido);
			time_add(&rtt_medido, &t_vinda, &rtt_medido);
			//rtt_medido = t_vinda + ((double)now-(double)almtf_h->timestamp-t_assim);				
			if (time_compare_us(&rtt_medido, 0) < 0) {
				rtt_medido.tv_sec = 0;
				rtt_medido.tv_usec = 0;
			}
		}
		if (FiltraRTT) {
			if (time_compare(&ctpRTT,&rtt_medido) > 0) {
				struct timeval tmp1;
				time_mul(&rtt_medido, 0.1, &tmp1);
				time_mul(&ctpRTT, 0.9, &ctpRTT);
				time_add(&ctpRTT, &tmp1, &ctpRTT);
				//RTT.tv_sec = 0.9*RTT.tv_sec + 0.1*rtt_medido.tv_sec;
				//RTT.tv_usec = 0.9*RTT.tv_usec + 0.1*rtt_medido.tv_usec;
			} else {						
				struct timeval tmpRTT;
				time_mul(&ctpRTT, 1.25, &tmpRTT);
				if (time_compare(&rtt_medido, &tmpRTT) > 0)						
					ctpRTT = rtt_medido;
			}
			//if (rtt_medido < RTT) // se rtt medido menor => banda maior - filtra para nao subir muito rapido
			//	RTT = 0.9 * RTT + 0.1 * rtt_medido; // medicao do pacote vale apenas 10% do rtt
			//else
			//	if (rtt_medido > RTT*1.25) // se rtt medido maior que 25% do anterior atualiza
			//		RTT = rtt_medido;	
		} else {
			time_mul(&t_vinda, 2.0, &ctpRTT);
			//RTT = 2 * t_vinda; // supoe link simetrico na primeira vez // TODO t_vinda = RTT/2 ali em cima .. entõa aqui o RTT deve continuar mesma coisa, nem precisa esse código
			FiltraRTT = true; // todas outras medidas utiliza o filtro para deixar mais estavel
		}		
		//////// Fim do calculo de RTT /////////
		if ((almtf_h->flagsALMTF & FL_MEDERTT) == 0) {   // se flag desligada, entao campo significa num receptores
			Numrec = almtf_h->numReceptor;
			if (Numrec > 60) // maximo 60 receptores para calculo de intervalo entre feedbacks
				Numrec = 60;
		}
	}
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
	pthread_mutex_unlock(&cptNumloss_mutex);	
}

void ALMTF_recvPkt(ALMTFRecLayer *layer, transm_packet* pkt, struct timeval *timepkt)
{
	hdr_almtf* almtf_h = &(pkt->header);
	if (layer->layerID == 0) 
	{		
#ifdef SINCRONISMO_JOIN
		if (almtf_h->sincjoin > (unsigned)Sincjoin_recv)
			Sincjoin_recv = almtf_h->sincjoin;
#else
		Sincjoin_recv = 100;
#endif
		pthread_mutex_lock(&cptRTT_mutex);
		RTT = ALMTF_calculaRTT(layer->seqno, pkt, timepkt, RTT);
		pthread_mutex_unlock(&cptRTT_mutex);
	}
	// verifica perda de pacotes
	layer->seqno = almtf_h->seqno;
    if (layer->seqno_expected >= 0) {
        int loss = layer->seqno - layer->seqno_expected; // loss representa o num de pacotes perdidos em rajada
        if (loss > 0) {
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
	i->Bwatual = core_getRatesCumActual();
	i->Bweq = Info.Bweq;
	i->Bwjanela = Info.Bwjanela;
	i->Bwmax = Bwmax;
	struct timeval tmpRTT = ALMTF_getRTT();
	double dbRTT = time_timeval2double(&tmpRTT);
	i->RTT = dbRTT*1000000;
	i->Numloss = ALMTF_getNumloss();
}
#endif

#ifdef NETSTATE
void init_netvars(NETSTATEvars *net)
{
	net->Bwjanela = 0;
	net->Bwmax = 0;
	net->Cwnd = 0;
	net->addTime.tv_sec = 0;
	net->addTime.tv_usec = 0;
}

void save_netvars(NETSTATEvars *net, double Bwjanela, double Bwmax, double Cwnd)
{
	net->Bwjanela = Bwjanela;
	net->Bwmax = Bwmax;
	net->Cwnd = Cwnd;
	time_getus(&(net->addTime));
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

int main(int argc, char *argv[])
{
	utils_init();
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
