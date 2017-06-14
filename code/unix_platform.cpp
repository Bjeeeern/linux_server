#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include "handmade.h"

#define BUFFER_SIZE Kilobytes(64)
#define SIMULTANEOUS_CONNECTIONS 16

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
they_dont_differ(char *string, const char *tag)
{
	while(*tag != '\0')
	{
		s32 diff = *tag++ - *string++;
		if(diff != 0)
		{
			return false;
		}
	}
	return true;
}
internal_function s32
they_differ(char *string, const char *tag)
{
	return !they_dont_differ(string, tag);
}

internal_function s32
get_line_nr_at_tag(char *string, const char *tag)
{
	s32 line_nr = 0;
	while(*string != '\0')
	{
		if(*string == *tag)
		{
			if(they_dont_differ(string, tag))
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
	while(they_differ(string + start_index++, pre)){}
	start_index += string_size(pre) - 1;

	s32 end_index = start_index;
	while(they_differ(string + end_index++, post)){}
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
	while(they_differ(string, tag))
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
handle_request_through_fork(s32 client_socket_handle)
{
	s32 pid = getpid();

KEEP_ALIVE:
	s32 bytes_in_queue = 0;
	f32 seconds_waited = 0.0f;
	while(bytes_in_queue == 0)
	{
		ioctl(client_socket_handle, FIONREAD, &bytes_in_queue);
		if(bytes_in_queue == 0)
		{
			usleep(16000);
			seconds_waited += 0.0167;
			if(seconds_waited > 10.0f)
			{
				return;
			}
		}
	}

	u8* data_received = (u8*)mmap(0, bytes_in_queue + 1,
																PROT_READ | PROT_WRITE,
																MAP_PRIVATE | MAP_ANONYMOUS,
																-1, 0);

	s32 bytes_received = recv(client_socket_handle, (void *)data_received, 
												bytes_in_queue, 0);
	//NOTE(bjorn): Make it a string so i can use it with my string functions.
	data_received[bytes_in_queue] = '\0';

	Assert(bytes_in_queue == bytes_received);

	printf("\t[----received message\n%.*s\n\t[----\n\tpid: %i\n\t[----\n", 
				 bytes_received, data_received, pid);


	char path_to_executable[1024] = {};
	getcwd(path_to_executable, sizeof(path_to_executable));

	append_to_string(path_to_executable, "/../webroot");
	char *absolute_path_to_webroot = path_to_executable;

	if(get_line_nr_at_tag((char *)data_received, "GET") == 0)
	{
		char *GET_request_path = get_string_between_tags((char *)data_received, 
																										 "GET ", " ");

		append_to_string(absolute_path_to_webroot, GET_request_path);
		if( ! string_does_contain_tag(GET_request_path, "."))
		{
			append_to_string(absolute_path_to_webroot, "index.html");
		}
		munmap(GET_request_path, string_size(GET_request_path)+1);

		char *absolute_request_path = absolute_path_to_webroot;


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

			char *size_in_text = int_to_string(size_of_file);
			append_to_string(GET_response, size_in_text);
			munmap(size_in_text, string_size(size_in_text)+1);

			append_to_string(GET_response, "\n");

			if(string_does_contain_tag((char*)data_received, "keep-alive"))
			{
				append_to_string(GET_response, "Connection: keep-alive\n");
			}
			else
			{
				append_to_string(GET_response, "Connection: close\n");
			}

		  printf("\tAbsolute request path: %s\n\t[----\n", absolute_request_path);
			if(string_does_contain_tag(absolute_request_path, ".html"))
			{
				append_to_string(GET_response, "Content-Type: text/html; charset=utf-8\n");
			}
			else if(string_does_contain_tag(absolute_request_path, ".ico"))
			{
				append_to_string(GET_response, "Content-Type: image/ico\n");
			}
			else if(string_does_contain_tag(absolute_request_path, ".css"))
			{
				append_to_string(GET_response, "Content-Type: text/css; charset=utf-8\n");
			}
			else if(string_does_contain_tag(absolute_request_path, ".js"))
			{
				append_to_string(GET_response, 
												 "Content-Type: application/javascript; charset=utf-8\n");
			}
			else
			{
				//TODO(bjorn): Log this.
			}
			append_to_string(GET_response, "\n");

			s32 size_of_response_header = string_size(GET_response);

			write(client_socket_handle, GET_response, size_of_response_header);

			printf("\tGET response:\n%s\n", GET_response);
			if(string_does_contain_tag(absolute_request_path, ".html"))
			{
				printf("\tGET response html content:\n%.*s\n", size_of_file, file);
			}

			write(client_socket_handle, file, size_of_file);

			close(file_handle);

			munmap(file, size_of_file);
		}
		else
		{
			//TODO(bjorn): 404 file not found!
		}

		if(string_does_contain_tag((char*)data_received, "keep-alive"))
		{
			munmap(data_received, bytes_in_queue + 1);
			goto KEEP_ALIVE;
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
				if(listen(SocketHandle, SIMULTANEOUS_CONNECTIONS) == 0)
				{
					while(true)
					{
						struct sockaddr_in ClientAddress = {};
						socklen_t ClientAddressLenght = sizeof(ClientAddress);

						s32 client_socket_handle = accept(SocketHandle, 
																							(struct sockaddr *)&ClientAddress, 
																							&ClientAddressLenght);
						if(client_socket_handle == -1)
						{
							usleep(16000);
						}
						else
						{
							//TODO(bjorn): There should maybe always be bytes here waiting.
							s32 bytes_waiting = 0;
							ioctl(client_socket_handle, FIONREAD, &bytes_waiting);

							if(bytes_waiting > 0)
							{
								s32 pid = fork();

								if(pid == 0)
								{
									//NOTE(bjorn): For thread debugging purposes.
									//kill(0, SIGSTOP);

									handle_request_through_fork(client_socket_handle);

									close(client_socket_handle);
									printf("Connection ended correctly\n");

									_exit(0);
								}
							}
						}
					}
				}
				else
				{
					//TODO(bjorn): Log this.
					printf("Can not listen to socket for simultaneous connections.\n");
				}
			}
			else
			{
				//TODO(bjorn): Log this.
				printf("Can not bind socket properly.\n");
			}
		}
		else
		{
			//TODO(bjorn): Log this.
			printf("Can not set socket flags properly.\n");
		}
	}
	else
	{
		//TODO(bjorn): Log this.
		printf("Can not create an Inet socket.\n");
	}

	return 0;
}
