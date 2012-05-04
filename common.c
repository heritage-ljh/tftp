#include "common.h"

int create_socket(int domain, int type, int protocol)
{
	int sID = -1;
	sID = socket(domain, type, protocol);

	return(sID);
}

int make_bind(int sockID, short int family, unsigned short int port, char *host)
{
	int ok = -1;
	struct sockaddr_in sockInfo;
	bzero(&sockInfo, sizeof(struct sockaddr_in));

	sockInfo.sin_family	= family;
	sockInfo.sin_port	= htons(port);

	if(host == NULL)
	{
		sockInfo.sin_addr.s_addr	= INADDR_ANY;
	}
	else
	{
		sockInfo.sin_addr	= *(struct in_addr *)gethostbyname(host)->h_addr;
	}

	ok = bind(sockID, (struct sockaddr *) &sockInfo, sizeof(struct sockaddr_in));
	return(ok);
}

//====================

void help(struct PARAMS *params)
{
	printf("[-p] port \t -- port number\n");
	printf("[-v] \t\t -- verbose mode (shows a log on screen)\n");
	printf("[-h] \t\t -- shows the help\n");

	if(params->sc == TRIVIAL)
	{
		printf("[-r|-w] \t --read or write\n");
		printf("[-t] rexmt \t -- retrasmission time\n");
		printf("[-H] host \t -- name of host server\n");
		printf("[-f] file \t -- name of the file\n");
	}
}

void param_parser(int argc, char *argv[], struct PARAMS *params)
{
	int i = 0;
	for(i = 0; i < argc; i++)
	{
		if(!strcmp("-p", argv[i]))
		{
			params->port = atoi(argv[++i]);
		}

		if(!strcmp("-v", argv[i]))
		{
			params->verbose = 1;
		}

		if(!strcmp("-h", argv[i]))
		{
			help(params);
			exit(EXIT_SUCCESS);
		}

		if(params->sc == TRIVIAL)
		{
			if(!strcmp("-r", argv[i]))
			{
				params->read_write = RFC1350_OP_RRQ;
			}

			if(!strcmp("-w", argv[i]))
			{
				params->read_write = RFC1350_OP_WRQ;
			}

			if(!strcmp("-f", argv[i]))
			{
				strcpy(params->file, argv[++i]);
			}

			if(!strcmp("-H", argv[i]))
			{
				strcpy(params->host, argv[++i]);
			}

			if(!strcmp("-t", argv[i]))
			{
				params->rexmt = atoi(argv[++i]);
			}
		}
	}
}

//====================

int rwq_serialize(tftp_rwq_hdr *rwq, char *buffer)
{
	
	int i = 0;
	bzero(buffer, MAX_BUFFER);

	stshort(rwq->opcode, buffer);
	int sz = sizeof(unsigned short int);

	for(i = 0; rwq->filename[i] != '\0' && i < MAXPATH_STRLEN; i++, sz++)
	{
		  *(buffer+sz) = rwq->filename[i];
	}

	for(i = 0; rwq->mode[i] != '\0' && i < MAXMODE_STRLEN; i++, sz++)
	{
		  *(buffer+sz+1) = rwq->mode[i];
	}

	return(++sz);
}

void rwq_deserialize(tftp_rwq_hdr *rwq, char *buffer)
{
	int i = 0;
	bzero(rwq, sizeof(tftp_rwq_hdr));
	rwq->opcode = ldshort(buffer);

	for(i = 0, buffer += sizeof(unsigned short int); *buffer != '\0' && i < MAXPATH_STRLEN; i++, buffer++)
	{
		rwq->filename[i] = *buffer;
	}

	for(i = 0, buffer += 1; *buffer != '\0' && i < MAXMODE_STRLEN; i++, buffer++)
	{
		rwq->mode[i] = *buffer;
	}

	for(i = 0, buffer += 1; *buffer != '\0' && i < 8; i++, buffer++)
	{
		rwq->tout[i] = *buffer;
	}

	for(i = 0, buffer += 1; *buffer != '\0' && i < 4; i++, buffer++)
	{
		rwq->toutv[i] = *buffer;
	}
}

