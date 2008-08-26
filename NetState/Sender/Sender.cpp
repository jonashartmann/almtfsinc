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

#include "Sender.h"
#include "../Almtf.h"
#include "../Utils/Utils.h"
#include "../Utils/Socket_Utils.h"

bool SenderRunning = true; // ainda está rodando o transmissor?
bool TimeLimited= TIME_LIMITED; // limitar o tempo de execução. pode ser passado como parâmetro
unsigned long TimeLimit= TIME_LIMIT; // tempo de execução em ms. pode ser passado como parâmetro da aplicação
list<ALMTFSenderLayer> LayerList; // lista de camadas .. a primeira camada criada fica no início da lista
int TotalLayers = 0; // número de camadas lidas do arquivo

// Variáveis para o socket que espera mensagens vindas dos receptores
int ListenSocket = 0; // socket que aguarda mensagens dos receptores
unsigned int ListenPort = 0; // porta utilizada pelo ListenSocket
list<string> Printlist; // lista de strings para impressão na tela
#ifdef SINCRONISMO_JOIN
#ifdef SINCRONISMO_JOIN_1
	static const int vet_sinc[] = { 1, 2, 1, 3, 1, 2, 1, 4 };
#else
#ifdef SINCRONISMO_JOIN_2
	static const int vet_sinc[] = { 2, 1, 3, 1, 2, 3, 4, 1 };
#else
#ifdef SINCRONISMO_JOIN_3
	static const int vet_sinc[] = { 3, 1, 2, 3, 4, 1, 2, 3 };
#else
#ifdef SINCRONISMO_JOIN_4
	static const int vet_sinc[] = { 4, 2, 1, 3, 1, 2, 3, 1 };
#endif
#endif
#endif
#endif	
#endif
#ifdef TIME_SLOTS
struct timeval time_slot;
#endif

#ifdef _WIN32_

DWORD dwPrintfThread;
HANDLE haPrintfThread;
DWORD dwRecvThread;
HANDLE haRecvThread;
DWORD dwThreadEvent;
HANDLE *haThreadEvent;

#else

pthread_t ptRecvThread; // Thread que recebe os pacotes
pthread_t ptPrintfThread;
pthread_mutex_t *condition_layer_mutex;
pthread_cond_t *condition_layer_cond;
int LayerCount = 0;

#endif

#ifdef _LOG_DEBUG
ofstream Ofs_logDebug;
#endif

#ifdef _LOG_SENDER_INFO
ofstream Ofs_logSendinfo;
#endif

/*
 * Thread que varre a lista de mensagem 'Printlist' e imprime as mensagens
 * na saída 'cout'.
 */
void *printf_thread(void *lc) {
	list<string> *l = (list<string> *)lc;
	while (SenderRunning) {
		if (!l->empty()) {
			cout << l->front() << endl;
			l->pop_front();
		} else {
#ifdef _WIN32_
			Sleep(200);
#else
			pthread_testcancel();
			usleep(500000);
#endif
		}
	}
	return (NULL);
}

/*
 * Lê o arquivo com os grupos multicast.
 */
bool create_layers() {
	// abre o arquivo com as configurações
	ifstream file(FILENAME_IN_GROUPS);
	if (!file.is_open()) {
		cerr << "ERRO abrindo arquivo de configuracao!" << endl;
		return false;
	} else
		cerr << "Arquivo de configuracao lido com SUCESSO!" << endl;

	// carrega todas as configurações e cria os agentes para transmissão
	if (!file.eof())
		file >> ListenPort; // pega a porta que o servidor usará para esperar mensagens dos receptores
	int id = 0;
	while (!file.eof()) {
		char IP[16];
		bzero(IP, sizeof(IP));
		int port;
		long int speed;
		file >> IP;
		file >> speed;
		file >> port;
		if (IP[0] != '\0') {
			// cria a struct que representa uma camada e coloca ela na lista
			ALMTFSenderLayer layer;
			strcpy(layer.IP, IP);
			layer.speed = speed;
			layer.port = port;
			layer.layerID = id;
			// as outras propriedades da camada são setadas em layer_start()
			LayerList.push_back(layer); // coloca a camada no FIM da lista			
			id++;			
		}
	}
	file.close();
	TotalLayers = id;	
	if (TotalLayers == 0)
		return false;

	// imprime a lista de camadas disponibilizadas
	stringstream ss("");
	ss << endl << "Grupos multicast:" << endl;
	ss << "----------------------------------------" << endl;
	if (!LayerList.empty()) {
		list<ALMTFSenderLayer>::iterator i;
		for (i = LayerList.begin(); i != LayerList.end(); ++i) {
			ss << "Layer " << (*i).layerID << ": IP " << (*i).IP;
			ss << ", Speed " << (*i).speed << endl;
		}
		ss << "----------------------------------------" << endl;
		Printlist.push_back(ss.str());
	}
	return true;
}

