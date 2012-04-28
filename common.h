#ifndef COMMON_H
	#define COMMON_H

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <errno.h>

 	#include <sys/types.h>
    	#include <sys/time.h>
    	#include <sys/resource.h>
    	#include <sys/wait.h>

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <netdb.h>


	#include "tftp.h"

	
	#define stshort(sval, addr)	(*((unsigned short int *)(addr)) = htons(sval))
	#define ldshort(addr) 		(ntohs (*((unsigned short int *)(addr))))


	#define SHORT_SIZE		sizeof(unsigned short int)
	#define MAX_BUFFER		1024

	#define PARAMS_TEXT_SIZE	50
	#define CMD_READ 		1
	#define CMD_WRITE		2

	#define	TRIVIALD		1
	#define	TRIVIAL			2

	struct PARAMS
	{
		int	sc;
		int	port;
		int	verbose;
		int	rexmt;
		int 	read_write;
		char	host[PARAMS_TEXT_SIZE];
		char	file[PARAMS_TEXT_SIZE];
	};

	//====================

	int		create_socket(int domain, int type, int protocol);
	int 	make_bind(int sockID, short int family, unsigned short int port, char *host);

	//====================

	void 	help(struct PARAMS *params);
	void 	param_parser(int argc, char *argv[], struct PARAMS *params);

	//====================

	int 	rwq_serialize(tftp_rwq_hdr *rwq, char *buffer);
	void 	rwq_deserialize(tftp_rwq_hdr *rwq, char *buffer);

	int 	ack_serialize(tftp_ack_hdr *ack, char *buffer);
	void 	ack_deserialize(tftp_ack_hdr *ack, char *buffer);

	int 	data_serialize(tftp_data_hdr *data, char *buffer, int dsz);
	void 	data_deserialize(tftp_data_hdr *data, char *buffer, int dsz);
	
	int		error_serialize(tftp_error_hdr *error, char *buffer);
	void	error_deserialize(tftp_error_hdr *error, char *buffer, int dsz);

	//====================

	int		sendACKOpt(int sockID, struct sockaddr_in sockInfo, char *opt, int value);
	int 	sendInfo(int sockID, struct sockaddr_in sockInfo, char *buffer, int sz);
	int 	sendACK(int sockID, struct sockaddr_in sockInfo, unsigned short int ack);
	int		sendError(int sockID, struct sockaddr_in sockInfo, unsigned short int opcode, unsigned short int errcode, char *errtext);

	void 	wrqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params);
	void 	rwqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params);

	//====================

	int 	mode_transfer(char *modeFound, char *modeCorrect, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params);
	int		get_data(tftp_data_hdr *data, int sockID, struct sockaddr_in sockInfo, struct PARAMS *param);
	int 	get_ack(tftp_ack_hdr *ack, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params);
	int 	select_func(int sockID, int time);

	FILE 	*open_file(char *fName, char *mode, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params);

#endif
