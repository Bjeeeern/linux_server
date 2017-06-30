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
#include <sys/ioctl.h>

#include <stdio.h>
#include <string.h>


#define BUFFER_SIZE Kilobytes(64)
#define SIMULTANEOUS_CONNECTIONS 16
#define MEMORY_PER_THREAD Megabytes(1)

struct protocol_api
{
	server_handle_connection *handle_connection;
	server_this_is_my_protocol *this_is_my_protocol;

	void *dll_handle;
	char dll_path[FILE_PATH_MAX_SIZE];
};

internal_function b32
string_matches_tag(char *string, char *tag)
{
	while(*tag != '\0')
	{ 
		if(*string == '\0') { return false; }
		if(*string++ != *tag++){ return false; }
	}
	return true;
}

struct string_matrix
{
	char *strings;
	s32 lines;
	s32 max_line_width;
	char *
		at(s32 line, s32 character)
		{
			return &((this->strings)[line*this->max_line_width + character]);
		}
};
	internal_function string_matrix
all_dyn_lib_names_in_dir(char *directory)
{
	string_matrix result = {};
	
	DIR *directory_handle;
	struct dirent *directory_info;

	directory_handle = opendir(directory);
	if(directory_handle == 0)
	{
		printf("opendir(\"%s\") failed\n", directory);
		return result;
	}

	while((directory_info = readdir(directory_handle)) != NULL)
	{
		s32 file_name_lenght = 0;
		{
			char *counter = directory_info->d_name;
			while(*counter++ != '\0'){ ++file_name_lenght; }
			++file_name_lenght;
		}

		if(file_name_lenght >= 8) //NOTE(bjorn): dir + blah + .so\0
		{
			char *tail_pointer = directory_info->d_name;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			tail_pointer -= 3;

			b32 is_a_dynlib_file = string_matches_tag(tail_pointer, ".so");
			if(is_a_dynlib_file)
			{
				if(file_name_lenght > result.max_line_width) 
				{
					result.max_line_width = file_name_lenght;
				}
				result.lines += 1;
			}
		}
	}
	closedir(directory_handle);

	result.strings = (char *)mmap(0, result.lines * result.max_line_width, 
												 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	directory_handle = opendir(directory);

	s32 file_index = 0;
	while((directory_info = readdir(directory_handle)) != NULL)
	{
		s32 file_name_lenght = 0;
		{
			char *counter = directory_info->d_name;
			while(*counter++ != '\0'){ ++file_name_lenght; }
			++file_name_lenght;
		}

		if(file_name_lenght >= 8) //NOTE(bjorn): dir + blah + .so\0
		{
			char *tail_pointer = directory_info->d_name;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			tail_pointer -= 3;

			b32 is_a_dynlib_file = string_matches_tag(tail_pointer, ".so");
			if(is_a_dynlib_file)
			{
				char *copy_pointer = directory_info->d_name;
				s32 char_index = 0;
				while(*copy_pointer != '\0')
				{ 
					*result.at(file_index, char_index++) = *copy_pointer++; 
				}
				*result.at(file_index, char_index) = '\0';

				file_index += 1;
			}
		}
	}
	closedir(directory_handle);
	return result;
}

	internal_function string_matrix 
all_api_function_names(char *dir, char *api_name_order_file)
{
	string_matrix result = {};

	char full_path[FILE_PATH_MAX_SIZE] = {};
	{
		s32 char_index = 0;

		char *copy_pointer = dir;
		while(*copy_pointer != '\0'){ full_path[char_index++] = *copy_pointer++; }
		copy_pointer = api_name_order_file;
		while(*copy_pointer != '\0'){ full_path[char_index++] = *copy_pointer++; }
		full_path[char_index] = '\0';
	}

	s32 file_handle;
	{
		file_handle = open(full_path, O_RDONLY);
		if(file_handle == -1)
		{
			printf("open(%s)", full_path);
			printf("failed to open");
			return result;
		}
	}

	struct stat file_stats;
	fstat(file_handle, &file_stats);

	char *temp_file = (char *)mmap(0, file_stats.st_size, 
																		 PROT_READ, MAP_PRIVATE, file_handle, 0);
	close(file_handle);

	{
		s32 function_index = 0;
		s32 char_index = 0;
		char *file_pointer = temp_file;
		while(*file_pointer != '\0')
		{
			if(*file_pointer == '\n' || *file_pointer == '\r')
			{
				if(*file_pointer == '\r' && *(file_pointer+1) == '\n')
				{
					++file_pointer;
				}
				function_index += 1;

				if(char_index+1 > result.max_line_width)
				{
					result.max_line_width = char_index + 1;
				}
				char_index = 0;
			}
			else
			{
				++char_index;
			}
			++file_pointer;
		}
		//NOTE(bjorn): The function_index was incremented on the last newline.
		result.lines = function_index;
	}

	result.strings = (char *)mmap(0, result.lines * result.max_line_width, 
												 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	{
		s32 function_index = 0;
		s32 char_index = 0;
		char *file_pointer = temp_file;
		while(*file_pointer != '\0')
		{
			if(*file_pointer == '\n' || *file_pointer == '\r')
			{
				if(*file_pointer == '\r' && *(file_pointer+1) == '\n')
				{
					++file_pointer;
				}
				*result.at(function_index, char_index++) = '\0';
				function_index += 1;
				char_index = 0;
			}
			else
			{
				*result.at(function_index, char_index++) = *file_pointer;
			}
			++file_pointer;
		}
		*result.at(function_index, char_index) = '\0';
	}

	munmap(temp_file, file_stats.st_size);

	return result;
}

internal_function void
load_dyn_libs(string_matrix dyn_libs, string_matrix api_funcs, 
							char *dir_of_dlls, connection_memory *conn_mem_temp, 
							protocol_api *protocols, s32 protocol_count)
{
	s32 protocol_index = 0; 
	for(s32 dyn_lib_file_index = 0;
			dyn_lib_file_index < dyn_libs.lines;
			++dyn_lib_file_index)
	{
		char *dyn_lib_file = dyn_libs.at(dyn_lib_file_index, 0);

		void *dyn_lib_handle;
		char absolute_path_to_dyn_lib_file[FILE_PATH_MAX_SIZE] = {};
		{
			char *copy_pointer = dir_of_dlls;
			char *dest_pointer = absolute_path_to_dyn_lib_file;
			while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
			copy_pointer = dyn_lib_file;
			while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
			*dest_pointer = '\0';

			dyn_lib_handle = dlopen(absolute_path_to_dyn_lib_file, RTLD_NOW);
		}

		if(string_matches_tag(dyn_lib_file, "lib_unix_api"))
		{
			for(s32 api_function_index = 0;
					api_function_index < api_funcs.lines;
					++api_function_index)
			{
				conn_mem_temp->api_function_pointers[api_function_index] = 
					dlsym(dyn_lib_handle, api_funcs.at(api_function_index, 0));

				conn_mem_temp->dll_handle = dyn_lib_handle;
				char *copy_pointer = absolute_path_to_dyn_lib_file;
				char *dest_pointer = conn_mem_temp->dll_path;
				while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
				*dest_pointer = '\0';
			}
		}
		else
		{
			if(protocol_index < protocol_count)
			{
				protocols[protocol_index].handle_connection = 
					(server_handle_connection *)dlsym(dyn_lib_handle, "handle_connection");
				protocols[protocol_index].this_is_my_protocol = 
					(server_this_is_my_protocol *)dlsym(dyn_lib_handle,
																							"this_is_my_protocol");

				protocols[protocol_index].dll_handle = dyn_lib_handle;
				char *copy_pointer = absolute_path_to_dyn_lib_file;
				char *dest_pointer = protocols[protocol_index].dll_path;
				while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
				*dest_pointer = '\0';

				protocol_index += 1;
			}
		}
	}
}


	s32 
main()
{
	char current_working_directory[FILE_PATH_MAX_SIZE] = {};
	u32 size = FILE_PATH_MAX_SIZE;

#ifdef __APPLE__
	if (_NSGetExecutablePath(current_working_directory, &size) != 0) {
		// Buffer size is too small.
		printf("_NSGetExecutablePath() failed\n");
		return 2;
	}
#else // not __APPLE__
	if(readlink("/proc/self/exe", current_working_directory, size) == -1)
	{
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

	string_matrix dyn_libs = all_dyn_lib_names_in_dir(current_working_directory);

	{
		s32 file_index = 0;
		while(file_index < dyn_libs.lines)
		{
			printf("%s\n", dyn_libs.at(file_index, 0));
			file_index++;
		}
	}

	string_matrix api_funcs = all_api_function_names(current_working_directory, 
																									 "platform_api_names_in_order.txt");

	{
		s32 function_name_index = 0;
		while(function_name_index < api_funcs.lines)
		{
			printf("%s\n", api_funcs.at(function_name_index++, 0));
		}
	}

	connection_memory connection_memory_template = {};
	connection_memory_template.storage_size = MEMORY_PER_THREAD;


	char path_to_webroot[FILE_PATH_MAX_SIZE] = {};
	char *write_destination = path_to_webroot;
	{
		char *copy_pointer = current_working_directory;
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }

		copy_pointer = "../webroot/";
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }
		*write_destination = '\0';
	}

	connection_memory_template.path_to_webroot = path_to_webroot;

	s32 protocol_count = dyn_libs.lines - 1;
	protocol_api *protocols = (protocol_api *)mmap(0, 
																								 sizeof(protocol_api)*protocol_count,
																								 PROT_READ|PROT_WRITE,
																								 MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	load_dyn_libs(dyn_libs, api_funcs, current_working_directory, 
								&connection_memory_template, protocols, protocol_count);

	platform_log_string *api_log_string = 
		connection_memory_template.api.log_string;
	platform_sleep_x_seconds *api_sleep_x_seconds = 
		connection_memory_template.api.sleep_x_seconds;

	s32 socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle == -1)
	{
		api_log_string("Can not create an Inet socket.\n");
		return 2;
	}

	int flags = fcntl(socket_handle, F_GETFL, 0);
	if(fcntl(socket_handle, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		api_log_string("Can not set socket flags properly.\n");
		return 2;
	}

	struct sockaddr_in address = {};
	//TODO(bjorn): Read this from a settings text file.
	address.sin_family = AF_INET;
	address.sin_port = htons(8000);
	address.sin_addr.s_addr = INADDR_ANY;

	if(bind(socket_handle, (struct sockaddr *)&address, sizeof(address)) != 0)
	{
		api_log_string("Can not bind socket properly.\n");
		return 2;
	}

	if(listen(socket_handle, SIMULTANEOUS_CONNECTIONS) != 0)
	{
		api_log_string("Can not listen to socket for simultaneous connections.\n");
		return 2;
	}

	while(true)
	{
		struct sockaddr_in client_address = {};
		socklen_t client_address_lenght = sizeof(client_address);

		s32 client_socket_handle = accept(socket_handle, 
																			(struct sockaddr *)&client_address, 
																			&client_address_lenght);
		if(client_socket_handle == -1)
		{
			//TODO(bjorn): Check for updates to the dlls.
			api_sleep_x_seconds(0.016f); 
		}
		else
		{
			//TODO(bjorn): There should maybe always be bytes here waiting.
			s32 bytes_waiting;
			ioctl(client_socket_handle, FIONREAD, &bytes_waiting);

			if(bytes_waiting > 0)
			{
				s32 pid = fork();

				if(pid == 0)
				{
					//NOTE(bjorn): For thread debugging purposes.
					//kill(0, SIGSTOP);

					connection_memory memory = connection_memory_template;

					memory.storage = mmap(0, memory.storage_size, 
												 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

					dlerror();
					void *dll_handle = dlopen(memory.dll_path, RTLD_NOW);
					if(dll_handle != memory.dll_handle)
					{
						api_log_string("Repeated calls to the same dlopen is not"
													 " generating the same handle! OSAPI\n");
						api_log_string("Old handle:");
						char empty_string[256];
						api_log_string(int_to_string((s64)memory.dll_handle, empty_string));
						api_log_string("\n");
						api_log_string("New handle:");
						api_log_string(int_to_string((s64)dll_handle, empty_string));
						api_log_string("\n");

					}

					void *load_location = mmap(0, bytes_waiting, PROT_READ|PROT_WRITE,
																		 MAP_SHARED|MAP_ANONYMOUS, -1, 0);
					s32 bytes_read = recv(client_socket_handle, load_location, bytes_waiting,
															 	MSG_PEEK);
					if(bytes_waiting != bytes_read)
					{
						api_log_string("Bytes read not same as bytes in queue.\n");
					}

					b32 message_was_not_handled = true;
					for(s32 protocol_index = 0;
							protocol_index < protocol_count;
							++protocol_index)
					{
						protocol_api protocol = protocols[protocol_index];

						if(protocol.this_is_my_protocol(load_location, bytes_read)) 
						{
							
							void *dll_handle = dlopen(protocol.dll_path, RTLD_NOW);
							if(dll_handle != protocol.dll_handle)
							{
								api_log_string("Repeated calls to the same dlopen is not"
															 " generating the same handle! PROTOCOL\n");
								api_log_string("Old handle:");
								char empty_string[256];
								api_log_string(int_to_string((s64)protocol.dll_handle, empty_string));
								api_log_string("\n");
								api_log_string("New handle:");
								api_log_string(int_to_string((s64)dll_handle, empty_string));
								api_log_string("\n");
							}

							protocol.handle_connection(client_socket_handle, getpid(), memory);

							dlclose(protocol.dll_handle);

							message_was_not_handled = false;

							//TODO(bjorn): What is the right thing to do here? Should
							//multiple protocols be able to respond to a single connection?
							//Some sort of priority value?
							break;
						}
					}
					if(message_was_not_handled)
					{
						api_log_string("No fitting protocol was found for pid:");
					}
					else
					{
						api_log_string("Connection ended for pid:");
					}
					char empty_string[256];
					api_log_string(int_to_string(getpid(), empty_string));
					api_log_string("\n");

					dlclose(memory.dll_handle);
					close(client_socket_handle);

					_exit(0);
				}
			}
		}
	}
	return 0;
}
