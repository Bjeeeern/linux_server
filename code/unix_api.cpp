#include "connection_protocol.h"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>


/*
 * TODO
 * API
 *
 * os to protocol
 *
 * open_file (max 512 kb)
 * next_part_of_file
 *	struct
 *		pointer
 *		size
 *		page
 *		max_size
 * get_file_last_edited
 *		64u seconds since origin
 * bytes_in_connection_buffer
 *		32u (if larger than 256 kb, send error too large request)
 * read_from_connection
 * write_to_connection
 * sleep_x_seconds
 * run_bash_script
 *	(will work on windows aswell!)
 *
 * protocol to os
 *
 * (LATER is_tcp)
 * protocol_main_thread
 *	connection_id
 *	1 mb memory in, no leaks 100%
 * this_is_my_protocol
 *	read 1kb without pulling from tcpbuffer and 
 *	1kb url and judge it
 *	(http://seyama.se/github-push-event)
 *
 */

#include <stdio.h>
PLATFORM_LOG_STRING(log_string)
{
	printf(message);
}

PLATFORM_GET_LAST_EDIT_TIMESTAMP(get_last_edit_timestamp)
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

PLATFORM_OPEN_FILE(open_file)
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

PLATFORM_GET_NEXT_PART_OF_FILE(get_next_part_of_file)
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
		log_string("Could not find file [");
		log_string(result.file_path);
		log_string("]\n");
		return result;
	}
}

PLATFORM_BYTES_IN_CONNECTION_QUEUE(bytes_in_connection_queue)
{
	s32 bytes_in_queue;
	ioctl(connection_id, FIONREAD, &bytes_in_queue);
	return bytes_in_queue;
}

PLATFORM_SLEEP_X_SECONDS(sleep_x_seconds)
{
	usleep((useconds_t)(1000.0f * 1000.0f * seconds));
}

PLATFORM_EXECUTE_SHELL_COMMAND(execute_shell_command)
{
	s32 status_code = system(script);
	if(status_code == 0)
	{
		return true;
	}

	log_string("get_next_part_of_file()\n");
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

PLATFORM_WRITE_TO_CONNECTION(write_to_connection)
{
	write(connection_id, source, bytes_to_send);
}

PLATFORM_READ_FROM_CONNECTION(read_from_connection)
{
	s32 bytes_read = recv(connection_id, load_location, bytes_to_read, 0);
	if(bytes_read < bytes_to_read)
	{
		log_string("get_next_part_of_file()\n");
		log_string("less bytes where collected than expected\n");
	}
}

