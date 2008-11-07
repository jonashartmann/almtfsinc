
#include "Socket_Utils.h"

/*---------- FUNÇÕES SENDER ----------*/

#ifdef _WIN32_
bool sock_sender_MCastCon(SOCKET *sock, char *IP, unsigned int port)
#else
bool sock_sender_MCastCon(int *sock, char *IP, unsigned int port)
#endif
{
	//const int flag_on = 1;
	const unsigned char nIP_TTL = 127;
	struct sockaddr_in sa_in_Server;	
	//struct in_addr MCastAddr;
	/* AF_INET = Address Family, Internet = IP Addresses
	 * PF_INET = Packet Format, Internet = IP, TCP/IP or UDP/IP
	 * SOCK_DGRAM style is used for sending individually-addressed packets, unreliably. It is the diametrical opposite of SOCK_STREAM.
	 * SOCK_DGRAM = UDP Datagram
	 * IPPROTO_UDP = Protocolo UDP */
	if ((*sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		errno_error(errno);
		return false;
	}
	/* SOL_SOCKET = Options to be accessed at socket level, not protocol level. 
	 * SO_REUSEADDR = Reuse of local addresses is supported. */
/*	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on)) < 0) {
		errno_error(errno);
		return false;
	}
*/
    /* IP_MULTICAST_TTL = Opção para setar o TTL do socket */ 
	if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&nIP_TTL, sizeof(nIP_TTL)) < 0) {
		errno_error(errno);
		return false;
	}	
/*	// cria o endereço para multicast e seta como opção do socket	
	memset(&MCastAddr, 0, sizeof(MCastAddr));
	MCastAddr.s_addr = htonl(INADDR_ANY);	
	if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&MCastAddr, sizeof(struct in_addr )) < 0) {
		errno_error(errno);
		return false;
	}
*/	
	// liga o socket address do servidor ao socket criado
	memset(&sa_in_Server, 0, sizeof(sa_in_Server));
	sa_in_Server.sin_family = AF_INET; 
	sa_in_Server.sin_addr.s_addr = inet_addr(IP);
	sa_in_Server.sin_port = htons(port);
/*	if (bind (*sock, (struct sockaddr *)&sa_in_Server, sizeof(struct sockaddr_in)) < 0) {
		errno_error(errno);
		return false;
	}
*/
	return true;
}

/*---------- FUNÇÕES RECEIVER ----------*/

#ifdef _WIN32_
bool sock_recv_MCastCon(SOCKET *sock, char *IP, unsigned int port, ip_mreq *req)
#else
bool sock_recv_MCastCon(int *sock, char *IP, unsigned int port, ip_mreq *req)
#endif
{
	const int flag_on = 1;
	struct sockaddr_in serverAddress;
	/* AF_INET = Address Family, Internet = IP Addresses
	 * PF_INET = Packet Format, Internet = IP, TCP/IP or UDP/IP
	 * SOCK_DGRAM style is used for sending individually-addressed packets, unreliably. It is the diametrical opposite of SOCK_STREAM.
	 * SOCK_DGRAM = UDP Datagram
	 * IPPROTO_UDP = Protocolo UDP */
	if ((*sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		errno_error(errno);
		return false;
	}
	/* SOL_SOCKET = Options to be accessed at socket level, not protocol level. 
	 * SO_REUSEADDR = Reuse of local addresses is supported. */
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on)) < 0) {
		errno_error(errno);
		return false;
	}
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	/* IP Multicast do Sender para receber os pacotes apenas do IP específico da camada. */ 
	/* Se setado para htonl(INADDR_ANY) receberá pacotes de todas as camadas. */
	serverAddress.sin_addr.s_addr = inet_addr(IP);
	/* Porta utilizada para a conexão multicast, a mesma porta recebe todas as conexões. */
	serverAddress.sin_port = htons(port);
	if (bind(*sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		errno_error(errno);
		return false;
	}
	/* construct an IGMP join request structure */
	req->imr_multiaddr.s_addr = inet_addr(IP);		/* group addr */ 
	req->imr_interface.s_addr = htonl(INADDR_ANY);	/* use default */
	/* send an ADD MEMBERSHIP message via setsockopt
	 * SOL_SOCKET is for the socket layer, IPPROTO_IP for the IP layer. For multicast programming, level will always be IPPROTO_IP. */
	if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)req, sizeof(struct ip_mreq)) < 0) {
		errno_error(errno);
		cerr << "IP: " << IP << " PORT: " << port << endl << endl;
		cerr << "Error in IP_ADD_MEMBERSHIP" << endl;
		return false;
	}
	return true;
}

#ifdef _WIN32_
void sock_recv_MCastLeaveGroup(SOCKET *sock, ip_mreq *req)
#else
void sock_recv_MCastLeaveGroup(int *sock, ip_mreq *req)
#endif
{
	setsockopt(*sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)req, sizeof(struct ip_mreq));
}

/*---------- FUNÇÕES COMUNS ----------*/