bool all_threads_finished(void) {
	bool v = true;
	if (!LayerList.empty()) {
		list<ALMTFSenderLayer>::iterator i;
		for (i = LayerList.begin(); i != LayerList.end() && v; ++i)
			v = v && (*i).threadFinished;
	}
	return v;
}

bool finish_execution(void) {
	if (!LayerList.empty()) {
		list<ALMTFSenderLayer>::iterator i;
		for (i = LayerList.begin(); i != LayerList.end(); ++i) {
#ifdef _WIN32_
			if (CloseHandle((*i).sendThreadHD) != 0) {
				cerr << "Layer " << (*i).layerID << ": finalizando..." << endl;
			}
#else
			if (pthread_cancel((*i).sendThreadID) == 0) {
				cerr << "Layer " << (*i).layerID << ": finalizando..." << endl;
			}
#endif
		}
	}
	return true;
}

/*
 * Verifica os argumentos 'argv' e 'argc' vindos da linha de comando e seta
 * as variáveis correspondentes.
 */
void get_args(int argc, char *argv[]) {
	// busca os argumentos se existem
	if (argc > 1) {
		TimeLimit = atoi(argv[1]); // primeiro argumento é o tempo limite
		if (TimeLimit == 0) // se zero, sem limite de tempo
			TimeLimited = false;
	}
	stringstream ss("");
	if (TimeLimited)
		ss << "Limite de tempo: "<<TimeLimit<<" ms"<<endl;
	else
		ss << "Sem limite de tempo"<<endl;
	ss << "----------------------------------------" << endl;
	Printlist.push_back(ss.str());
}

#ifdef SINCRONISMO_JOIN

void init_sincvars(ALMTFSincVar *vrSinc) {
	vrSinc->sincjoin = 1; // sincronismo do join - indica até qual camada pode ser dado join
	vrSinc->div_sinc = (double)TotalLayers/4; // TotalLayers é a última camada q o receptor pode se inscrever
	vrSinc->ind_sinc = 0;
	vrSinc->time_ultsinc.tv_sec=0; // ultima vez que foi enviado sincjoin
	vrSinc->time_ultsinc.tv_usec=0;
	vrSinc->deltasinc=0; // intervalo entre sincronismos
}

unsigned int set_sincvar(ALMTFSincVar *vrSinc) {
	struct timeval time_now;
	time_getus(&time_now);
	struct timeval timetmp;
	timetmp = time_double2timeval(vrSinc->deltasinc);
	time_add(&timetmp, &vrSinc->time_ultsinc, &timetmp);
	if (time_compare(&time_now, &timetmp) > 0) {
		double r = utils_rand();
		vrSinc->deltasinc = ((double)r/RAND_MAX)*ALMTF_DTSINC
				+ (double)ALMTF_DTSINC/2; // valores que, na media, vao dar o atraso estipulado na simulacao
		vrSinc->time_ultsinc = time_now;
		vrSinc->sincjoin = (int)(vet_sinc[vrSinc->ind_sinc]*vrSinc->div_sinc);
		if (++vrSinc->ind_sinc > 7)
			vrSinc->ind_sinc=0;
	}
	return (vrSinc->sincjoin); // envia numero da camada ate a qual os receptores podem fazer join
}

#endif

/* 
 * Inicia a camada 'layer' e dispara as threads para envio dos dados.
 */
