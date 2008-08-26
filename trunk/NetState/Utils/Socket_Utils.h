#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#ifdef _WIN32_

#include <winsock.h>

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
//getsockopt() e setsockopt()
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#endif

#include <iostream>
using namespace std;

/////////////// SENDER ///////////////

/*
 * Cria uma conexão no socket 'sock' para enviar os dados por multicast para
 * o IP multicast 'IP' na porta 'port'.
 */
#ifdef _WIN32_

bool sock_sender_MCastCon(SOCKET *sock, char *IP, unsigned int port);

#else

bool sock_sender_MCastCon(int *sock, char *IP, unsigned int port);

#endif

/////////////// RECEIVER ///////////////

/*
 * Cria uma conexão no socket 'sock' para receber os dados que são enviados para
 * o grupo multicast de IP 'IP' e na porta 'port'.
 */
#ifdef _WIN32_

bool sock_recv_MCastCon(SOCKET *sock, char *IP, int port, ip_mreq *req);

#else

bool sock_recv_MCastCon(int *sock, char *IP, unsigned int port, ip_mreq *req);

#endif

/*
 * Dá um leave no grupo multicast representado por 'req' no socket 'sock'.
 */
#ifdef _WIN32_

void sock_recv_MCastLeaveGroup(SOCKET *sock, ip_mreq *req);

#else

void sock_recv_MCastLeaveGroup(int *sock, ip_mreq *req);

#endif

/////////////// FUNÇÕES COMUNS ///////////////

/*
 * Envia uma mensagem usando o socket 'sock' para o ip 'IP' usando a porta 'port'
 */
#ifdef _WIN32_

bool sock_sendMsg(SOCKET *sock, char *buff, int sbuff, char *IP, int port);

#else

bool sock_sendMsg(int *sock, char *buff, int sbuff, char *IP, int port);

#endif

/*
 * Espera por alguma mensagem usando o socket 'sock' e escutando a porta 'port'.
 */
#ifdef _WIN32_

bool sock_recvMsgRecv(SOCKET *sock, char *buff, int sbuff, int port, char **senderIP);

bool sock_recvMsgServ(SOCKET *sock, char *buff, int sbuff, int port);

#else

int sock_recvMsgRecv(int *sock, char *buff, int sbuff, int port, char **senderIP);

int sock_recvMsgServ(int *sock, char *buff, int sbuff, int port);

#endif

/*
 * Finaliza o socket 'sock'
 */
#ifdef _WIN32_

void sock_close(SOCKET *sock);

#else

void sock_close(int *sock);

#endif

/*
 * Cria uma conexão no socket 'sock' para escutar a porta 'port'.
 */
#ifdef _WIN32_

bool sock_listenCon(SOCKET *sock, int port);

#else

bool sock_listenCon(int *sock, int port);

#endif

#ifdef _WIN32_

void handle_error();

#else

void errno_error(int error);

#endif

#endif /*SOCKET_UTILS_H_*/
