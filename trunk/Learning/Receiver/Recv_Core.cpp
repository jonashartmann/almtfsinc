/*
 *   Universidade Federal do Rio Grande do Sul - UFRGS
 *   PRAV - Projetos de Audio e Vídeo 
 *   Projeto SAM - Sistema Adaptativo Multimedia
 *
 *	 Copyright(C) 2008 - Andrea Collin Krob <ackrob@inf.ufrgs.br>
 *                       Alberto Junior     <e-mail>
 *                       Athos Fontanari    <aalfontanari@inf.ufrgs.br>
 *                       João Victor Portal <e-mail>
 *                       Leonardo Daronco   <e-mail>
 *                       Valter Roesler     <e-mail>
 *
 *
 *     Recv_Core.cpp: Funções do Núcleo do Receptor ALMTF.
 *
 */

#include "Recv_Core.h"

#ifdef _LOG_LAYERS
ofstream ofs_logLayersCore;
#endif
#ifdef _LOG_COREDEBUG
ofstream ofs_logCoreDebug;
#endif
#ifdef _LOG_STATUS
ofstream ofs_logCoreStatus;
#endif

deque<ALMTFRecLayer> LayerList;	// lista de camadas .. a primeira camada criada fica no início da lista
int TotalLayers = 0;								// número de camadas lidas do arquivo
int *RatesCum;									// vetor de taxas cumulativas das camadas
int CurrentLayer = 0;							// camada atual
int LogTime = LOG_TIME_LIMIT;			// tempo de gravação do log
int LogStep = LOG_TIME_STEP;			// intervalo de gravação do log

// TODO por enquanto considera que existe apenas um servidor enviando os dados
bool ServerKnown = false;			// o servidor que está enviando os dados é conhecido?
char *ServerIP;
int ServerPort = 0;

struct timeval TimeLastPkt; // Momento de tempo em que foi recebido o último pacote. Usado para verificar timeout
struct timeval TimeLastPktLayerZero;
struct timeval TimeLastTimestamp;

pthread_t IDthreadprintf;
pthread_t IDthreadlogsp;
pthread_t IDthreadgraph;

list<string> recv_printlist;	// lista de strings para impressão na tela

void core_init()
{
#ifdef _LOG_LAYERS
	ofs_logLayersCore.open("LOG_LayersCore.txt");
#endif
#ifdef _LOG_COREDEBUG
	ofs_logCoreDebug.open("LOG_CoreDebug.txt");
#endif
#ifdef _LOG_SPEED_AND_PACKETS
	int vec_size = sizeof(int)*(LogTime/LogStep);
#endif	
	// lê o arquivo com os grupos multicast
	core_readConfigs();
	RatesCum = new int[sizeof(int)*TotalLayers];
	for (int k =0; k < TotalLayers; k++)
		 RatesCum[k] = 0;
	// inicializa as camadas
	ServerIP = NULL;
	TimeLastPkt.tv_sec = 0;
	TimeLastPkt.tv_usec = 0;
	TimeLastPktLayerZero.tv_sec = 0;
	TimeLastPktLayerZero.tv_usec = 0;
	TimeLastTimestamp.tv_sec = 0;
	TimeLastTimestamp.tv_usec = 0;
	int sum = 0;
	int id = 0;
	if (!LayerList.empty()) {
		deque<ALMTFRecLayer>::iterator i;		
		id = 0;
		for (i = LayerList.begin(); i != LayerList.end(); i++) {
			 i->thread_running = false;
			 i->thread_enabled = false;
			 i->num_packets = 0;
			 i->layerID = id;			
			 i->nlost = 0;
			 i->seqno = -1;
			 i->seqno_expected = -1;
			 i->seqno_next = -1;
			 i->first_packet.tv_sec = -1;
			 i->first_packet.tv_usec = 0;
			 i->start_time.tv_sec = 0;
			 i->start_time.tv_usec = 0;
			 i->measure = 0;
			 // taxas cumulativas de velocidade
			 sum += i->speed;
			 RatesCum[id] = sum;		
#ifdef _LOG_SPEED_AND_PACKETS
			 // vetores para o log ... TODO				
			 i->packetsXtime =new int[vec_size];
			 i->speedXtime = new int[vec_size];
			 for (unsigned int j = 0; j < (vec_size/sizeof(int)); j++) {
			 	  i->packetsXtime[j] = 0;
				  i->speedXtime[j] = 0;
			 }
#endif
			 id++;
		}
	}
	// inicia sem estar registrado em nenhuma camada
	CurrentLayer = -1;
#ifdef _LOG_SPEED_AND_PACKETS
	// inicia o processo de log
	if (pthread_create(&IDthreadlogsp, NULL, core_logSaveResultsThread, (void *)NULL) != 0) {
		cerr << "Error Creating Thread: core_logSaveResultsThread!" << endl;	
	}
#endif
	// inicia a thread de impressão na tela	
	if (pthread_create(&IDthreadprintf, NULL, core_printThread, (void *)&recv_printlist) != 0) {
		cerr << "Error Creating Thread: core_printThread!" << endl;
	}

#ifdef GRAPH_JAVAINTERFACE
	if (pthread_create(&IDthreadgraph, NULL, GRAPH_sendMsg, (void *)NULL) != 0) {
		cerr << "Error Creating Thread: GRAPH_sendMsg!" << endl;
	}
#endif

}