void start_layers()
{
	// Inicializa e da start em todas as camadas
	list<ALMTFSenderLayer>::iterator i;
	if (!LayerList.empty()) {
		for (i = LayerList.begin(); i != LayerList.end(); ++i) {
			layer_start(*i);
		}
	}
}

void layer_start(ALMTFSenderLayer &layer)
{
	layer.numPackets = 0;	
	layer.seqno = 0;
	layer.active = true;
	layer.sendInfRTT = false;
	layer.PP_numPkt = 0;
	layer.totalRecv = 0;
	layer.lastReportRecv.tv_sec = 0;
	layer.lastReportRecv.tv_usec = 0;
	layer.numNodereceptorEcho = 0;
	layer.timestampEcho.tv_sec = 0;
	layer.timestampEcho.tv_usec = 0;
	if (ALMTF_PP)
		layer.burstSize = ALMTF_PP_BURST_LENGTH;
	else
		layer.burstSize = 1;

#ifdef _WIN32_
	// cria a thread que vai fazer o envio dos dados
	layer.sendThreadHD = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) sendThread, (void *)&layer, 0,
			&(layer.sendThreadID));
	if (layer.sendThreadHD == NULL)
	cerr << "CreateThread(Sender Layer "<< layer.layerID << " ) Error: " << GetLastError() << endl;
	dwThreadEvent = WaitForMultipleObjects(TotalLayers, haThreadEvent, FALSE, 2000);
	if (dwThreadEvent == WAIT_TIMEOUT)
	cerr << "CreateThread Layer " << layer.layerID << "Timed out!" << endl;
#else
	// Cria a thread que vai fazer o envio dos dados
	if (pthread_create(&(layer.sendThreadID), NULL, sendThread, (void *)&layer) != 0) {
		cerr << "Error Creating Thread!" << endl;
	}
#endif
}

/*
 * Função executada em uma thread para envio dos dados.
 * Cria o pacote de dados, envia e calcula o tempo de envio do próximo pacote.
 */