int ack_serialize(tftp_ack_hdr *ack, char *buffer)
{
	int sz = 0;
	bzero(buffer, MAX_BUFFER);

	stshort(ack->opcode, buffer + sz);
	sz += SHORT_SIZE;

	stshort(ack->num_block, buffer + sz);
	sz += SHORT_SIZE;

	return(sz);
}

void ack_deserialize(tftp_ack_hdr *ack, char *buffer)
{
	int sz = 0;
	bzero(ack, sizeof(tftp_ack_hdr));

	ack->opcode = ldshort(buffer + sz);
	sz += SHORT_SIZE;
	
	ack->num_block = ldshort(buffer + sz);
	sz += SHORT_SIZE;
}

int data_serialize(tftp_data_hdr *data, char *buffer, int dsz)
{
	int sz = 0;
	bzero(buffer, MAX_BUFFER);
		
	stshort(data->opcode, buffer + sz);
	sz += SHORT_SIZE;
	
	stshort(data->num_block, buffer + sz);
	sz += SHORT_SIZE;
	
	int i = 0;
	for(i = 0; i < dsz; i++, sz++)
	{
		*(buffer + sz) = data->data[i];
	}	
	
	return(sz);
}

void data_deserialize(tftp_data_hdr *data, char *buffer, int dsz)
{
	int sz = 0;
	bzero(data, sizeof(tftp_data_hdr));

	data->opcode = ldshort(buffer+sz);
	sz += SHORT_SIZE;

	data->num_block = ldshort(buffer+sz);
	sz += SHORT_SIZE;

	int i = 0;
	for(i = 0; i < (dsz - SHORT_SIZE * 2); i++, sz++)
	{
		data->data[i] = *(buffer+sz);
	}
}

int error_serialize(tftp_error_hdr *error, char *buffer)
{
	int sz = 0;
	bzero(buffer, MAX_BUFFER);
	
	stshort(error->opcode, buffer + sz);
	sz += SHORT_SIZE;
	
	stshort(error->code, buffer + sz);
	sz += SHORT_SIZE;

	int i = 0;
	for(i = 0; i < MAX_ERROR_STRLEN && error->message[i] != '\0'; i++, sz++)
	{
		*(buffer + sz) = error->message[i];
	}
	
	return(++sz);
}

void error_deserialize(tftp_error_hdr *error, char *buffer, int dsz)
{
	int sz = 0;
	bzero(error, sizeof(tftp_error_hdr));
	
	error->opcode = ldshort(buffer + sz);
	sz += SHORT_SIZE;
	
	error->code = ldshort(buffer + sz);
	sz += SHORT_SIZE;
	
	int i = 0;
	for(i = 0; i < (dsz - SHORT_SIZE * 2); i++, sz++)
	{
		error->message[i] = *(buffer + sz);
	}
}

//================

int sendACKOpt(int sockID, struct sockaddr_in sockInfo, char *opt, int value)
{
	int sz = 0;
	char buffer[MAX_BUFFER];
	char vchar[4];
	
	bzero(buffer, MAX_BUFFER);
	bzero(vchar, 4);
	
	stshort(6, buffer + sz);
	sz += SHORT_SIZE;
	
	int i = 0;
	for(i = 0; *(opt+i) != '\0'; sz++, i++)
	{
		*(buffer+sz+1) = *(opt+i);
	}
	
	sprintf(vchar, "%d", value);
	for(i = 0; *(vchar+i) != '\0'; sz++, i++)
	{
		*(buffer+sz+1) = *(vchar+i);
	}
	
	return(++sz);
}

int sendACK(int sockID, struct sockaddr_in sockInfo, unsigned short int ack)
{
	char buffer[MAX_BUFFER];
	tftp_ack_hdr aCk;

	bzero(buffer, MAX_BUFFER);
	bzero(&aCk, sizeof(tftp_ack_hdr));


	aCk.opcode = RFC1350_OP_ACK;
	aCk.num_block = ack;

	int sz = ack_serialize(&aCk, buffer);
	int ok = sendto(sockID, (char *) buffer, sz, 0, (struct sockaddr *) &sockInfo, sizeof(struct sockaddr_in));

	return(ok);
}

