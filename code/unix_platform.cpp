#include "connection_protocol.h"

//TODO(bjorn): A general todo list:
//
// Enable HTTPS on port 443
//	http://www.informit.com/articles/article.aspx?p=22078
// 	https://www.openssl.org/source/
// 	https://certbot.eff.org/#debianjessie-other
//
// 	(make the SSL selectable per dll)
// 	(make the dlls be able to recieve from multiple ports)
//
// Make the server be able to listen to an arbitrary number of ports, using the
// dlls to figure out from what port to which dll(s) a incoming message should
// be distributed to. Maybe a single read and write and multiple read - style
// for a single port.
//

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else // not __APPLE__
#include <dlfcn.h>
#endif

#include <signal.h>
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
	void
		free()
		{
			if(this->strings)
			{
				munmap(this->strings, this->lines * this->max_line_width);
				this->lines = 0;
				this->max_line_width = 0;
				this->strings = 0;
			}
		}
};

	internal_function void
determine_area_for_dll_filename(char *directory, string_matrix *result)
{
	result->max_line_width = 0;
	result->lines = 0;

	DIR *directory_handle = opendir(directory);
	if(directory_handle == 0){ return; }

	struct dirent *directory_info;
	while((directory_info = readdir(directory_handle)) != NULL)
	{
		s32 file_name_lenght = 0;
		{
			char *counter = directory_info->d_name;
			while(*counter++ != '\0'){ ++file_name_lenght; }
			++file_name_lenght;
		}

		b32 might_be_a_dll = file_name_lenght > 3;
		if(might_be_a_dll) //NOTE(bjorn): blah + .so\0
		{
			char *tail_pointer = directory_info->d_name;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			tail_pointer -= 3;

			b32 is_a_dll = string_matches_tag(tail_pointer, ".so");
			if(is_a_dll)
			{
				if(file_name_lenght > result->max_line_width) 
				{
					result->max_line_width = file_name_lenght;
				}
				result->lines += 1;
			}
		}
	}
	closedir(directory_handle);
}

internal_function void
all_dyn_lib_names_in_dir(char *directory, string_matrix *result)
{
	DIR *directory_handle = opendir(directory);
	if(directory_handle == 0){ return; }

	s32 file_index = 0;
	struct dirent *directory_info;
	while((directory_info = readdir(directory_handle)) != NULL)
	{
		s32 file_name_lenght = 0;
		{
			char *counter = directory_info->d_name;
			while(*counter++ != '\0'){ ++file_name_lenght; }
			++file_name_lenght;
		}

		b32 might_be_a_dll = file_name_lenght > 3;
		if(might_be_a_dll) //NOTE(bjorn): blah + .so\0
		{
			char *tail_pointer = directory_info->d_name;
			while(*tail_pointer != '\0'){ ++tail_pointer; }
			tail_pointer -= 3;

			b32 is_a_dll = string_matches_tag(tail_pointer, ".so");
			if(is_a_dll)
			{
				char *copy_pointer = directory_info->d_name;
				s32 char_index = 0;
				while(*copy_pointer != '\0')
				{ 
					*result->at(file_index, char_index++) = *copy_pointer++; 
				}
				*result->at(file_index, char_index) = '\0';

				file_index += 1;
			}
		}
	}
	closedir(directory_handle);
}

	internal_function void 
determine_area_for_api_funcs(char *path_to_file, string_matrix *result)
{
	s32 file_handle;
	{
		file_handle = open(path_to_file, O_RDONLY);
		if(file_handle == -1)
		{
			printf("open(%s)", path_to_file);
			printf("failed to open");
			return;
		}
	}

	result->max_line_width = 0;
	result->lines = 0;

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

				if(char_index+1 > result->max_line_width)
				{
					result->max_line_width = char_index + 1;
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
		result->lines = function_index;
	}
}

	internal_function void 