#ifdef _WIN32_
bool sock_sendMsg(SOCKET *sock, char *buff, int sbuff, char *IP, int port)
#else
bool sock_sendMsg(int *sock, char *buff, int sbuff, char *IP, int port)
#endif
{
	struct sockaddr_in mc_addr;
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family = AF_INET;	
	mc_addr.sin_addr.s_addr = inet_addr(IP);
	mc_addr.sin_port = htons(port);
	int bytes = 0;
	bytes = sendto(*sock, buff, sbuff, 0, (struct sockaddr *)&mc_addr, sizeof(mc_addr));
	if (bytes < 0) {
		errno_error(errno);
		return false;
	}
	return true;
}

#ifdef _WIN32_
bool sock_recvMsg(SOCKET *sock, char *buff, int sbuff, int port)
#else
int sock_recvMsgServ(int *sock, char *buff, int sbuff, int port)
#endif
{
	int mc_addr_size;
	struct sockaddr_in mc_addr;
	mc_addr_size = sizeof(mc_addr);
	memset(&mc_addr, 0, mc_addr_size);
	int bytes;
	bytes = recvfrom(*sock, buff, sbuff, 0, (struct sockaddr *)&mc_addr, (socklen_t *)&mc_addr_size);
	if (bytes <= 0) {
		errno_error(errno);
	}
	return bytes;
}

#ifdef _WIN32_
bool sock_recvMsg(SOCKET *sock, char *buff, int sbuff, int port, char **senderIP)
#else
int sock_recvMsgRecv(int *sock, char *buff, int sbuff, int port, char **senderIP)
#endif
{
	int mc_addr_size;
	struct sockaddr_in mc_addr;
	mc_addr_size = sizeof(mc_addr);
	memset(&mc_addr, 0, mc_addr_size);
	int bytes;
	bytes = recvfrom(*sock, buff, sbuff, 0, (struct sockaddr *)&mc_addr, (socklen_t *)&mc_addr_size);
	if (bytes <= 0) {
		errno_error(errno);
	}	
	if (*senderIP == NULL) {
		*senderIP = new char[16];	
		strcpy(*senderIP, inet_ntoa(mc_addr.sin_addr));
	}
	return bytes;
}