int sendInfo(int sockID, struct sockaddr_in sockInfo, char *buffer, int sz)
{
	return(sendto(sockID, (char *) buffer, sz, 0, (struct sockaddr *) &sockInfo, sizeof(struct sockaddr_in)));
}

int	sendError(int sockID, struct sockaddr_in sockInfo, unsigned short int opcode, unsigned short int errcode, char *errtext)
{
	tftp_error_hdr error;
	char buffer[MAX_BUFFER];

	bzero(&error, sizeof(tftp_error_hdr));
	bzero(buffer, MAX_BUFFER);
			
	error.opcode = opcode;
	error.code   = errcode;
	strncpy(error.message, errtext, MAX_ERROR_STRLEN);
			
	int sz = error_serialize(&error, buffer);
	return(sendInfo(sockID, sockInfo, buffer, sz));
}

//================

int timeout_option(tftp_rwq_hdr *rwq, struct PARAMS *params)
{
	int ok = -1;

	if(!strcmp(rwq->tout, "timeout") && atoi(rwq->toutv) > 0 && atoi(rwq->toutv) < 256)
	{
		ok = 1;
		params->rexmt = atoi(rwq->toutv);

		if(params->verbose)
		{
			printf("New timeout value: %d s\n", params->rexmt);
		}
	}

	return(ok);
}

int select_func(int sockID, int time)
{
	fd_set fds;
	struct timeval t_retx;
	bzero(&t_retx, sizeof(struct timeval));

	FD_ZERO(&fds);
	FD_SET(sockID, &fds);
	t_retx.tv_sec = time;

	int s = select(sockID + 1, &fds , NULL , NULL , &t_retx);
	return(s);
}

int mode_transfer(char *modeFound, char *modeCorrect, struct PARAMS *params)
{
	int ok = 1;

	if(strcmp(modeFound, modeCorrect))
	{
		if(params->verbose)
		{
			printf("ERROR Client transfer mode is set to: %s\n", modeFound);
		}

		ok = -1;
	}

	return(ok);
}

FILE *open_file(char *fName, char *mode, struct  PARAMS *params)
{
	FILE *pFile = NULL;
	
	if((pFile = fopen(fName, mode)) == NULL)
	{
		if(params->verbose)
		{
			printf("Error opening file: %s\n", fName);
		}
	}

	return(pFile);
}

int file_exists(char *fName)
{
	FILE *pFile = NULL;
	int OK = -1;

	if((pFile = fopen(fName, "r")) != NULL)
	{
		fclose(pFile);
		OK = 1;
	}

	return(OK);
}

int get_data(tftp_data_hdr *data, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params)
{
	int sz = 0;
	char buffer[MAX_BUFFER];
	socklen_t socklen = sizeof(struct sockaddr_in);

	bzero(buffer, MAX_BUFFER);
	bzero(data, sizeof(tftp_data_hdr));

	sz = recvfrom(sockID, (char *) buffer, MAX_BUFFER, 0, (struct sockaddr *) &sockInfo, &socklen);
	data_deserialize(data, buffer, sz);

	if(params->verbose)
	{
		printf("C\t %s: Data [%d#] [%lu]\n", inet_ntoa(sockInfo.sin_addr), data->num_block, sz - SHORT_SIZE * 2);
		printf("S\t ACK [%d#]\n", data->num_block);
	}

	return(sz - SHORT_SIZE * 2);
}

int get_ack(tftp_ack_hdr *ack, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params)
{
	int sz = 0;
	char buffer[MAX_BUFFER];
	socklen_t socklen = sizeof(struct sockaddr_in);

	bzero(buffer, MAX_BUFFER);
	bzero(ack, sizeof(tftp_ack_hdr));

	sz = recvfrom(sockID, (char *) buffer, MAX_BUFFER, 0, (struct sockaddr *) &sockInfo, &socklen);
	ack_deserialize(ack, buffer);

	if(params->verbose)
	{
		printf("S\t ACK [%d#]\n", ack->num_block);
	}

	return(sz);
}