all_api_function_names(char *path_to_file, string_matrix *result)
{
	s32 file_handle;
	{
		file_handle = open(path_to_file, O_RDONLY);
		if(file_handle == -1)
		{
			printf("open(%s)", path_to_file);
			printf("failed to open");
			return;
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
				*result->at(function_index, char_index++) = '\0';
				function_index += 1;
				char_index = 0;
			}
			else
			{
				*result->at(function_index, char_index++) = *file_pointer;
			}
			++file_pointer;
		}
		*result->at(function_index, char_index) = '\0';
	}

	munmap(temp_file, file_stats.st_size);
}

internal_function void
load_dyn_libs(string_matrix dll_filenames, string_matrix api_funcs, 
							char *dir_of_dlls, char *dir_of_log_file, 
							connection_memory *conn_mem_temp, 
							protocol_api *protocols, s32 protocol_count)
{
	s32 protocol_index = 0; 
	for(s32 dyn_lib_file_index = 0;
			dyn_lib_file_index < dll_filenames.lines;
			++dyn_lib_file_index)
	{
		char *dyn_lib_file = dll_filenames.at(dyn_lib_file_index, 0);

		void *dyn_lib_handle;
		char absolute_path_to_dyn_lib_file[FILE_PATH_MAX_SIZE] = {};
		{
			char *copy_pointer = dir_of_dlls;
			char *dest_pointer = absolute_path_to_dyn_lib_file;
			while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
			copy_pointer = dyn_lib_file;
			while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
			*dest_pointer = '\0';

			dlerror();
			dyn_lib_handle = dlopen(absolute_path_to_dyn_lib_file, RTLD_NOW);
		}
		if(dyn_lib_handle == 0)
		{
			printf("%s\n", absolute_path_to_dyn_lib_file);
			printf("failed to load lib\n");
			fprintf(stderr, "%s\n", dlerror());
		}

		if(string_matches_tag(dyn_lib_file, "lib_unix_api"))
		{
			for(s32 api_function_index = 0;
					api_function_index < api_funcs.lines;
					++api_function_index)
			{

				conn_mem_temp->api_function_pointers[api_function_index] = 
					dlsym(dyn_lib_handle, api_funcs.at(api_function_index, 0));
			}

			conn_mem_temp->api.set_log_file(dir_of_log_file);

			conn_mem_temp->dll_handle = dyn_lib_handle;
			char *copy_pointer = absolute_path_to_dyn_lib_file;
			char *dest_pointer = conn_mem_temp->dll_path;
			while(*copy_pointer != '\0'){ *dest_pointer++ = *copy_pointer++; }
			*dest_pointer = '\0';
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

//TODO(bjorn): Do a little bit more rigorous testing to ensure that we are not
//leaking memory.
	s32 
main()
{
	char dir_of_exe[FILE_PATH_MAX_SIZE] = {};

	{
		u32 size = FILE_PATH_MAX_SIZE;
#ifdef __APPLE__
		if (_NSGetExecutablePath(dir_of_exe, &size) != 0) {
			// Buffer size is too small.
			printf("_NSGetExecutablePath() failed\n");
			return 2;
		}
#else // not __APPLE__
		if(readlink("/proc/self/exe", dir_of_exe, size) == -1)
		{
			printf("readlink(\"/proc/self/exe\") failed\n");
			return 2;
		}
#endif
		char *tail_pointer = dir_of_exe;
		while(*tail_pointer != '\0'){ ++tail_pointer; }
		while(*tail_pointer != '/'){ --tail_pointer; }
		*(tail_pointer+1) = '\0';
	}

	printf("dir_of_exe: %s\n", dir_of_exe);

	string_matrix dll_filenames = {};
	string_matrix api_funcs = {};
	char full_path_api_names_file[FILE_PATH_MAX_SIZE] = {};
	{
		determine_area_for_dll_filename(dir_of_exe, &dll_filenames);

		dll_filenames.strings = 
			(char *)mmap(0, dll_filenames.lines * dll_filenames.max_line_width, 
									 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

		all_dyn_lib_names_in_dir(dir_of_exe, &dll_filenames);

		char *api_func_names_file = "platform_api_names_in_order.txt";
		{
			s32 char_index = 0;
			char *copy_pointer = dir_of_exe;
			while(*copy_pointer != '\0')
			{ 
				full_path_api_names_file[char_index++] = *copy_pointer++; 
			}
			copy_pointer = api_func_names_file;
			while(*copy_pointer != '\0')
			{ 
				full_path_api_names_file[char_index++] = *copy_pointer++; 
			}
			full_path_api_names_file[char_index] = '\0';
		}

		determine_area_for_api_funcs(full_path_api_names_file, &api_funcs);

		api_funcs.strings = 
			(char *)mmap(0, api_funcs.lines * api_funcs.max_line_width, 
									 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

		all_api_function_names(full_path_api_names_file, &api_funcs);
	}

	connection_memory connection_memory_template = {};
	connection_memory_template.storage_size = MEMORY_PER_THREAD;

	char path_to_webroot[FILE_PATH_MAX_SIZE] = {};
	{
		char *write_destination = path_to_webroot;
		char *copy_pointer = dir_of_exe;
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }

		copy_pointer = "../webroot/";
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }
		*write_destination = '\0';
	}

	connection_memory_template.path_to_webroot = path_to_webroot;

	s32 protocol_count = 0;
	protocol_api *protocols = 0;
	{
		protocol_count = dll_filenames.lines - 1;
		protocols = (protocol_api *)mmap(0, sizeof(protocol_api)*protocol_count,
													 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	}

	char dir_of_log_file[FILE_PATH_MAX_SIZE] = {};
	{
		char *write_destination = dir_of_log_file;
		char *copy_pointer = dir_of_exe;
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }

		copy_pointer = "server.log";
		while(*copy_pointer != '\0'){ *write_destination++ = *copy_pointer++; }
		*write_destination = '\0';
	}

	load_dyn_libs(dll_filenames, api_funcs, dir_of_exe, dir_of_log_file,
								&connection_memory_template, protocols, protocol_count);

	platform_log_string *api_log_string = 
		connection_memory_template.api.log_string;
	platform_sleep_x_seconds *api_sleep_x_seconds = 
		connection_memory_template.api.sleep_x_seconds;

	api_log_string("Dynamic libraries loaded.\n");

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
			api_sleep_x_seconds(0.016f); 

			string_matrix current_dll_filenames = {};
			{
				determine_area_for_dll_filename(dir_of_exe, &current_dll_filenames);

				b32 no_dll_files = 
					(current_dll_filenames.lines * current_dll_filenames.max_line_width) == 0;
				if(no_dll_files)
				{
					continue;
				}

				current_dll_filenames.strings = 
					(char *)mmap(0, 
											 current_dll_filenames.lines * 
											 current_dll_filenames.max_line_width, 
											 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

				all_dyn_lib_names_in_dir(dir_of_exe, &current_dll_filenames);
			}

			b32 the_file_name_has_changed = true;
			{
				b32 lib_unix_api_dll_file_was_found = false;
				s32 current_lib_index;
				for(current_lib_index = 0;
						current_lib_index < current_dll_filenames.lines;
						++current_lib_index)
				{
					if(string_matches_tag(current_dll_filenames.at(current_lib_index, 0), 
																		"lib_unix_api"))
					{ 
						lib_unix_api_dll_file_was_found = true;
						break;
					}
				}


				//TODO(bjorn): Here I am saying that if the lib_unix_api.so file was
				//found then all other relevant .so files has already compiled. This
				//implies that lib_unix_api.so compiles last and puts a demand on the
				//flow of the makefile. The program could instead inspect the makefile
				//and try to figure out how many .so files compiles or maybe it should
				//just continiously search for new protocols and integrate them as they
				//pop up.

				if(!lib_unix_api_dll_file_was_found)
				{
					current_dll_filenames.free();
					continue;
				}

				s32 past_lib_index;
				for(past_lib_index = 0;
						past_lib_index < dll_filenames.lines;
						++past_lib_index)
				{
					if(string_matches_tag(dll_filenames.at(past_lib_index, 0), "lib_unix_api"))
					{ 
						break;
					}
				}


				the_file_name_has_changed = 
					!(string_matches_tag(current_dll_filenames.at(current_lib_index, 0), 
															 dll_filenames.at(past_lib_index, 0)) && 
						string_matches_tag(dll_filenames.at(past_lib_index, 0),
															 current_dll_filenames.at(current_lib_index, 0))
					 );
			}

			if(the_file_name_has_changed)
			{
				dll_filenames.free();
				dll_filenames = current_dll_filenames;

				api_funcs.free();
				{
					determine_area_for_api_funcs(full_path_api_names_file, &api_funcs);

					api_funcs.strings = 
						(char *)mmap(0, api_funcs.lines * api_funcs.max_line_width, 
												 PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

					all_api_function_names(full_path_api_names_file, &api_funcs);
				}

				connection_memory_template.api.sleep_x_seconds(1.0f);

				{
					for(s32 protocol_index = 0;
							protocol_index < protocol_count;
							++protocol_index)
					{
						dlclose(protocols[protocol_index].dll_handle);
					}
					munmap(protocols, sizeof(protocol_api) * protocol_count);
					protocols = 0;

					dlclose(connection_memory_template.dll_handle);
				}

				protocol_count = dll_filenames.lines - 1;
				protocols = (protocol_api *)mmap(0, sizeof(protocol_api)* protocol_count,
																				 PROT_READ|PROT_WRITE, 
																				 MAP_SHARED|MAP_ANONYMOUS, -1, 0);

				load_dyn_libs(dll_filenames, api_funcs, dir_of_exe, dir_of_log_file, 
											&connection_memory_template, protocols, protocol_count);

				api_log_string = connection_memory_template.api.log_string;
				api_sleep_x_seconds = connection_memory_template.api.sleep_x_seconds;

				api_log_string("Dynamic libraries reloaded.\n");
			}
			else
			{
				current_dll_filenames.free();
			}
		}
		else
		{
			//TODO(bjorn): There should maybe always be bytes here waiting. Test.
			s32 bytes_waiting;
			ioctl(client_socket_handle, FIONREAD, &bytes_waiting);

			if(bytes_waiting > 0)
			{
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

					connection_memory api_only = connection_memory_template;
					if(protocol.this_is_my_protocol(load_location, bytes_read, api_only)) 
					{
						message_was_not_handled = false;

						s32 pid = fork();

						if(pid == 0)
						{
							api_log_string("Connection started\n");

							pid = getpid();
							//NOTE(bjorn): For thread debugging purposes.
							//kill(pid, SIGSTOP);

							connection_memory memory = connection_memory_template;

							memory.storage = mmap(0, memory.storage_size, 
																		PROT_READ|PROT_WRITE, 
																		MAP_SHARED|MAP_ANONYMOUS, -1, 0);

							void *dll_handle = dlopen(memory.dll_path, RTLD_NOW);

							for(s32 protocol_index = 0;
									protocol_index < protocol_count;
									++protocol_index)
							{
								protocol_api protocol = protocols[protocol_index];

								void *dll_handle = dlopen(protocol.dll_path, RTLD_NOW);
							}

							//NOTE(bjorn): For thread debugging purposes.
							//kill(pid, SIGSTOP);

							protocol.handle_connection(client_socket_handle, pid, memory);

							//TODO(bjorn): What is the right thing to do here? Should
							//multiple protocols be able to respond to a single connection?
							//Some sort of priority value?
							//
							// In the meantime I have decided to go with one thread per
							// protocol per connection so that it is a 1-to-1 conversation
							// seen from the clients perspective.

							for(s32 protocol_index = 0;
									protocol_index < protocol_count;
									++protocol_index)
							{
								dlclose(protocols[protocol_index].dll_handle);

							}
							dlclose(memory.dll_handle);

							api_log_string("Connection ended\n");

							close(client_socket_handle);

							kill(pid, SIGKILL);
						}
						break;
					}
				}
				if(message_was_not_handled)
				{
					api_log_string("No fitting protocol was found\n");
					api_log_string("\n%.*s\n", bytes_read, load_location);

					close(client_socket_handle);
				}
			}
		}
	}
	return 0;
}