bool core_readConfigs()
{
	int id = 0;
	// abre o arquivo com as configurações
	ifstream file(FILENAME_IN_GROUPS);
	if (!file.is_open()) {
		cout << "\nError opening file"<<endl;
	    exit(0);
	} else cout << "Succeeded opening file"<<endl;
    // carrega todas as configurações em "stream[]"
	if(!file.eof())
		file >> ServerPort; // pega a porta que o servidor usará para esperar mensagens dos receptores
	while (!file.eof()) {
		char IP[16];
		int port;
		long int speed;
		memset(IP, 0, sizeof(IP));
		file >> IP;
		file >> speed;
		file >> port;
		if(IP[0] != '\0') {
			ALMTFRecLayer layer;
			memcpy(layer.IP, IP, sizeof(IP));
			layer.speed = speed;
			layer.port = port;
			layer.layerID = id;
			// as outras propriedades da camada são setadas em init()
			LayerList.push_back(layer); // coloca a camada no FIM da lista
			id++;
		}
	}
	file.close();
	// achou o número de Layers
	TotalLayers = id;
	if (TotalLayers == 0)
		return false;
	cout << endl << "Grupos multicast:" << endl;
	cout << "----------------------------------------" << endl;
	if (!LayerList.empty()) {
		deque<ALMTFRecLayer>::iterator i;
		for(i = LayerList.begin(); i != LayerList.end(); ++i) {
			cout << "Layer " << (*i).layerID << ": IP " << (*i).IP;
			cout << ", Speed " << (*i).speed << endl;
		}
	}
	cout << "----------------------------------------" << endl << endl;
	return true;
}

void core_addLayer()
{
	stringstream sslist("");
	if (CurrentLayer < TotalLayers-1) { 
		// cria a thread e habilita a execução dela
		ALMTFRecLayer *layer = &LayerList.at(CurrentLayer+1);
		if (!layer->thread_enabled) {
			// cria a thread esperando os pacotes
			layer->thread_enabled = true;
			// thread que recebe os pacotes e armazena em uma lista
#ifdef _WIN32_
			layer->haRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)core_recvThread, (LPVOID)layer, 0, &(layer->dwRecvThread));
#else
			if (pthread_create(&(layer->IDthread), NULL, core_recvThread, (void *)layer) != 0) {
				cerr << "Error Creating Thread: core_recvThread!" << endl;
			}
#endif
			CurrentLayer++;
			sslist << "-------------------------------------------------------"<<endl;
			sslist << "Cadastrou na camada "<<CurrentLayer<<" ("<<layer->speed<<" kbits/s)!";
			sslist << " -> total: ";
			sslist << RatesCum[CurrentLayer];
			sslist <<" kbits/s"<<endl;
			if (CurrentLayer < TotalLayers-1)
				sslist <<"\t\t- para a proxima camada: "<<RatesCum[CurrentLayer+1]<<" kbits/s"<<endl;
			sslist << "-------------------------------------------------------"<<endl;
		} else {
			sslist << "Tentou se cadastrar na camada " << CurrentLayer+1 << " mas já está cadastrado!" << endl;
		}
		recv_printlist.push_back(sslist.str());	
	}
}

void core_leaveLayer()
{	
	stringstream sslist("");
	if (CurrentLayer >= 0) {
		// desabilita a thread para ela encerrar sua execução
		ALMTFRecLayer *layer = &LayerList.at(CurrentLayer);
		layer->thread_enabled = false;
		CurrentLayer--;
		sslist << "-------------------------------------------------------"<<endl;
		sslist << "Deixou a camada "<<(CurrentLayer+1)<<" ("<<layer->speed<<" kbits/s)!";
		sslist << " -> total: ";
		if (CurrentLayer != -1)
			sslist <<RatesCum[CurrentLayer];
		else
			sslist << "0";
		sslist <<" kbits/s"<<endl;
		sslist << "-------------------------------------------------------"<<endl;
	} else 
		sslist << "Receptor: não é possível deixar a camada "<<CurrentLayer<<endl;
	recv_printlist.push_back(sslist.str());
}

