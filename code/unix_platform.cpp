#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>

#include "handmade.h"

#define BUFFER_SIZE 1024

s32 main()
{
	s32 SocketHandle = socket(AF_INET, SOCK_STREAM, 0);
	if(SocketHandle != -1)
	{
		int flags = fcntl(SocketHandle, F_GETFL, 0);
		if(fcntl(SocketHandle, F_SETFL, flags | O_NONBLOCK) != -1)
		{
			struct sockaddr_in Address = {};
			//TODO(bjorn): Read this from a settings text file.
			Address.sin_family = AF_INET;
			Address.sin_port = htons(8000);
			Address.sin_addr.s_addr = INADDR_ANY;

			if(bind(SocketHandle, (struct sockaddr *)&Address, sizeof(Address)) == 0)
			{
				if(listen(SocketHandle, 10) == 0)
				{
					while(true)
					{
						struct sockaddr_in ClientAddress = {};
						socklen_t ClientAddressLenght = sizeof(ClientAddress);
						int ClientSocketHandle = -1;
						while(ClientSocketHandle == -1)
						{
							ClientSocketHandle = accept(SocketHandle, 
																					(struct sockaddr *)&ClientAddress, 
																					&ClientAddressLenght);
							usleep(1000);
						}

						char Buffer[BUFFER_SIZE] = {};
						int BytesRecieved = -1;
						while(BytesRecieved != 0)
						{
							BytesRecieved = recv(ClientSocketHandle, (void *)Buffer, BUFFER_SIZE-1, 0);
							if(BytesRecieved > 0)
							{
								printf("[-[-[%s]-]-]\n",Buffer);
							}
							usleep(1000);
						}
						printf("Connection ended correctly\n");
					}
				}
				else
				{
					//TODO(bjorn): Log this.
				}
			}
			else
			{
				//TODO(bjorn): Log this.
			}
		}
		else
		{
			//TODO(bjorn): Log this.
		}
	}
	else
	{
		//TODO(bjorn): Log this.
	}

	return 0;
}
