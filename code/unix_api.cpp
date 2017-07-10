#include "connection_protocol.h"

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

//TODO(bjorn): Figure out how I want to let log_string know where to put the logs.
global_variable char *global_log_output_file;

__attribute__((constructor)) void
initialize()
{
	global_log_output_file = 0;
}

internal_function void
internal_log_string(char *format, va_list args)
{
	char *log_output_file;
	if(global_log_output_file)
	{
		log_output_file = global_log_output_file;
	}
	else
	{
		log_output_file = "server.log";
	}

	FILE *file_handle;

	s32 log_file_size = 0;
	{
		s32 unix_file_handle = open(log_output_file, O_RDONLY);
		if(unix_file_handle != -1)
		{
			struct stat file_stats;
			fstat(unix_file_handle, &file_stats);

			log_file_size = file_stats.st_size;

			close(unix_file_handle);
		}
	}

	if(log_file_size > Megabytes(1))
	{
		file_handle	= fopen(log_output_file,"w");
	}
	else
	{
		file_handle	= fopen(log_output_file,"a+");
	}


	time_t rawtime;
  time(&rawtime);
	char *str = ctime(&rawtime);
	s32 len = strlen(str);

	fprintf(file_handle, "[%d][%.*s] ", getpid(), len-1, str);

	vfprintf(file_handle, format, args);

	fclose(file_handle);
}

extern "C" PLATFORM_SET_LOG_FILE(set_log_file)
{
	global_log_output_file = path_to_log_file;
}

extern "C" PLATFORM_LOG_STRING(log_string)
{
	va_list args;
  va_start(args, format);

	internal_log_string(format, args);
}

extern "C" PLATFORM_GET_LAST_EDIT_TIMESTAMP(get_last_edit_timestamp)
{
	s32 file_handle;
	struct stat file_stats;

	file_handle = open(file_path, O_RDONLY);
	if(file_handle)
	{
		fstat(file_handle, &file_stats);

		return file_stats.st_mtime;
	}
	else
	{
		log_string("get_last_edit_timestamp()\n");
		log_string("Could not find file [");
		log_string(file_path);
		log_string("]\n");
		return 0;
	}
}

extern "C" PLATFORM_OPEN_FILE(open_file)
{
	loaded_file result = {};

	s32 file_handle;
	struct stat file_stats;

	file_handle = open(file_path, O_RDONLY);
	if(file_handle != -1)
	{
		fstat(file_handle, &file_stats);
		result.total_file_size = file_stats.st_size;
		result.content = load_location;
		result.file_path = file_path;

		if(result.total_file_size <= maximum_page_size)
		{
			result.content_size = result.total_file_size;
		}
		else
		{
			result.there_is_more_content = true;

			result.content_size = maximum_page_size;
		}
		void *temp_file = mmap(0, result.content_size, 
													 PROT_READ, MAP_PRIVATE, file_handle, 0);
		memcpy(result.content, temp_file, result.content_size);
		munmap(temp_file, result.content_size);

		return result;
	}
	else
	{
		log_string("open_file()\n");
		log_string("Could not find file [");
		log_string(file_path);
		log_string("]\n");
		return result;
	}
}

extern "C" PLATFORM_GET_NEXT_PART_OF_FILE(get_next_part_of_file)
{
	loaded_file result = file;
	if(!result.there_is_more_content)
	{
		log_string("get_next_part_of_file()\n");
		log_string("Redundant call\n");
		return result;
	}

	s32 file_handle;
	struct stat file_stats;

	file_handle = open(result.file_path, O_RDONLY);
	if(file_handle != -1)
	{
		s32 bytes_read = (result.page_number + 1) * result.content_size;
		s32 bytes_left = result.total_file_size - bytes_read;

		s32 unix_page_size = sysconf(_SC_PAGE_SIZE);

		s32 number_of_offset_tables = bytes_read / unix_page_size;
		s32 offset_bytes = bytes_read - (unix_page_size * number_of_offset_tables);

		if(bytes_left <= result.content_size)
		{
			result.there_is_more_content = false;
			result.content_size = bytes_left;
		}
		else
		{
			result.content_size = file.content_size;
		}
		result.page_number += 1;

		void *temp_file = mmap(0, offset_bytes + result.content_size, 
													 PROT_READ, MAP_PRIVATE, file_handle, 
													 number_of_offset_tables * unix_page_size);
		memcpy(result.content, 
					 (void *)((u8*)temp_file+offset_bytes), 
					 result.content_size);
		munmap(temp_file, offset_bytes + result.content_size);

		return result;
	}
	else
	{
		log_string("get_next_part_of_file()\n");
		log_string("Could not find file [%s]\n", result.file_path);
		return result;
	}
}

extern "C" PLATFORM_BYTES_IN_CONNECTION_QUEUE(bytes_in_connection_queue)
{
	s32 bytes_in_queue;
	ioctl(connection_id, FIONREAD, &bytes_in_queue);

	if(bytes_in_queue > 0)
	{
		return bytes_in_queue;
	}
	else
	{
		return 0;
	}
}

extern "C" PLATFORM_SLEEP_X_SECONDS(sleep_x_seconds)
{
	usleep((useconds_t)(1000.0f * 1000.0f * seconds));
}

internal_function void 
append_to_string(char *string, char *appendix)
{
	while(*string != '\0'){ string++; }
	while(*appendix != '\0'){ *string++ = *appendix++; }
	*string = '\0';
}

extern "C" PLATFORM_EXECUTE_SHELL_COMMAND(execute_shell_command)
{
	append_to_string(script, " >> ");
	append_to_string(script, global_log_output_file);

	log_string("%s\n", script);

	s32 status_code = system(script);
	if(status_code == 0)
	{
		return true;
	}

	log_string("execute_shell_command()\n");
	if(status_code == 127)
	{
		log_string("Could not find/run /bin/sh\n");
	}
	else
	{
		log_string("The command [");
		log_string(script);
		log_string("] returned in error\n");
	}
	return false;
}

extern "C" PLATFORM_WRITE_TO_CONNECTION(write_to_connection)
{
	write(connection_id, source, bytes_to_send);
}

extern "C" PLATFORM_READ_FROM_CONNECTION(read_from_connection)
{
	s32 bytes_read = recv(connection_id, load_location, bytes_to_read, 0);
	if(bytes_read < bytes_to_read)
	{
		log_string("get_next_part_of_file()\n");
		log_string("less bytes where collected than expected\n");
	}
}

extern "C" PLATFORM_PAUSE_THREAD(pause_thread)
{
	kill(getpid(), SIGSTOP);
}

extern "C" PLATFORM_KILL_THREAD(kill_thread)
{
	kill(getpid(), SIGKILL);
}

extern "C" PLATFORM_ASSERT(assert)
{
	if(!statement)
	{
		va_list args;
		va_start(args, format);

		internal_log_string(format, args);

		kill_thread();
	}
}
