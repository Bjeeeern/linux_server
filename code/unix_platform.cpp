#include "connection_protocol.h"

#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h> 
#include <limits.h>

#include <stdio.h>
#include <string.h>


#define BUFFER_SIZE Kilobytes(64)
#define SIMULTANEOUS_CONNECTIONS 16

	s32 
main()
{
	//execute_shell_command("cd /Users/kuma/Desktop/myProjects/linux_server/code && git pull && make build");
	char current_working_directory[1024] = {};
	if(readlink("/proc/self/exe", current_working_directory, 1024) == -1)
	{
		// TODO(bjorn): Figure out what to do here.
		//log_string("getcwd()\n");
		//log_string("could not get hold of current directory\n");
		printf("crashed\n");
		return 0;
	}

	char dyn_lib_file_names[256][64] = {};

	DIR *directory_handle;
  struct dirent *directory_info;
  directory_handle = opendir(current_working_directory);
	if(directory_handle == 0)
	{
		// TODO(bjorn): Figure out what to do here.
		return 0;
	}
	s32 file_name_index = 0;
	while((directory_info = readdir(directory_handle)) != NULL)
	{
		printf("%s\n", directory_info->d_name);
		strcpy(&(dyn_lib_file_names[file_name_index++][0]), 
					 directory_info->d_name);
	}
	closedir(directory_handle);

/*
	s32 socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle != -1)
	{
		int flags = fcntl(socket_handle, F_GETFL, 0);
		if(fcntl(socket_handle, F_SETFL, flags | O_NONBLOCK) != -1)
		{
			struct sockaddr_in address = {};
			//TODO(bjorn): Read this from a settings text file.
			address.sin_family = AF_INET;
			address.sin_port = htons(8000);
			address.sin_addr.s_addr = INADDR_ANY;

			if(bind(socket_handle, (struct sockaddr *)&address, sizeof(address)) == 0)
			{
				if(listen(socket_handle, SIMULTANEOUS_CONNECTIONS) == 0)
				{
					while(true)
					{
						struct sockaddr_in client_address = {};
						socklen_t client_address_lenght = sizeof(client_address);

						s32 client_socket_handle = accept(socket_handle, 
																							(struct sockaddr *)&client_address, 
																							&client_address_lenght);
						if(client_socket_handle == -1)
						{
							api.sleep_x_seconds(0.016f); }
						else
						{
							//TODO(bjorn): There should maybe always be bytes here waiting.
							s32 bytes_waiting = api.bytes_in_connection_queue(client_socket_handle);

							if(bytes_waiting > 0)
							{
								s32 pid = fork();

								if(pid == 0)
								{
									//NOTE(bjorn): For thread debugging purposes.
									//kill(0, SIGSTOP);

									//TODO(bjorn): Allocate here within the new thread, no
									//leakage and automatic cleanup guaranteed

									// s32 bytes_read = recv(connection_id, //											load_location, 
									//											bytes_to_read, MSG_PEEK);

									connection_memory memory = {};
									handle_connection_stub(client_socket_handle, pid, memory);

									close(client_socket_handle);
									api.log_string("Connection ended.\n");

									_exit(0);
								}
							}
						}
					}
				}
				else
				{
					//TODO(bjorn): Log this.
					api.log_string("Can not listen to socket for simultaneous connections.\n");
				}
			}
			else
			{
				//TODO(bjorn): Log this.
				api.log_string("Can not bind socket properly.\n");
			}
		}
		else
		{
			//TODO(bjorn): Log this.
			api.log_string("Can not set socket flags properly.\n");
		}
	}
	else
	{
		//TODO(bjorn): Log this.
		api.log_string("Can not create an Inet socket.\n");
	}
*/
	return 0;
}