bool core_allLayersFinished()
{
	bool v = true;
	deque<ALMTFRecLayer>::iterator i;
	for(i = LayerList.begin(); i != LayerList.end(); i++)
		v = v && !(*i).thread_running;
	return v;
}

#ifdef _LOG_SPEED_AND_PACKETS

void *core_logSaveResultsThread(void *)
{
	struct timeval now;
	time_getus(&now);	
	long ms_next_loop = now.tv_sec*1000 + now.tv_usec/1000 + LogStep;
	int *packets_before = new int[sizeof(int)*TotalLayers]; // total de pacotes recebidos até o ciclo anterior
	for (int i = 0; i < TotalLayers; i++)
		 packets_before[i] = 0;
	int i_log = 0;
	struct timeval start;
	time_getus(&start);
	long startTime = start.tv_sec*1000 + start.tv_usec/1000;
	deque<ALMTFRecLayer>::iterator it;
	int steps_in_sec = 1000/LogStep;
	int nsteps = LogTime / LogStep;
	struct timeval timenow;
	while (1) {
		time_getus(&timenow);
		long time_ms = timenow.tv_sec*1000 + timenow.tv_usec/1000;		
		if (time_ms-startTime <= LogTime || i_log < nsteps)  {
			if (ms_next_loop <= time_ms) {
				// registra os dados da transmissão nos vetores de log				
				int i = 0;
				for (it = LayerList.begin(); it != LayerList.end(); it++) {
					 int bytes_rcv = (it->num_packets-packets_before[i])*(ALMTF_PACKETSIZE);
					 it->packetsXtime[i_log] = it->num_packets-packets_before[i];					
					 packets_before[i] = it->num_packets;
					 it->speedXtime[i_log] = (bytes_rcv*steps_in_sec*8)/1000;					 
					 i++;
				 }
				 i_log++;
				 // Calcula o tempo para o próximo registro
			 	 ms_next_loop += LogStep;
			} else {				
				usleep(100000);
			}
		} else {
			stringstream ss("");
			ss <<endl<< "\t ==================      vai salvar      ================== "<<endl<<endl;
			recv_printlist.push_back(ss.str());			
			usleep(100000);
			core_logSaveResults();			
			ss.str("");
			ss << "\t ================== salvou os resultados ================== "<< endl<<endl;
			recv_printlist.push_back(ss.str());
			break;
		}
	}
	return (NULL);
}

