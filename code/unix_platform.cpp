#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include "handmade.h"

#define BUFFER_SIZE Kilobytes(64)

/*
	s32 path_index = 0;
	while(path_to_executable[path_index++] != '\0'){}

	const char *relative_path_to_webroot = "../webroot";
	char *absolute_path_to_webroot = path_to_executable;

	s32 rel_index = 0;
	while(relative_path_to_webroot[rel_index] != '\0')
	{
		absolute_path_to_webroot[path_index++] = absolute_path_to_webroot[rel_index++];
	}

	u8* index_html = (u8*)mmap(0, bytes_received + 1,
																						PROT_READ | PROT_WRITE,
																						MAP_PRIVATE | MAP_ANONYMOUS,
																						-1, 0); 
*/
internal_function void append_to_string(char *string, char *appendix)
{
	while(*string++ != '\0'){}
	--string;
	while(*appendix != '\0')
	{
		*string++ = *appendix++;
	}
	*string = '\0';
}
internal_function void
append_to_string(char *string, const char *appendix)
{
	append_to_string(string, (char*) appendix);
}

internal_function void
append_data_to_header(char *string, void *file, s32 size_of_file)
{
	while(*string++ != '\0'){}
	--string;

	s32 copy_index = 0;
	while(copy_index != size_of_file)
	{
		*string++ = ((char *)file)[copy_index++];
	}
}

internal_function s32
diff_with_tag(char *string, const char *tag)
{
	s32 diff = 0;
	while(*tag != '\0')
	{
		diff += *tag++ - *string++;
	}
	return diff;
}

internal_function s32
get_line_nr_at_tag(char *string, const char *tag)
{
	s32 line_nr = 0;
	while(*string != '\0')
	{
		if(*string == *tag)
		{
			s32 diff = diff_with_tag(string, tag);
			if(diff == 0)
			{
				return line_nr;
			}
		}
		if(*string == '\n')
		{
			line_nr += 1;
		}
		++string;
	}
	return -1;
}

internal_function s32
string_size(char *string)
{
	s32 size_of_string = 0;
	while(*string++ != '\0')
	{
		size_of_string += 1;
	}
	return size_of_string;
}
internal_function s32
string_size(const char *string)
{
	return string_size((char*)string);
}

internal_function char *
get_string_between_tags(char *string, const char *pre, const char *post)
{
	s32 start_index = 0;
	while(diff_with_tag(string + start_index++, pre) != 0){}
	start_index += string_size(pre) - 1;

	s32 end_index = start_index;
	while(diff_with_tag(string + end_index++, post) != 0){}
	end_index -= 1;

	s32 size_of_new_string = end_index - start_index;
	char *new_string = (char *)mmap(0, size_of_new_string + 1,
														 PROT_READ | PROT_WRITE,
														 MAP_PRIVATE | MAP_ANONYMOUS,
														 -1, 0); 

	s32 copy_index = start_index;
	s32 new_string_index = 0;
	while(copy_index != end_index)
	{
		new_string[new_string_index++] = string[copy_index++];
	}
	new_string[new_string_index] = '\0';

	return new_string;
}

internal_function b32
string_does_contain_tag(char *string, const char *tag)
{
	while(diff_with_tag(string, tag) != 0)
	{
		if(*string++ == '\0')
		{
			return false;
		}
	}
	return true;
}
		
internal_function char *
int_to_string(s32 number)
{
	if(number == 0)
	{
		return (char *)"0";
	}

	char buffer[256] = {};
	s32 size_of_string = 0;

	while(number != 0) 
	{
		s32 dividend = number / 10;
		s32 rest = number - (dividend * 10);
		buffer[size_of_string] = '0' + (char)rest;

		number /= 10;
		size_of_string += 1;
	}

	char *string = (char *)mmap(0, size_of_string + 1,
															PROT_READ | PROT_WRITE,
															MAP_PRIVATE | MAP_ANONYMOUS,
															-1, 0); 
	string[size_of_string] = '\0';

	s32 copy_index = size_of_string - 1;
	s32 buffer_index = 0;
	while(copy_index != -1)
	{
		string[copy_index--] = buffer[buffer_index++];
	}
	return string;
}

internal_function void
handle_request_through_fork(s32 client_socket_handle, void *data_received, s32 length_of_data)
{
	char path_to_executable[1024] = {};
	getcwd(path_to_executable, sizeof(path_to_executable));

	printf("Current working dir: %s\n", path_to_executable);

	append_to_string(path_to_executable, "/../webroot");
	char *absolute_path_to_webroot = path_to_executable;

	printf("Absolute path to webroot: %s\n", absolute_path_to_webroot);

	if(get_line_nr_at_tag((char *)data_received, "GET") == 0)
	{
		char *GET_request_path = get_string_between_tags((char *)data_received, 
																										 "GET ", " ");

		append_to_string(absolute_path_to_webroot, GET_request_path);
		if( ! string_does_contain_tag(GET_request_path, "."))
		{
			append_to_string(absolute_path_to_webroot, "index.html");
		}

		char *absolute_request_path = absolute_path_to_webroot;
		printf("Absolute request path: %s\n", absolute_request_path);

		//load the file into memor
		s32 file_handle;
		struct stat file_stats;

		file_handle = open(absolute_request_path, O_RDONLY);
		fstat(file_handle, &file_stats);

		s32 size_of_file = file_stats.st_size;
		void *file = mmap(0, size_of_file, PROT_READ, MAP_PRIVATE, file_handle, 0);

		if(file)
		{
			char GET_response[1024] = 
				"HTTP/1.1 200 OK\n"
				"Content-Length: ";

			append_to_string(GET_response, int_to_string(size_of_file));
			append_to_string(GET_response, "\n");

			if(string_does_contain_tag(absolute_request_path, ".html"))
			{
				append_to_string(GET_response, "Content-Type: text/html; charset=utf-8\n");
			}
			else if(string_does_contain_tag(absolute_request_path, ".ico"))
			{
				printf("favicon.ico sent!");
			}
			else
			{
				//TODO(bjorn): Log this.
			}
			append_to_string(GET_response, "\n");

			s32 size_of_response_header = 0;
			while(GET_response[size_of_response_header++] != 0){}
			size_of_response_header -= 1;

			append_data_to_header(GET_response, file, size_of_file);

			if(string_does_contain_tag(absolute_request_path, ".html"))
			{
				printf("GET response: %s\n", GET_response);
			}
			write(client_socket_handle, GET_response, size_of_response_header + size_of_file);
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
								s32 pid = fork();
								//NOTE(bjorn): For thread debugging purposes.
								//kill(0, SIGSTOP);

								if(pid)
								{
									printf("[-[-[\n%s]-]-]\npid: %i\n", buffer, pid);
								}
								else
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

									handle_request_through_fork(client_socket_handle, 
																							(void *)data_recieved_from_client, 
																							bytes_received + 1);

									// free(data_recieved_from_client);
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
