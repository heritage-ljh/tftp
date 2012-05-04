#include "common.h"

void 	wrqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params);
void 	rwqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params);

void	terminator();
int 	socketBind();

int main(int argc, char *argv[])
{
	if(argc > 5)
	{
		printf("triviald -h\n");
		return(EXIT_FAILURE);
	}

	int s_id	= 0;
	char buffer[MAX_BUFFER];
	socklen_t socklen = sizeof(struct sockaddr_in);

	struct PARAMS params;
	struct sockaddr_in sockInfo;

	bzero(buffer, MAX_BUFFER);
	bzero(&params, sizeof(struct PARAMS));
	bzero(&sockInfo, sizeof(struct sockaddr_in));

	params.sc 		= TRIVIALD;
	params.port 	= 8108;
	params.rexmt 	= 5;

	param_parser(argc, argv, &params);

	if( (s_id = create_socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("Error creating socket\n");
		exit(EXIT_FAILURE);
	}

	if(make_bind(s_id, AF_INET, params.port, NULL) == -1)
	{
		printf("Error making bind\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, &terminator);

	while(1)
	{

		if(params.verbose)
		{
			printf("Waiting for new connection\n");
		}

		bzero(buffer, MAX_BUFFER);
		recvfrom(s_id, (char *) buffer, MAX_BUFFER, 0, (struct sockaddr *) &sockInfo, &socklen);
		
		if(fork() == 0)
		{
			close(s_id);
			s_id = socketBind();

			switch(ldshort(buffer))
			{
				case RFC1350_OP_RRQ:
				{
					rwqAction(s_id, sockInfo, buffer, &params);
				}break;

				case RFC1350_OP_WRQ:
				{
					wrqAction(s_id, sockInfo, buffer, &params);
				}break;
				
				default:
				{
					if(sendError(s_id, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ILEGALOP, "Illegal TFTP operation") == -1)
					{
						printf("Error default case\n");
					}
				}break;
			}
			break;
		}
	}
	
	close(s_id);
	return(EXIT_SUCCESS);
}

void terminator()
{
	while(wait3(NULL, WNOHANG, NULL) > 0 );
}

int socketBind()
{
	int s_id = 0;

	if( (s_id = create_socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("Error creating socket\n");
		exit(EXIT_FAILURE);
	}

	if(make_bind(s_id, AF_INET, 0, NULL) == -1)
	{
		printf("Error making bind\n");
		exit(EXIT_FAILURE);
	}

	return(s_id);
}

void wrqAction(int sockID, struct sockaddr_in sockInfo, char *buffer, struct PARAMS *params)
{
	tftp_rwq_hdr rwq;
	tftp_data_hdr data;
	
	rwq_deserialize(&rwq, buffer);
	if(mode_transfer(rwq.mode, "octet", params) == -1)
	{
		sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_NOTDEF, "Set transfer mode to octet");
		return;
	}

	if(file_exists(rwq.filename) == 1)
	{
		sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_FEXISTS, "File already exists");
		return;
	}



	FILE *pFile = NULL;
	if((pFile = open_file(rwq.filename, "wb", params)) == NULL)
	{
		switch(errno)
		{
			case EACCES:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ACCESS, "Permission denied");
			}break;

			case ENOMEM:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_DISKFULL, "Not enough space");
			}break;

			default:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_NOTDEF, strerror(errno));
			}break;
		}
		return;
	}

	if(timeout_option(&rwq, params) == -1)
	{
		printf("Ok!");
		sendACK(sockID, sockInfo, 0);
	}
	else
	{
		printf("NOk!");
		sendACKOpt(sockID, sockInfo, "timeout", params->rexmt);
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
			if((t_out = select_func(sockID, params->rexmt)) == 1)
			{
				sz = get_data(&data, sockID, sockInfo, params);
				sendACK(sockID, sockInfo, data.num_block);
				fwrite(data.data, sz, 1, pFile);
			}
		}while(t_out == 0 && --intents);
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

	if(mode_transfer(rwq.mode, "octet", params) == -1)
	{
		sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_NOTDEF, "Set transfer mode to octet");
		return;
	}

	FILE *pFile = NULL;
	if((pFile = open_file(rwq.filename, "rb", params)) == NULL)
	{
		switch(errno)
		{
			case ENOENT:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_FNOTFOUND, "No such file or directory");
			}break;

			case EACCES:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ACCESS, "Permission denied");
			}break;

			default:
			{
				sendError(sockID, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_NOTDEF, strerror(errno));
			}break;
		}

		return;
	}

	if(timeout_option(&rwq, params) == 1)
	{
		sendACKOpt(sockID, sockInfo, "timeout", params->rexmt);
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
			if((t_out = select_func(sockID, params->rexmt)) == 1)
			{
				get_ack(&ack, sockID, sockInfo, params);
				ack_serialize(&ack, buffer);
			}
		}while(t_out == 0 && --intents);
	}while((sz - SHORT_SIZE * 2) == RFC1350_BLOCKSIZE && t_out != -1);

	if(intents == 0)
	{
		printf("Transfer timed out\n");
	}

	fclose(pFile);
}
