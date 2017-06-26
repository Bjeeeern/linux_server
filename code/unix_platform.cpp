#include "connection_protocol.h"


#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h> 
#include <limits.h>
#include <dlfcn.h>
#include <sys/stat.h> 
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>


#define BUFFER_SIZE Kilobytes(64)
#define SIMULTANEOUS_CONNECTIONS 16

struct protocol_api
{
	server_handle_connection *handle_connection;
	server_this_is_my_protocol *this_is_my_protocol;
};

internal_function b32
string_matches_tag(char *string, char *tag)
{
	b32 result = true;
	while(*string != '\0')
	{ 
		if(*string++ != *tag++){ result = false; }
	}
	return result;
}

	s32 
main()
{
	char current_working_directory[1024] = {};

#ifdef __APPLE__
  u32 size = 1024;

  if (_NSGetExecutablePath(current_working_directory, &size) != 0) {
    // Buffer size is too small.
		printf("_NSGetExecutablePath() failed\n");
    return 2;
  }
#else // not __APPLE__
	if(readlink("/proc/self/exe", current_working_directory, 1024) == -1)
	{
		// TODO(bjorn): Figure out what to do here.
		//log_string("getcwd()\n");
		//log_string("could not get hold of current directory\n");
		printf("readlink(\"/proc/self/exe\") failed\n");
		return 2;
	}
#endif
	{
		char *tail_pointer = current_working_directory;
		while(*tail_pointer != '\0'){ ++tail_pointer; }
		while(*tail_pointer != '/'){ --tail_pointer; }
		*(tail_pointer+1) = '\0';
	}

	printf("cwd: %s\n", current_working_directory);

	char dyn_lib_file_names[256][64] = {};
	s32 dyn_lib_file_count = 0;

	{
		DIR *directory_handle;
		struct dirent *directory_info;
		directory_handle = opendir(current_working_directory);
		if(directory_handle == 0)
		{
			printf("opendir(\"%s\") failed\n", current_working_directory);
			return 2;
		}

		s32 file_name_index = 0;
		while((directory_info = readdir(directory_handle)) != NULL)
		{
			s32 file_name_lenght = 0;
			{
				char *tail_pointer = directory_info->d_name;
				while(*tail_pointer++ != '\0'){ ++file_name_lenght; }
			}

			if(file_name_lenght >= 7) // dir___.so
			{
				char *tail_pointer = directory_info->d_name;
				while(*tail_pointer != '\0'){ ++tail_pointer; }
				tail_pointer -= 3;

				b32 is_a_dynlib_file = string_matches_tag(tail_pointer, ".so");
				if(is_a_dynlib_file)
				{
					printf("%s\n", directory_info->d_name);
					strcpy(&(dyn_lib_file_names[file_name_index++][0]), directory_info->d_name);
				}
			}
		}
		closedir(directory_handle);
		dyn_lib_file_count = file_name_index;
	}


	{
		s32 file_name_index = 0;
		while(file_name_index < dyn_lib_file_count)
		{
			printf("%s\n", &(dyn_lib_file_names[file_name_index++][0]));
		}
	}

	char api_function_names[MAX_NUMBER_OF_PLATFORM_FUNCTIONS][256];
	s32 api_function_count = 0;
	{
		s32 file_handle;
		{
			char *api_name_order_file = "platform_api_names_in_order.txt";
			char *tail_pointer = current_working_directory;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			while(*api_name_order_file != '\0'){ *tail_pointer++ = *api_name_order_file++; }
			*tail_pointer = '\0';

			char *absolute_path_to_api_name_order_file = current_working_directory;
			file_handle = open(absolute_path_to_api_name_order_file, O_RDONLY);
			if(file_handle == -1)
			{
				printf("open(%s)", absolute_path_to_api_name_order_file);
				printf("failed to open");
				return 2;
			}

			tail_pointer = current_working_directory;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			while(*tail_pointer != '/'){ --tail_pointer; }
			*(tail_pointer+1) = '\0';
		}

		struct stat file_stats;
		fstat(file_handle, &file_stats);

		char *temp_file = (char *)mmap(0, file_stats.st_size, 
													 PROT_READ, MAP_PRIVATE, file_handle, 0);
		close(file_handle);

		s32 function_index = 0;
		s32 char_index = 0;
		while(*temp_file != '\0')
		{
			if(*temp_file == '\n' || *temp_file == '\r')
			{
				if(*temp_file == '\r' && *(temp_file+1) == '\n')
				{
					++temp_file;
				}
				api_function_names[function_index][char_index++] = '\0';
				function_index += 1;
				char_index = 0;
			}
			else
			{
				api_function_names[function_index][char_index++] = *temp_file;
			}
			++temp_file;
		}
		api_function_names[function_index][char_index] = '\0';
		//NOTE(bjorn): The function_index was incremented on the last newline.
		api_function_count = function_index;

		munmap(temp_file, file_stats.st_size);

		s32 function_name_index = 0;
		while(function_name_index < api_function_count)
		{
			printf("%s\n", &(api_function_names[function_name_index++][0]));
		}
	}

	//TODO(bjorn): load the dlls! :3

	connection_memory connection_memory_template = {};
#define MEMORY_PER_THREAD Megabytes(1)
	connection_memory_template.storage_size = MEMORY_PER_THREAD;


	char path_to_webroot[2048] = {};
	char *write_destination = path_to_webroot;
	{
		char *copy_pointer = current_working_directory;
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }

		copy_pointer = "../webroot/";
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }
		*write_destination = '\0';
	}

	connection_memory_template.directory_of_executable = path_to_webroot;