void core_logSaveResults()
{	
#ifdef _LOG_SPEED_AND_PACKETS	
	// técnica de pares de pacotes, não tem como imprimir um número ímpar de pacotes, pois sempre manda uma rajada de 2 pacotes
	// arquivo com o número de pacotes enviados por segundo
	deque<ALMTFRecLayer>::iterator i;
	ofstream ofs_logPacketxTime;
	ofs_logPacketxTime.open(FILENAME_OUT_PACK);	
	ofs_logPacketxTime << "\nGrupos multicast:" << endl;
	ofs_logPacketxTime << "----------------------------------------" << endl;
	for (i = LayerList.begin(); i != LayerList.end(); ++i)
		ofs_logPacketxTime << "Layer "<<(*i).layerID<<": IP "<<(*i).IP<<", Speed "<<(*i).speed << endl;
	ofs_logPacketxTime << "----------------------------------------" << endl<<endl<<endl;
	ofs_logPacketxTime << "Pacotes ("<<ALMTF_PACKETSIZE<<" bytes) x Tempo (s)"<<endl;
	ofs_logPacketxTime << "----------------------------------------" << endl;
	long time = LogStep;
	for (int j = 0; j < LogTime/LogStep; j++) {
		ofs_logPacketxTime << time/1000.0 << "\t\t";
		for (i = LayerList.begin(); i != LayerList.end(); i++)
			ofs_logPacketxTime << (*i).packetsXtime[j] << "\t";
		time += LogStep;
		ofs_logPacketxTime << endl;
	}
	ofs_logPacketxTime.flush();
	ofs_logPacketxTime.close();

	// arquivo com a taxa de velocidade por segundo (kbits/s)
	ofstream ofs_logSpeedxTime;
	ofs_logSpeedxTime.open(FILENAME_OUT_SPEED);
	ofs_logSpeedxTime << "\nGrupos multicast:" << endl;
	ofs_logSpeedxTime << "----------------------------------------" << endl;
	for (i = LayerList.begin(); i != LayerList.end(); i++)
		ofs_logSpeedxTime << "Layer "<<(*i).layerID<<": IP "<<(*i).IP<<", Speed "<<(*i).speed << endl;
	ofs_logSpeedxTime << "----------------------------------------" << endl<<endl<<endl;
	ofs_logSpeedxTime << "Velocidade (bits/s) x Tempo (s)"<<endl;
	ofs_logSpeedxTime << "----------------------------------------" << endl;
	time = LogStep;
	for (int j = 0; j < LogTime/LogStep; j++) {
		ofs_logSpeedxTime << time/1000.0 << "\t\t";
		for (i = LayerList.begin(); i != LayerList.end(); i++)
			ofs_logSpeedxTime << (*i).speedXtime[j] << "\t";
		time += LogStep;
		ofs_logSpeedxTime << endl;
	}
	ofs_logSpeedxTime.flush();
	ofs_logSpeedxTime.close();
#endif
#ifdef _LOG_STATUS
	int totPackets=0;
	int totLoss=0;
	double percentLoss;
	ofs_logCoreStatus.open("LOG_CoreStatus.txt");
	ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
	ofs_logCoreStatus<<"|           Estatísticas ALMTF           |"<<endl;
	ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
	ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
	for (i = LayerList.begin(); i != LayerList.end(); i++) {
		ofs_logCoreStatus<<"  Layer "<<(*i).layerID<<endl;
		ofs_logCoreStatus<<"  Total de Pacotes Recebidos.: "<<(*i).num_packets<<endl;
		ofs_logCoreStatus<<"  Total de Pacotes Perdidos..: "<<(*i).nlost<<endl;
		percentLoss = 0.0;
		if ((*i).num_packets > 0)
			percentLoss = ((double)(*i).nlost/(double)(*i).num_packets)*100;
		ofs_logCoreStatus<<"  Percentual de Perdas.......: "<<percentLoss<<"%"<<endl;
		ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
		totPackets+=(*i).num_packets;
		totLoss+=(*i).nlost;
	}	
	ofs_logCoreStatus<<"|       Estatísticas ALMTF Globais       |"<<endl;
	ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
	ofs_logCoreStatus<<"  Total de Pacotes Recebidos.: "<<totPackets<<endl;
	ofs_logCoreStatus<<"  Total de Pacotes Perdidos..: "<<totLoss<<endl;
	percentLoss=0.0;
	if (totPackets > 0) 
		percentLoss = ((double)totLoss/(double)totPackets)*100;
	ofs_logCoreStatus<<"  Percentual de Perdas.......: "<<percentLoss<<"%"<<endl;
	ofs_logCoreStatus<<"|----------------------------------------|"<<endl;
	ofs_logCoreStatus.flush();
	ofs_logCoreStatus.close();
#endif
}

#endif

#ifdef _PACKETS_BUFFER
void *core_recvPacket(void *layerc)
{
	ALMTFRecLayer *layer = (ALMTFRecLayer *)layerc;
	ALMTFRecvdPacket *item;
	while (layer->thread_enabled) {
		if (!layer->PktList.empty()) {			
			item = layer->PktList.front();
			layer->PktList.pop_front();
			// chama a função do ALMTF para processar o pacote recebido
			ALMTF_recvPkt(layer, item->pkt, &(item->timepkt));
			free(item->pkt);
			free(item);
		}
		usleep(1000);
	}
	return(NULL);
}
#endif

void *core_recvThread(void *layerc)
{
	ALMTFRecLayer *layer = (ALMTFRecLayer *)layerc;
	layer->thread_running = true;	
	time_getus(&layer->start_time);
	stringstream sslist("");
	// cria a conexão multicast com o grupo
	sock_recv_MCastCon(&layer->sock, (char *)layer->IP, (unsigned int)layer->port, &(layer->sock_mreq));
	char *cbuff = new char[sizeof(transm_packet)];
	struct timeval tv_recvTime;
	// thread que pega os pacotes da lista e faz o processamento
#ifdef _PACKETS_BUFFER
	if (pthread_create(&(layer->ptProcThread), NULL, core_recvPacket, (void *)layer) != 0) {
		cerr << "Error Creating Thread: core_recvPacket!" << endl;	
	}		
#endif
	while (layer->thread_enabled) {
		sock_recvMsgRecv(&layer->sock, cbuff, sizeof(transm_packet), layer->port, &ServerIP);
		time_getus(&tv_recvTime);
		TimeLastPkt = tv_recvTime;
		if (layer->layerID == 0)
			TimeLastPktLayerZero = tv_recvTime;
		transm_packet *tpkt = new transm_packet[sizeof(transm_packet)]; 
		memmove(tpkt, cbuff, sizeof(transm_packet));
#ifndef _PACKETS_BUFFER
		layer->num_packets++;
		ALMTF_recvPkt(layer, tpkt, &tv_recvTime);
		delete(tpkt);
#else
		ALMTFRecvdPacket* item = new ALMTFRecvdPacket[sizeof(ALMTFRecvdPacket)];
		item->pkt = tpkt;
		item->timepkt = tv_recvTime;
		layer->PktList.push_back(item);
		layer->num_packets++;
#endif
	}
	sock_recv_MCastLeaveGroup(&layer->sock, &(layer->sock_mreq));
	sock_close(&layer->sock);
	delete(cbuff);
	// informa que a camada não está mais em execução
	layer->thread_running = false;
	sslist.str()="";
	sslist << "Layer "<<layer->layerID<<": [encerrou execucao]"<<endl;
	recv_printlist.push_back(sslist.str());
	return (NULL);
}

