#if !defined(CONNECTION_PROTOCOL_H) 
/* NOTE(bjorn):

   :HANDMADE_INTERNAL:
   0 - Build for public release. 
   1 - Build for developer only.

   :HANDMADE_SLOW:
   0 - No slow code allowed!
   1 - Slow code welcome.
 */

#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef u32 b32;

// TODO(bjorn): Implement sine on my own.
//#include <math.h>
#define Pi32 3.14159265359f

#define internal_function static 
#define local_persist static 
#define global_variable static 

#define Kilobytes(number) (number * 1024LL)
#define Megabytes(number) (Kilobytes(number) * 1024LL)
#define Gigabytes(number) (Megabytes(number) * 1024LL)
#define Terabytes(number) (Gigabytes(number) * 1024LL)

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

// TODO(bjorn): swap, min, max...   macros???

//
// NOTE(bjorn): Stuff the platform provides to the server.
//
#define PLATFORM_LOG_STRING(name) void name(char *message)
typedef PLATFORM_LOG_STRING(platform_log_string);

#if HANDMADE_SLOW
#define assert(expression, message) if(!(expression)){ platform_log_string(message); }
#else
#define assert(expression, message)
#endif


struct loaded_file
{
	char *file_path;
	s32 total_file_size;
  s32 content_size;
  void *content;
	s32 page_number;
	b32 there_is_more_content;
};
#define PLATFORM_OPEN_FILE(name) loaded_file name(char *file_path, \
																									u32 maximum_page_size, \
																									void *load_location)
typedef PLATFORM_OPEN_FILE(platform_open_file);

#define PLATFORM_GET_NEXT_PART_OF_FILE(name) loaded_file name(loaded_file file)
typedef PLATFORM_GET_NEXT_PART_OF_FILE(platform_get_next_part_of_file);

#define PLATFORM_GET_LAST_EDIT_TIMESTAMP(name) u64 name(char *file_path)
typedef PLATFORM_GET_LAST_EDIT_TIMESTAMP(platform_get_last_edit_timestamp);

#define PLATFORM_BYTES_IN_CONNECTION_QUEUE(name) s32 name(s32 connection_id)
typedef PLATFORM_BYTES_IN_CONNECTION_QUEUE(platform_bytes_in_connection_queue);

#define PLATFORM_READ_FROM_CONNECTION(name) void name(s32 connection_id, \
																								 void *load_location, \
																								 s32 bytes_to_read)
typedef PLATFORM_READ_FROM_CONNECTION(platform_read_from_connection);

#define PLATFORM_WRITE_TO_CONNECTION(name) void name(s32 connection_id, \
																										 void *source, \
																										 s32 bytes_to_send)
typedef PLATFORM_WRITE_TO_CONNECTION(platform_write_to_connection);

#define PLATFORM_SLEEP_X_SECONDS(name) void name(f32 seconds)
typedef PLATFORM_SLEEP_X_SECONDS(platform_sleep_x_seconds);

#define PLATFORM_EXECUTE_SHELL_COMMAND(name) b32 name(char *script)
typedef PLATFORM_EXECUTE_SHELL_COMMAND(platform_execute_shell_command);

//
// NOTE(bjorn): Stuff the server provides to the platform.
//

// TODO(bjorn): what_protocol()

struct platform_api
{
	platform_log_string									log_string;
	platform_open_file									open_file;
	platform_get_next_part_of_file			get_next_part_of_file;
	platform_get_last_edit_timestamp		get_last_edit_timestamp;
	platform_bytes_in_connection_queue	bytes_in_connection_queue;
	platform_read_from_connection				read_from_connection;
	platform_write_to_connection				write_to_connection;
	platform_sleep_x_seconds						sleep_x_seconds;
	platform_execute_shell_command			execute_shell_command;
};
struct connection_memory
{
	s32 storage_size;
	void *storage;
	platform_api api;
};

#define SERVER_HANDLE_CONNECTION(name) void name(s32 connection_id, \
																								 s32 pid, \
																								 connection_memory memory)
typedef SERVER_HANDLE_CONNECTION(server_handle_connection);
SERVER_HANDLE_CONNECTION(handle_connection_stub)
{
  return;
}

#define SERVER_THIS_IS_MY_PROTOCOL(name) b32 name(void *content_sniff, \
																									s32 content_sniff_size, \
																									char *url, \
																									u32 tcp_protocol_number)
typedef SERVER_THIS_IS_MY_PROTOCOL(server_this_is_my_protocol);
SERVER_THIS_IS_MY_PROTOCOL(this_is_my_protocol_stub)
{
	return false;
}

#define CONNECTION_PROTOCOL_H
#endif