#define MAX_NUMBER_OF_PROTOCOLS 256
	protocol_api protocols[MAX_NUMBER_OF_PROTOCOLS] = {}; 
	s32 protocol_count = dyn_lib_file_count - 1;

	void *dyn_lib_handels[MAX_NUMBER_OF_PROTOCOLS + 1] = {};

	s32 protocol_index = 0;
	for(s32 dyn_lib_file_index = 0;
			dyn_lib_file_index < dyn_lib_file_count;
			++dyn_lib_file_index)
	{
		char *dyn_lib_file = &(dyn_lib_file_names[dyn_lib_file_index][0]);

		void *dyn_lib_handle;
		{
			char *tail_pointer = current_working_directory;
			char *copy_pointer = dyn_lib_file;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			while(*copy_pointer != '\0'){ *tail_pointer++ = *copy_pointer++; }
			*tail_pointer = '\0';

			char *absolute_path_to_dyn_lib_file = current_working_directory;
			dyn_lib_handle = dlopen(absolute_path_to_dyn_lib_file, RTLD_NOW);

			tail_pointer = current_working_directory;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			while(*tail_pointer != '/'){ --tail_pointer; }
			*(tail_pointer+1) = '\0';
		}

		if(string_matches_tag(dyn_lib_file, "lib_unix_api.so"))
		{
			for(s32 api_function_index = 0;
					api_function_index < api_function_count;
					++api_function_index)
			{
				connection_memory_template.api_function_pointers[api_function_index] = 
					dlsym(dyn_lib_handle, api_function_names[api_function_index]);
			}
		}
		else
		{
			if(protocol_index < protocol_count)
			{
				protocols[protocol_index].handle_connection = 
					(server_handle_connection *)dlsym(dyn_lib_handle, "handle_connection");
				protocols[protocol_index].this_is_my_protocol = 
					(server_this_is_my_protocol *)dlsym(dyn_lib_handle, "this_is_my_protocol");

				protocol_index += 1;
			}
		}
	}

	connection_memory_template.api.log_string("jaaa");
	connection_memory_template.api.execute_shell_command("echo \"ja gjorde de!\"");
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