void *sendThread(void *layerc)
{
	ALMTFSenderLayer *layer = (ALMTFSenderLayer *)layerc;	
	pthread_mutex_lock(&condition_layer_mutex[layer->layerID]);
	// Bloqueia até a thread anterior iniciar e desbloquear a thread seguinte
	while (LayerCount != (layer->layerID+1)) {
		   pthread_cond_wait(&condition_layer_cond[layer->layerID], &condition_layer_mutex[layer->layerID]);
	}
	pthread_mutex_unlock(&condition_layer_mutex[layer->layerID]);

#ifdef SINCRONISMO_JOIN
	// Variáveis de Sincronismo
	ALMTFSincVar vrSinc;
	if (layer->layerID == 0) {
		init_sincvars(&vrSinc);
	}
#endif
	
	transm_packet tpkt; // pacote de transmissão	
	layer->threadFinished = false; // inicia a thread
	// abre o arquivo com os dados a serem enviados
	ifstream file(FILENAME_DATA,ifstream::in);
	if (!file.is_open()) {
		cerr << "Error opening DATA file!" << endl;
		exit(1);
	}
	file.read(tpkt.buffer, ALMTF_PACKETSIZE);
	file.close();
	
	// imprime informações
	stringstream ss("");
	ss << "Layer " << layer->layerID << ": iniciando...";
	Printlist.push_back(ss.str());
	// cria a conexão com o grupo multicast
	if (!sock_sender_MCastCon(&layer->socketSendMC, layer->IP, layer->port)) {
		cerr << "Não foi possível criar o socket para envio da msg para o transmissor!" << endl;
		exit(1);
	}
	
	struct timeval time_now;
	time_getus(&time_now);
	struct timeval raj_next; // tempo para envio da próxima rajada em us
	struct timeval raj_step; // tempo entre uma rajada e outra em us
	// Calcula o intervalo entre as rajadas
	// Número de pacotes por segundo
	int packets_per_sec = (layer->speed*1000) / (ALMTF_PACKETSIZE*8);
	// Número de rajadas por segundo, caso seja 2 pacotes em rajada a uma taxa de 1024 Kbps, então serão 128 rajadas por segundo
	int burst_per_sec = packets_per_sec / layer->burstSize;
	// Step de 1 segundo, então deverá ser enviado um pacote a cada 0.0078125 s = 7812.5us
	double d_raj_step = 1.0 / burst_per_sec;
	raj_step = time_double2timeval(d_raj_step);
	raj_next = time_now;
	// Tempo para a pŕoxima rajada será o tempo atual + o tempo de step da rajada
	time_add(&raj_next, &raj_step, &raj_next);
	struct timeval raj_step_x3;
	// Tempo de step da rajada multiplicado por 3
	time_mul(&raj_step, 3.0, &raj_step_x3);
	// Libera a execução da próxima camada, apenas para sincronismo de início de execução das camadas
	pthread_mutex_lock(&condition_layer_mutex[layer->layerID]);
	// Se chegou na última camada não existe pŕoxima camada para desbloqueiar
	if (LayerCount <= TotalLayers) {
		LayerCount++;
		// Envia o signal para a próxima camada se desbloquear e iniciar a execução
		pthread_cond_signal( &condition_layer_cond[layer->layerID + 1]);
	}
	pthread_mutex_unlock(&condition_layer_mutex[layer->layerID]);	
	// Enquanto a camada estiver ativa, permanecerá enviando os pacotes
	struct timeval timesleep;
	struct timeval raj_tmp;

	while (layer->active) {
		// Chama a função que realmente envia o pacote
#ifdef SINCRONISMO_JOIN
		if (!sendpkt(layer, tpkt, &vrSinc))
			break;
#else   
		if (!sendpkt(layer, tpkt))
			break;
#endif
		// se chegou no último pacote da rajada dorme, senão segue direto para o próximo
		if (layer->PP_numPkt >= (layer->burstSize - 1)) {
			layer->PP_numPkt = 0;
			time_getus(&time_now);
			// Proteção caso ocorra algum erro que tranque a execução do programa
			// Se a próxima rajada estiver muito atrasada, ignora o tempo perdido
			time_subtract(&time_now, &raj_step_x3, &raj_tmp);
			if (time_compare_s(&time_now, 1) > 0 && time_compare(&raj_tmp, &raj_next) > 0) {
				raj_next = time_now;
			}
			// tempo para a próxima rajada
			time_add(&raj_next, &raj_step, &raj_next);
			// espera o tempo para a próxima rajada
			time_subtract(&raj_next, &time_now, &timesleep);
			
/*
			struct timeval tantes;			
			if (layer->layerID == 0) {
				time_getus(&tantes);
				Ofs_logDebug<< time_getnow_sec() << "," << time_getnow_usec() << ": Camada: "<<layer->layerID << endl;
				Ofs_logDebug<<"timesleep: " << timesleep.tv_sec << "," << timesleep.tv_usec << endl;
			}
*/
			
			if (timesleep.tv_sec >= 0 && timesleep.tv_usec >= 0) { //Se for negativo está atrasado e portando não dorme...				
				time_usleep(&timesleep);
			}
			
/*
			struct timeval tdepois;
			if (layer->layerID == 0) {
				time_getus(&tdepois);
				Ofs_logDebug<<"\t tantes: " << tantes.tv_sec << ","	<< tantes.tv_usec << endl;
				Ofs_logDebug<<"\t tdepois: " << tdepois.tv_sec << "," << tdepois.tv_usec << endl;
				time_subtract(&tdepois, &tantes, &tdepois);
				Ofs_logDebug<< time_getnow_sec() << "," << time_getnow_usec() << ":" << endl;
				Ofs_logDebug<<"\t time de sleep: "<<(timesleep.tv_sec*1000000)+(timesleep.tv_usec)<<"us"<<endl;
				Ofs_logDebug<<"\t time  efetivo: "<<(tdepois.tv_sec*1000000)+(tdepois.tv_usec)<<"us"<<endl;
				Ofs_logDebug<<"-------------------------------------"<<endl;
			}
*/
			
		} else {
			layer->PP_numPkt++;
		}

/*		// Mensagem quando enviar +1000 pacotes	
		if (layer->numPackets % 256 == 0)  {
			ss.str("");
			ss << "Layer " << layer->layerID << ": +1000 pacotes enviados..."<<endl;
			for(int i = -1; i < layer->layerID; i ++) ss << "  ";
			Printlist.push_back(ss.str());			
		 }
*/		
		
		// Cria um ponto para finalização da thread
		pthread_testcancel();
	}
	layer->threadFinished = true;
	sock_close(&layer->socketSendMC); // finaliza o socket
	return (NULL);
}

