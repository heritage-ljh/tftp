#include "common.h"

void 	wrqAction(int sockID, struct sockaddr_in sockInfo, struct PARAMS *params);
void 	rwqAction(int sockID, struct sockaddr_in sockInfo, struct PARAMS *params);

int main(int argc, char *argv[])
{
	if(argc > 12)
	{
		printf("Numero de argumentos incorrecto\n");
		return(EXIT_FAILURE);
	}
	

	int c_id = 0;
	struct PARAMS params;
	struct sockaddr_in sockInfo;
	
	bzero(&params, sizeof(struct PARAMS));
	bzero(&sockInfo, sizeof(struct sockaddr_in));

	params.sc   = TRIVIAL;
	params.port = 8108;
	params.rexmt = 5;

	strcpy(params.host, "localhost");
	param_parser(argc, argv, &params);

	if( (c_id = create_socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("Error al crear el socket\n");
		exit(EXIT_FAILURE);
	}

	if(make_bind(c_id, AF_INET, 0, NULL) == -1)
	{
		printf("Error al hacer el bind\n");
		exit(EXIT_FAILURE);
	}

	sockInfo.sin_family	= AF_INET;
	sockInfo.sin_port	= htons((unsigned short int) params.port);
	sockInfo.sin_addr	= *(struct in_addr *)gethostbyname(params.host)->h_addr;
	
	switch(params.read_write)
	{
		case RFC1350_OP_RRQ:
		{
			rwqAction(c_id, sockInfo, &params);
		}break;

		case RFC1350_OP_WRQ:
		{
			wrqAction(c_id, sockInfo, &params);
		}break;

		default:
		{
			if(sendError(c_id, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ILEGALOP, "Illegal TFTP operation") == -1)
			{
				printf("Error default case\n");
			}
		}break;			
	}

	close(c_id);
	return(EXIT_SUCCESS);
}

void wrqAction(int sockID, struct sockaddr_in sockInfo, struct PARAMS *params)
{
	tftp_rwq_hdr rwq;			bzero(&rwq, sizeof(tftp_rwq_hdr));
	tftp_data_hdr data;			bzero(&data, sizeof(tftp_data_hdr));
	char buffer[MAX_BUFFER];	bzero(buffer, MAX_BUFFER);

	int sz = 0;
	int fack = 0;
	int t_out = 0;
	int intents = 5;
	int blocknum = 1;
	socklen_t socklen = sizeof(struct sockaddr_in);

	FILE *pFile = NULL;
	if((pFile = open_file(params->file, "rb", params)) == NULL)
	{
		exit(EXIT_FAILURE);
	}

	
	rwq.opcode = RFC1350_OP_WRQ;
	strcpy(rwq.filename, params->file);
	strcpy(rwq.mode, "octet");

	/*if(params->rexmt != 5)
	{
		strcpy(rwq.tout, "timeout");
		snprintf(rwq.toutv, 4, "%d", params->rexmt);
	}*/

	sz = rwq_serialize(&rwq, buffer);
	sendInfo(sockID, sockInfo, buffer, sz);
	

	do
	{
		sz = 0;
		intents = 5;

		do
		{
			if((t_out = select_func(sockID, params->rexmt)) == 1)
			{
				sz = recvfrom(sockID, (char *) buffer, MAX_BUFFER, 0, (struct sockaddr *) &sockInfo, &socklen);
				switch(ldshort(buffer))
				{
					case RFC1350_OP_ACK:
					{
						tftp_ack_hdr ack;
						ack_deserialize(&ack, buffer);
						if(fack == 0)
						{
							fack++;
							if(ack.num_block != 0)
							{
								exit(EXIT_FAILURE);
								fclose(pFile);
							}
						}

						data.num_block = blocknum++;
						sz = fread(data.data, 1, RFC1350_BLOCKSIZE, pFile);
						sz = data_serialize(&data, buffer, sz);
						sendInfo(sockID, sockInfo, buffer, sz);	

						if(params->verbose)
						{
							printf("S\t %s: ACK [%d#]\n", inet_ntoa(sockInfo.sin_addr), ack.num_block);
							printf("C\t DATA [%d#] [%lu]\n", data.num_block, sz - SHORT_SIZE * 2);
						}
					}break;

					case RFC1350_OP_ERROR:
					{
						tftp_error_hdr error;
						error_deserialize(&error, buffer, sz);
						printf("Error Code: %d\n", error.code);
						printf("Error Text: %s\n", error.message);

					}break;

					default:
					{
						exit(EXIT_FAILURE);
						fclose(pFile);
					}
				}
			}
		}while(t_out == 0 && --intents);
	}while((sz - SHORT_SIZE * 2) == RFC1350_BLOCKSIZE && t_out != -1);

	if(intents == 0)
	{
		printf("Transfer timed out\n");
	}

	fclose(pFile);
}

void rwqAction(int sockID, struct sockaddr_in sockInfo, struct PARAMS *params)
{
	unsigned short int bnum = 1;
	unsigned short int sz  = 0;

	tftp_ack_hdr ack;
	tftp_rwq_hdr rwq;
	tftp_data_hdr data;
	char buffer[MAX_BUFFER];
	
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