void core_sendPacket(transm_packet *pkt)
{	
	int socketDescriptor;	
	if ((socketDescriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		errno_error(errno);
	}
	sock_sendMsg(&socketDescriptor, (char *)pkt, sizeof(transm_packet), ServerIP, ServerPort);
	sock_close(&socketDescriptor);
}

int core_getLogTime()
{
	return LogTime;
}

void core_setLogTime(int value)
{
	LogTime = value;
}

int core_getLogStep()
{
	return LogStep;
}

void core_setLogStep(int value)
{
	LogStep = value;
}

int core_getRatesCum(int layer)
{
	return RatesCum[layer];
}

int core_getRatesCumCurrent()
{	
	return RatesCum[CurrentLayer];
}

int core_getCurrentLayer()
{
	return CurrentLayer;
}

int core_getTotalLayers()
{
	return TotalLayers;
}

void core_getLastPktTime(struct timeval *t)
{
	t->tv_sec = TimeLastPkt.tv_sec;
	t->tv_usec = TimeLastPkt.tv_usec;
}

void core_getLastPktTimeLayerZero(struct timeval *t)
{
	t->tv_sec = TimeLastPktLayerZero.tv_sec;
	t->tv_usec = TimeLastPktLayerZero.tv_usec;
}

void core_getLastTimestamp(struct timeval *t)
{
	t->tv_sec = TimeLastTimestamp.tv_sec;
	t->tv_usec = TimeLastTimestamp.tv_usec;
}

void *core_printThread(void *lc)
{
	list<string> *l = (list<string> *)lc;
	while(1) {
		while (!l->empty()) 
		{			
			cout << l->front() << endl;
			l->pop_front();
		}
		usleep(5000);
	}
	return(NULL);
}

void core_sendPrintList(string s)
{
	recv_printlist.push_back(s);
}

#ifdef GRAPH_JAVAINTERFACE
void GRAPH_frame(GRAPHInfo num, int* n, char* res) 
{
    int s = 0;
	int i;
	memset(res,0,48*sizeof(char));
	char *tm = new char[8*sizeof(char)];
	s = sprintf(tm, "%d", num.Bwatual);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);	
	s = sprintf(tm, "%d", (int)num.Bweq);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);	
	s = sprintf(tm, "%d", (int)num.Bwmax);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);
	s = sprintf(tm, "%d", (int)num.Bwjanela);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);
	s = sprintf(tm, "%d", (int)num.RTT);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);
	s = sprintf(tm, "%d", (int)num.Numloss);
	for(i=0;i<(8-s);i++)
		strcat(res,"0");
	strcat(res,tm);
    *n = 48*sizeof(char);
	delete(tm);
}

void *GRAPH_sendMsg(void *)
{
	int sock, portn;
	int length, r, size;
	char* buffer;
	struct hostent *hp;	
	struct sockaddr_in server;
	memset((char *)&server, 0, sizeof(server));
	portn = 5112;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) cout<<"Error Opening Socket!\n";
	char IP[100];
	gethostname(IP,sizeof(IP));	
	hp = gethostbyname(IP);
	if (hp==0) cout<<"Unknown IP!\n";			   
	server.sin_family = AF_INET;
	memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
	server.sin_port=htons(portn);
	length = sizeof(struct sockaddr_in);	
	GRAPHInfo info;	
	buffer = new char[48*sizeof(char)];
	while (1)
	{	  
		GRAPH_getInfo(&info);		
		GRAPH_frame(info, &size, buffer);	
		r = sendto(sock, buffer, size, 0, (struct sockaddr *)&server, length);
		usleep(50000);
	}
	delete(buffer);
	return(NULL);
}
#endif