/*
 * Função executada em uma thread para recepção das mensagens vindas dos receptores.
 */
#ifdef _WIN32_
DWORD recvThread(ALMTFSenderLayer *layer)
#else
void *recvThread(void *layerc)
#endif
{
#ifndef _WIN32_	
	ALMTFSenderLayer *layer = (ALMTFSenderLayer *)layerc;
	pthread_mutex_lock(&condition_layer_mutex[layer->layerID]);
	LayerCount++;
	pthread_cond_signal( &condition_layer_cond[layer->layerID]);
	pthread_mutex_unlock(&condition_layer_mutex[layer->layerID]);
#endif
	struct timeval time_now;
	sock_listenCon(&ListenSocket, ListenPort); // conecta para esperar msgs na porta ListenPort
	transm_packet tpkt;
	string s;
	unsigned int ip[4];
	memset(ip, 0, sizeof(ip));
	// enquanto o transmissor estiver rodando...
	while (SenderRunning) {
		// espera alguma mensagem chegar
		if (sock_recvMsgServ(&ListenSocket, (char *)&tpkt,
			sizeof(transm_packet), ListenPort) <= 0) {
			s = "\t recvThread: PROBLEMA no recebimento de uma msg dos receptores";
			Printlist.push_back(s);
			continue;
		}
		time_getus(&time_now);
		if (!layer->sendInfRTT) { // so muda variaveis se elas estao liberadas			
			// armazena tempo de chegada do pacote para posterior calculo de offset
			layer->lastReportRecv = time_now;
			// momento que o receptor enviou deve voltar a ele para calculo de RTT
			layer->timestampEcho = tpkt.header.timestamp;
			// ecoa o IP do receptor para evitar confusao qdo 2 receptores pedem RTT simultaneamente			
#ifdef _LOG_SENDER_INFO			
			getrecvIP(tpkt.header.numReceptor, (unsigned int *)ip);
			Ofs_logSendinfo << "------------------------------" << endl;
			Ofs_logSendinfo << "Receptor Ativo:				  " << endl;
			Ofs_logSendinfo << "IP: "<<ip[0]<<"."<<ip[1]<<"."<<ip[2]<<"."<<ip[3]<<endl;
			Ofs_logSendinfo << "------------------------------" << endl;
#endif			
			layer->numNodereceptorEcho= tpkt.header.numReceptor;
			// avisa que no proximo pacote deve enviar tempo para receptor calcular RTT
			layer->sendInfRTT = true;
		}
		layer->totalRecv = layer_calc_num_rec(*layer, tpkt.header.numReceptor,
				&time_now); // calcula numero de receptores e atribui para "m_totreceptores"
#ifndef _WIN32_
		pthread_testcancel();
#endif
	}
	sock_close(&ListenSocket);
	return (NULL);
}

/*
 * Calcula número de receptores atualmente enviando feedback ao transmissor.
 * Atualiza variável totalRecv.
 */
int layer_calc_num_rec(ALMTFSenderLayer &layer, unsigned int argRec,
		struct timeval *argTime) {
	static struct tbl_rec *tbl_start= NULL;
	static int totreceptores = 0; // total de receptores enviando feedback ao transmissor
	struct tbl_rec *p, *ant;
	int found=0;
	p = tbl_start;
	ant = NULL;
	// Procura pelo receptor na lista encadeada
	while (p!=NULL) {
		if (p->rec == argRec && !found) {
			found = 1;
			p->time = *argTime;
			ant = p;
			p = p->next;
		} else {
			// se receptor nao enviou feedback em 120s, elimina da lista
			struct timeval time_dif;
			time_subtract(argTime, &(p->time), &time_dif);
			if (time_compare_s(&time_dif, 120) >= 0) { // argTime-(p->time) >= 120
				if (ant == NULL)
					tbl_start = p->next;
				else
					ant->next = p->next;
				delete p;
				p = ant->next;
				totreceptores--;
			} else {
				ant = p;
				p = p->next;
			}
		}
	}
	// Caso não encontre o receptor inclui um novo
	if (!found) {
		p = new struct tbl_rec;
		p->rec = argRec;
		p->time = *argTime;
		p->next = tbl_start;
		tbl_start = p;
		totreceptores++;
	}
	return totreceptores;
}

