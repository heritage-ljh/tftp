#include "common.h"

int	create_socket(int domain, int type, int protocol)
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
				params->read_write = CMD_READ;
			}

			if(!strcmp("-w", argv[i]))
			{
				params->read_write = CMD_WRITE;
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

	stshort(ack->opcode, buffer);
	sz += sizeof(unsigned short int);

	stshort(ack->num_block, buffer+sz);
	sz += sizeof(unsigned short int);

	return(sz);
}

void ack_deserialize(tftp_ack_hdr *ack, char *buffer)
{
	int sz = 0;
	bzero(ack, sizeof(tftp_ack_hdr));

	ack->opcode = ldshort(buffer + sz);
	sz += sizeof(unsigned short int);
	
	ack->num_block = ldshort(buffer + sz);
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

void wrqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params)
{
	tftp_rwq_hdr rwq;
	tftp_data_hdr data;
	
	rwq_deserialize(&rwq, buffer);
	if(timeout_option(&rwq, params, sockID, sockInfo) == 0)
	{
		sendACK(sockID, sockInfo, 0);
	}

	if(mode_transfer(rwq.mode, "octet", sockID, sockInfo, params) == -1)
	{
		return;
	}

	FILE *pFile = NULL;
	if((pFile = open_file(rwq.filename, "wb", sockID, sockInfo, params)) == NULL)
	{
		return;
	}

	int sz = 0;
	int t_out = 0;
	int intents = 5;

	if(params->verbose)
	{
		printf("C write\t  %s:  %s\n", inet_ntoa(sockInfo.sin_addr), rwq.filename);
	}
	
	do
	{
		sz = 0;
		intents = 5;

		do
		{
			intents--;
			if((t_out = select_func(sockID, params->rexmt)) == 1)
			{
				sz = get_data(&data, sockID, sockInfo, params);
				sendACK(sockID, sockInfo, data.num_block);
				fwrite(data.data, sz, 1, pFile);
			}
		}while(t_out == 0 && intents);
	}while(sz == RFC1350_BLOCKSIZE && t_out != -1);

	if(intents == 0)
	{
		printf("Transfer timed out\n");
	}

	fclose(pFile);
}

void rwqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params)
{
	unsigned short int bnum = 1;
	unsigned short int sz  = 0;

	tftp_rwq_hdr rwq;
	tftp_data_hdr data;
	tftp_ack_hdr ack;
	
	bzero(&data, sizeof(tftp_data_hdr));
	bzero(&rwq, sizeof(tftp_rwq_hdr));

	rwq_deserialize(&rwq, buffer);
	timeout_option(&rwq, params, sockID, sockInfo);

	if(mode_transfer(rwq.mode, "octet", sockID, sockInfo, params) == -1)
	{
		return;
	}

	FILE *pFile = NULL;
	if((pFile = open_file(rwq.filename, "rb", sockID, sockInfo, params)) == NULL)
	{
		return;
	}

	int t_out = 0;
	int intents = 5;
	
	do
	{
		bzero(buffer, MAX_BUFFER);
		bzero(&data, sizeof(tftp_data_hdr));

		data.opcode = RFC1350_OP_DATA;
		data.num_block = bnum++;

		sz = fread(data.data, 1, RFC1350_BLOCKSIZE, pFile);
		sz = data_serialize(&data, buffer, sz);		
	
		bzero(&ack, sizeof(tftp_ack_hdr));
		intents = 5;
		
		do
		{
			sendInfo(sockID, sockInfo, buffer, sz);
			intents--;
			
			if((t_out = select_func(sockID, params->rexmt)) == 1)
			{
				get_ack(&ack, sockID, sockInfo, params);
				ack_serialize(&ack, buffer);
			}
		}while(t_out == 0 && intents);
	}while((sz - SHORT_SIZE * 2) == RFC1350_BLOCKSIZE && t_out != -1);

	if(intents == 0)
	{
		printf("Transfer timed out\n");
	}

	fclose(pFile);
}

//================

int timeout_option(tftp_rwq_hdr *rwq, struct PARAMS *params, int sockID, struct sockaddr_in sockInfo)
{
	int ok = 0;

	if(!strcmp(rwq->tout, "timeout") && atoi(rwq->toutv) > 0 && atoi(rwq->toutv) < 256)
	{
		ok = 1;
		params->rexmt = atoi(rwq->toutv);
		sendACKOpt(sockID, sockInfo, "timeout", params->rexmt);

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

int mode_transfer(char *modeFound, char *modeCorrect, int sockID, struct sockaddr_in sockInfo, struct PARAMS *params)
{
	int ok = 1;

	if(strcmp(modeFound, modeCorrect))
	{
		if(params->verbose)
		{
			printf("ERROR Client transfer mode is set to: %s\n", modeFound);
		}

		sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ILEGALOP, "Set transfer mode to octet");
		ok = -1;
	}

	return(ok);
}

FILE *open_file(char *fName, char *mode, int sockID, struct sockaddr_in sockInfo, struct  PARAMS *params)
{
	FILE *pFile = NULL;
	
	if((pFile = fopen(fName, mode)) == NULL)
	{
		if(params->verbose)
		{
			printf("Error opening file: %s\n", fName);
		}

		sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_FNOTFOUND, "\0");
	}

	return(pFile);
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
