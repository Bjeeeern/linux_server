#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>

#include "handmade.h"

#define BUFFER_SIZE Kilobytes(64)

internal_function void
handle_request_through_fork(s32 client_socket_handle, u8 *data_received, s32 length_of_data)
{
	write(client_socket_handle, 
				"HTTP/1.1 200 OK\n"
				"Content-Length: 52\n"
				"Content-Type: text/html; charset=utf-8\n"
				"\n"
				"<html>\n"
				"<body>\n"
				"<h1>Hello, World!</h1>\n"
				"</body>\n"
				"</html>", 127);
}

	s32 
main()
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
						int client_socket_handle = -1;
						while(client_socket_handle == -1)
						{
							client_socket_handle = accept(SocketHandle, 
																						(struct sockaddr *)&ClientAddress, 
																						&ClientAddressLenght);
							usleep(16000);
						}

						u8 buffer[BUFFER_SIZE] = {};
						int bytes_received = -1;
						while(bytes_received != 0)
						{
							bytes_received = recv(client_socket_handle, 
																		(void *)buffer, BUFFER_SIZE-1, 0);
							if(bytes_received > 0)
							{
								u8* data_recieved_from_client = (u8*)mmap(0, bytes_received + 1,
																															PROT_READ | PROT_WRITE,
																															MAP_PRIVATE 
																															| MAP_ANONYMOUS,
																															-1, 0);
								for(s32 byte_index = 0;
										byte_index < bytes_received;
										++byte_index)
								{
									data_recieved_from_client[byte_index] = buffer[byte_index];
								}
								data_recieved_from_client[bytes_received] = '\0';

								if(fork())
								{
									printf("[-[-[\n%s]-]-]\n", data_recieved_from_client);
								}
								else
								{
									handle_request_through_fork(client_socket_handle, 
																							data_recieved_from_client, 
																							bytes_received + 1);
									_exit(0);
								}
							}
							usleep(16000);
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