/*
 * Recupera o número IP recebido dos receptores.
 */
void getrecvIP(unsigned int intip, unsigned int *r) {
	int a, b, c, d;
	unsigned int tmp;
	a = (intip / 16777216);
	tmp = intip - a*16777216;
	b = (tmp / 65536);
	tmp = tmp - b*65536;
	c = (tmp / 256);
	d = tmp - c*256;
	r[0]=a;
	r[1]=b;
	r[2]=c;
	r[3]=d;
}

/*
 * Monta o pacote para enviar para os receptores 
 */
#ifdef SINCRONISMO_JOIN
bool sendpkt(ALMTFSenderLayer *layer, transm_packet &tpkt, ALMTFSincVar *vrSinc)
#else
bool sendpkt(ALMTFSenderLayer *layer, transm_packet &tpkt)
#endif
{
	struct timeval time_now;
	if (!layer->active)
		return false;
	tpkt.header.timestampOffset.tv_sec = 0;
	tpkt.header.timestampOffset.tv_usec = 0;
	tpkt.header.timestampEcho.tv_sec = 0;
	tpkt.header.timestampEcho.tv_usec = 0;
	tpkt.header.numReceptor = 0;
	tpkt.header.timestamp.tv_sec = 0;
	tpkt.header.timestamp.tv_usec = 0;
	time_getus(&time_now);
	tpkt.header.seqno = layer->seqno++;
	tpkt.header.flagsALMTF = 0; // Desliga flags para garantir
	tpkt.header.sincjoin = TotalLayers;	// Deixa por padrão poder fazer join em todas as camadas
#ifdef SINCRONISMO_JOIN
	/* Usando um vetor de sincronização */
	if (layer->layerID == 0) { // Tem que ser camada zero pois garante que todos receptores estao inscritos
		tpkt.header.sincjoin = set_sincvar(vrSinc); // Envia numero da camada ate a qual os receptores podem fazer join
	}
#endif
#ifdef TIME_SLOTS
	/* Usando marcadores de time slots no pacote */
	time_subtract(&time_now, &time_slot, &time_slot);
	double auxdiff = time_timeval2double(&time_slot);
	if (auxdiff > N_ADD/1000){
		tpkt.header.flagsALMTF |= FL_ADDLAYER;
		time_getus(&time_slot);		// recomeça a contagem do tempo
	}
#endif
	// Envia informacoes para calculo de RTT
	if (layer->sendInfRTT) {
		// Envia offset em ms para receptor da camada zero calcular RTT
		time_subtract(&time_now, &(layer->lastReportRecv), &tpkt.header.timestampOffset);
		// Momento que o receptor enviou report
		tpkt.header.timestampEcho = layer->timestampEcho;
		// IP do receptor (num. do nodo)		
		tpkt.header.numReceptor = layer->numNodereceptorEcho;
		// Avisa que timestamp_offset eh valido
		tpkt.header.flagsALMTF |= FL_MEDERTT;
		layer->sendInfRTT = false;
	} else {
		tpkt.header.numReceptor = layer->totalRecv; // Mesmo campo compartilhado para total de receptores (ver tese)
	}
	
	if (layer->PP_numPkt == 0) {
		tpkt.header.flagsALMTF |= FL_INIRAJADA; // Liga flag que avisa inicio de nova rajada
	}
	
	// Todos pacotes layer 0: envia momento da transmissao para todos receptores calcularem RTT de volta
	tpkt.header.timestamp = time_now;
	// Envia o pacote
	if (!sock_sendMsg(&layer->socketSendMC, (char *)&tpkt, sizeof(transm_packet), layer->IP, layer->port)) {
		return false;
	}
	layer->numPackets++; // Mais um pacote enviado na contagem geral
	return true;
}