#ifdef _WIN32_
bool sock_listenCon(SOCKET *sock, int port)
#else
bool sock_listenCon(int *sock, int port)
#endif
{
	struct sockaddr_in serverAddress;
	//Cria o socket para esperar as conexões dos receptores
	*sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (*sock < 0) {
		errno_error(errno);
		return false;
	}
	//Liga o socket à porta de conexão
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if (bind(*sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		errno_error(errno);
		return false;
	}
	return true;
}

#ifdef _WIN32_
void sock_close(SOCKET *sock)
#else
void sock_close(int *sock)
#endif
{
	close(*sock);
}

#ifdef _WIN32_

void handle_error()
{
    switch (WSAGetLastError())
    {
		case WSANOTINITIALISED:
			cerr << "Unable to initialise socket.\n";
			break;
		case WSAEAFNOSUPPORT:
			cerr << "The specified address family is not supported.\n";
			break;
		case WSAEADDRNOTAVAIL:
			cerr << "Specified address is not available from the local machine.\n";
			break;
		case WSAECONNREFUSED:
			cerr << "The attempt to connect was forcefully rejected.\n";
			break;
		case WSAEDESTADDRREQ:
			cerr << "Address destination is required.\n";
			break;
		case WSAEFAULT:
			cerr << "The namelen argument is incorrect.\n";
			break;
		case WSAEINVAL:
			cerr << "The socket is not already bound to an address.\n";
			break;
		case WSAEISCONN:
			cerr << "The socket is already connected.\n";
			break;
		case WSAEADDRINUSE:
			cerr << "The specified address is already in use.\n";
			break;
		case WSAEMFILE:
			cerr << "No more file descriptors are available.\n";
			break;
		case WSAENOBUFS:
			cerr << "No buffer space available. The socket cannot be created.\n";
			break;
		case WSAEPROTONOSUPPORT:
			cerr << "The specified protocol is not supported.\n";
			break;
		case WSAEPROTOTYPE:
			cerr << "The specified protocol is the wrong type for this socket.\n";
			break;
		case WSAENETUNREACH:
			cerr << "The network can't be reached from this host at this time.\n";
			break;
		case WSAENOTSOCK:
			cerr << "The descriptor is not a socket.\n";
			break;
		case WSAETIMEDOUT:
			cerr << "Attempt timed out without establishing a connection.\n";
			break;
		case WSAESOCKTNOSUPPORT:
			cerr << "Socket type is not supported in this address family.\n";
			break;
		case WSAENETDOWN:
			cerr << "Network subsystem failure.\n";
			break;
		case WSAHOST_NOT_FOUND:
			cerr << "Authoritative Answer Host not found.\n";
			break;
		case WSATRY_AGAIN:
			cerr << "Non-Authoritative Host not found or SERVERFAIL.\n";
			break;
		case WSANO_RECOVERY:
			cerr << "Non recoverable errors, FORMERR, REFUSED, NOTIMP.\n";
			break;
		case WSANO_DATA:
			cerr << "Valid name, no data record of requested type.\n";
			break;
        	case WSAEINPROGRESS:
			cerr << "Address blocking Windows Sockets operation is in progress.\n";
			break;
		case WSAEINTR:
			cerr << "The (blocking) call was canceled via WSACancelBlockingCall().\n";
			break;
		default :
			cerr << "Unknown error.\n";
			break;
	}
	WSACleanup();
}

#else

void errno_error(int error)
{
	switch(error)
	{
		case EPROTONOSUPPORT:
			cerr <<"\tThe protocol is not supported by the address family, or the protocol is not supported by the implementation.\n";
			break;
		case EAFNOSUPPORT:
			cerr << "\tThe specified address is not a valid address for the address family of the specified socket.\n";
			break;
		case ENFILE:
			cerr << "\tNo more file descriptors are available for the system.\n";
			break;
		case EMFILE:
			cerr << "\tNo more file descriptors are available for this process.\n";
			break;
		case EPROTOTYPE:
			cerr << "\tThe socket type is not supported by the protocol.\n";
			break;
		case EACCES:
			cerr << "\tThe process does not have appropriate privileges.\n"; 
			break;
		case EBADF:
			cerr << "\tThe socket argument is not a valid file descriptor.\n";
			break;
		case EDOM:
			cerr << "\tThe send and receive timeout values are too big to fit into the timeout fields in the socket structure.\n";
			break;
		case EINVAL:
			cerr << "\tThe socket is already bound to an address, and the protocol does not support binding to a new address; or the socket has been shut down.\n";
			cerr << "\tRecvfrom(): The MSG_OOB flag is set and no out-of-band data is available.\n";
		    cerr << "\tSendto(): The dest_len argument is not a valid length for the address family.\n";
			break;
		case EISCONN:
			cerr << "\tThe socket is already connected, and a specified option cannot be set while the socket is connected.\n";
			break;
		case ENOPROTOOPT:
			cerr << "\tThe option is not supported by the protocol.\n";
			break;
		case ENOTSOCK:
			cerr << "\tThe socket argument does not refer to a socket.\n";
			break;
		case ENOMEM:
			cerr << "\tThere was insufficient memory available for the operation to complete.\n";
			break;
		case ENOBUFS:
			cerr << "\tInsufficient resources are available in the system to complete the call.\n";
			break;			    
		case EFAULT:
			cerr << "\tThe address pointed to by optval is not in a valid part of the process address space.\n";
			break;
		case EADDRINUSE:
			cerr << "\tThe specified address is already in use.\n";
			break;
		case EADDRNOTAVAIL:
			 cerr << "\tThe specified address is not available from the local machine.\n";
			 break;
		case EOPNOTSUPP:
			 cerr << "\tThe socket type of the specified socket does not support binding to an address.\n";
			 cerr << "\tThe socket argument is associated with a socket that does not support one or more of the values set in flags.\n";
			 break;
		case EWOULDBLOCK or EAGAIN:
			cerr << "\tThe socket's file descriptor is marked O_NONBLOCK and no data is waiting to be received; or MSG_OOB is set and no out-of-band data is available and either the socket's file descriptor is marked O_NONBLOCK or the socket does not support blocking to await out-of-band data.\n";
			break;
		case ECONNRESET:
			cerr << "\tA connection was forcibly closed by a peer.\n";
			break;
		case EINTR:
			cerr << "\tA signal interrupted recvfrom() before any data was available.\n";
			cerr << "\tA signal interrupted sendto() before any data was transmitted.\n";
			break;
		case ENOTCONN:
			cerr << "\tA receive is attempted on a connection-mode socket that is not connected.\n";
			break;
		case ETIMEDOUT:
			cerr << "\tThe connection timed out during connection establishment, or due to a transmission timeout on active connection.\n";
			break;
		case EIO:
			cerr << "\tAn I/O error occurred while reading from or writing to the file system.\n";
			break;
		case EMSGSIZE:
			cerr << "\tThe message is too large to be sent all at once, as the socket requires.\n";
			break;
		case EPIPE:
			cerr << "\tThe socket is shut down for writing, or the socket is connection-mode and is no longer connected. In the latter case, and if the socket is of type SOCK_STREAM, the SIGPIPE signal is generated to the calling thread.\n"; 
			break;
		case EDESTADDRREQ:
			 cerr << "\tThe socket is not connection-mode and does not have its peer address set, and no destination address was specified.\n";
			 break;
		case EHOSTUNREACH:
			cerr << "\tThe destination host cannot be reached (probably because the host is down or a remote router cannot reach it).\n";
			break;
		case ENETDOWN:
			cerr << "\tThe local network interface used to reach the destination is down.\n";
			break;
		case ENETUNREACH:
			cerr << "\tNo route to the network is present.\n";
			break;
		default:
			cerr << "\tUnknown error.\n";
			break;	
	}
}

#endif
