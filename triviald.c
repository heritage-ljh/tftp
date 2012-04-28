#include "common.h"

void terminator()
{
	while(wait3(NULL, WNOHANG, NULL) > 0 );
}

int socketBind()
{
	int s_id = 0;

	if( (s_id = create_socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("Error al crear el socket\n");
		exit(EXIT_FAILURE);
	}

	if(make_bind(s_id, AF_INET, 0, NULL) == -1)
	{
		printf("Error al hacer el bind\n");
		exit(EXIT_FAILURE);
	}

	return(s_id);
}

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
		printf("Error al crear el socket\n");
		exit(EXIT_FAILURE);
	}

	if(make_bind(s_id, AF_INET, params.port, NULL) == -1)
	{
		printf("Error al hacer el bind\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, &terminator);

	while(1)
	{

		if(params.verbose)
		{
			printf("Esperando por nueva connexion\n");
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