int main(int argc, char *argv[]) {
#ifdef _WIN32_
	// inicia a dll do winsock
	WSADATA wsaData;
	if (WSAStartup (MAKEWORD (2, 2), &wsaData) != 0)
	return false;
#endif
	utils_init();
	//Guarda o tempo de início da aplicação
	struct timeval time_start;
	time_getus(&time_start);
#ifdef _LOG_DEBUG
	Ofs_logDebug.open("LOG_DEBUG.txt");
#endif
#ifdef _LOG_SENDER_INFO
	Ofs_logSendinfo.open("LOG_INFO.txt");
#endif
#ifdef _WIN32_
	haPrintfThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)printf_thread, (void *)&Printlist, 0, &dwPrintfThread);
	if (haPrintfThread == NULL)
	cerr << "CreateThread Error - Printf Thread Error:  " << GetLastError() << endl;
#else	
	// cria a thread para imprimir na tela as informações dos agentes	
	if (pthread_create(&ptPrintfThread, NULL, printf_thread, (void *)&Printlist) != 0) {
		cerr << "Error Creating Thread!" << endl;
	}
#endif
	// pega os argumentos
	get_args(argc, argv);
	// cria as camadas e inicia a execução delas
	if (!create_layers()) {
		cout << "Nenhuma camada foi especificada no arquivo de configuracoes." << endl;		
		exit(1);
	}	

#ifdef _WIN32_
	int k;
	haThreadEvent = new HANDLE[TotalLayers];
	for (k = 0; k < TotalLayers; k++) {
		haThreadEvent[k] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (haThreadEvent == NULL)
		cerr << "CreateEvent Error: " << GetLastError() << endl;
	}
#else
	condition_layer_mutex = new pthread_mutex_t[sizeof(pthread_mutex_t)*TotalLayers];
	condition_layer_cond = new pthread_cond_t[sizeof(pthread_cond_t)*TotalLayers];
	int k;
	for (k = 0; k < TotalLayers; k++) {
		pthread_mutex_init(&condition_layer_mutex[k], NULL);
		pthread_cond_init(&condition_layer_cond[k], NULL);
	}
#endif
	start_layers();
	// cria a thread para receber dados vindos dos receptores
	ALMTFSenderLayer& firstLayer = LayerList.front();
#ifndef _WIN32_	
	if (pthread_create(&ptRecvThread, NULL, recvThread, (void *)&firstLayer)
			!= 0) {
		cerr << "Error Creating Thread!" << endl;
	}
#else
	haRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) recvThread, (void *)&firstLayer, 0, &dwRecvThread);
	if (haRecvThread == NULL)
	cerr << "CreateThread(Receive Thread) Error: " << GetLastError() << endl;
#endif	
	// se for limitado por tempo, fica esperando todos os agentes acabarem
	if (TimeLimited) {
		while (SenderRunning) {
			struct timeval time_exec;
			time_exec.tv_sec = 0;
			time_exec.tv_usec = 0;
			time_getus(&time_exec);
			time_subtract(&time_exec, &time_start, &time_exec);
			// se atingiu o tempo limite, desativa todas as camadas...
			if (TimeLimit > 0 && time_compare_us(&time_exec, TimeLimit*1000)
					>= 0) {
				// Primeiro parar de enviar, depois parar de receber, depois finalizar thread de impressão
				pthread_cancel(ptRecvThread);
				finish_execution();
				pthread_cancel(ptPrintfThread);
				sleep(2); // espera um tempo para todas as threads encerrarem a execução
				break;
			} else
				sleep(1);
		}
	} else
		while (SenderRunning)
			sleep(300);
	SenderRunning = false;
#ifdef _WIN32_
	for (k = 0; k < TotalLayers; k++)
	CloseHandle(haThreadEvent[k]);
	utils_close();
#else
	for (k = 0; k < TotalLayers; k++) {
		pthread_mutex_destroy(&condition_layer_mutex[k]);
		pthread_cond_destroy(&condition_layer_cond[k]);
	}
#endif
#ifdef _LOG_DEBUG
	Ofs_logDebug.close();
#endif
#ifdef _LOG_SENDER_INFO
	Ofs_logSendinfo.close();
#endif
	return true;
}

