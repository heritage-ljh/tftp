#include "common.h"

int main(int argc, char *argv[])
{
	if(argc > 12)
	{
		printf("Numero de argumentos incorrecto\n");
		return(EXIT_FAILURE);
	}
	

	int c_id = 0;
	char buffer[MAX_BUFFER];

	struct PARAMS params;
	struct sockaddr_in sockInfo;
	
	bzero(&params, sizeof(struct PARAMS));
	bzero(&sockInfo, sizeof(struct sockaddr_in));

	params.sc   = TRIVIAL;
	params.port = 8108;
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

	tftp_rwq_hdr rwq;
	rwq.opcode = RFC1350_OP_WRQ;
	strcpy(rwq.filename, params.file);
	strcpy(rwq.mode, "octet");

	int sz = rwq_serialize(&rwq, buffer);
	sendto(c_id, (char *) buffer, sz, 0, (struct sockaddr *) &sockInfo, sizeof(struct sockaddr_in));

	//tftp_ack_hdr ack;
	recvfrom(c_id, (char *) buffer, MAX_BUFFER, 0, NULL, NULL);
	
	switch(params.read_write)
	{
		case RFC1350_OP_RRQ:
		{
			//rwqAction(c_id, sockInfo, buffer, &params);
		}break;

		case RFC1350_OP_WRQ:
		{
			//wrqAction(c_id, sockInfo, buffer, &params);
		}break;

		default:
		{
			if(sendError(c_id, sockInfo, RFC1350_OP_ERROR, RFC1350_ERR_ILEGALOP, "Illegal TFTP operation") == -1)
			{
				printf("Error default case\n");
			}
		}break;			
	}

//	ack_deserialize(&ack, buffer);

	close(c_id);
	return(EXIT_SUCCESS);
}
