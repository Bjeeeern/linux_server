#include "connection_protocol.h"

internal_function b32
they_dont_differ(char *string, char *tag)
{
	while(*tag != '\0')
	{
		if(*string == '\0') { return false; }
		if(*tag++ != *string++)
		{
			return false;
		}
	}
	return true;
}

extern "C" SERVER_THIS_IS_MY_PROTOCOL(this_is_my_protocol)
{
	char *string = (char *)content_sniff;
	//NOTE(bjorn): This is just for simple HTTP.
	while(*string != ' ')
	{ 
		if(*string == '\0')
		{
			return false;
		}
		++string;
	}
	++string;

	while(*string != ' ')
	{ 
		if(*string == '\0')
		{
			return false;
		}
		++string;
	}
	++string;

	return they_dont_differ(string, "HTTP");
}

internal_function b32
http_is(char *header, char *method)
{
	while(*method != '\0'){ if(*header++ != *method++) return false; }
	return true;
}

internal_function b32
loose_fit(char *header, char *tag)
{
	while(*tag != '\0')
	{ 
		if(*header != *tag) 
		{
			if(!(
					 (*header - 'A' == *tag - 'a') ||
					 (*header - 'a' == *tag - 'A')
					 ))
			{
				return false; 
			}
		}

		++header;
		++tag;
	}
	return true;
}

internal_function b32
header_field_is(char *header, char *field, char *value)
{
	while(true)
	{
		while(!loose_fit(header, "\r\n")){ ++header; }
		header += 2;

		if(loose_fit(header, field))
		{
			while(*header++ != ' '){}

			return loose_fit(header, value);
		}

		if(loose_fit(header, "\r\n"))
		{
			return false;
		}
	}
}

internal_function b32 
path_is(char *header, char *path)
{
	while(*header++ != ' '){}
	return loose_fit(header, path);
}

internal_function void 
append_to_string(char *string, char *appendix)
{
	while(*string != '\0'){ string++; }
	while(*appendix != '\0'){ *string++ = *appendix++; }
	*string = '\0';
}

internal_function b32
not_end_of_url(char c)
{
	return !( c == ' ' || c == '?' || c == '#' );
}

internal_function b32
path_is_a_directory(char *header)
{
	while(*header++ != ' '){}
	while(not_end_of_url(header[0]))
	{
		++header;
	}
	return header[-1] == '/';
}

internal_function b32
file_is_blank(char *header)
{
	while(*header++ != ' '){}
	while(not_end_of_url(header[0]))
	{
		++header;
	}
	while(header[0] != '/')
	{
		--header;
	}

	while(not_end_of_url(header[0]))
	{
		++header;

		if(header[0] == '.')
		{
			return false;
		}
	}
	return true;
}

internal_function void
append_path_to_string(char *string, char *header)
{
	while(*header++ != ' '){}
	++header;

	char *copy_pointer = header;
	char *dest_pointer = string;
	while(*dest_pointer != '\0'){ ++dest_pointer; }
	while(not_end_of_url(copy_pointer[0])){ *dest_pointer++ = *copy_pointer++; }
}

//TODO(bjorn): Make this function cleaner written.
internal_function b32
path_containts_either_file_ending(char *header, char *file_formats)
{
	//NOTE(bjorn): File-format are listed as such: ".html .js .css .ico .png .jpg".
	while(*header++ != ' '){}
	while(not_end_of_url(*header)){ ++header; }
	while(*header != '.'){ --header; }
	++header;

	while(*file_formats++ != '.'){ }

	while(true)
	{
		b32 they_are_the_same = true;
		{
			char *header_file_type = header;
			char *test_file_type = file_formats;
			while(!(*test_file_type == ' ' || *test_file_type == '\0'))
			{ 
				if(*test_file_type != *header_file_type)
				{
					they_are_the_same = false;
					break;
				}
				++header_file_type;
				++test_file_type;
			}
		}

		if(they_are_the_same)
		{
			return true;
		}

		while(!(*file_formats == '.' || *file_formats == '\0')) { ++file_formats; }

		if(*file_formats == '\0'){ return false; }
		else { ++file_formats; }
	}
	return false;
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

struct static_memory
{
	char path[FILE_PATH_MAX_SIZE];
};
		
extern "C" SERVER_HANDLE_CONNECTION(handle_connection)
{
	//memory.api.pause_thread();

	//TODO(bjorn): The os should provide a memwipe function and give the memory
	//pre-wiped to the process.
	u8 *eraser = (u8 *)memory.storage;
	for(s32 eraser_index = 0;
				eraser_index < memory.storage_size;
				++eraser_index)
	{
		eraser[eraser_index] = 0;
	}

	static_memory *stat_mem = (static_memory*)memory.storage;

	void *data_out = (void *)(((u8*)memory.storage) + memory.storage_size/2);
	s32 data_out_size = memory.storage_size/2;

	void *data_in = (void *)&(stat_mem[1]);
	s32 data_in_size = memory.storage_size/2 - sizeof(static_memory);

	b32 wait_for_response = true;
	f32 seconds_waited = 0.0f;
	s32 lives_left = 4;
	while(wait_for_response)
	{
		s32 bytes_in_queue = memory.api.bytes_in_connection_queue(connection_id);
		while(!bytes_in_queue)
		{
			memory.api.sleep_x_seconds(1.0f/60.0f);
			seconds_waited += 1.0f/60.0f;

			if(seconds_waited > 5.0f)
			{
				//TODO(bjorn): Respond with 408 Request Timeout before quitting.
				return;
			}

			bytes_in_queue = memory.api.bytes_in_connection_queue(connection_id);
		}

		if(bytes_in_queue > data_in_size)
		{
			//TODO(bjorn): Send back error 431 Request Header Fields Too Large OR 
			//														 413 Payload Too Large.
			return;
		}
		memory.api.read_from_connection(connection_id, data_in, bytes_in_queue);

		char *header = (char*)data_in;
		memory.api.log_string("\n%.*s\n", bytes_in_queue, header);

		if(header_field_is(header, "connection", "keep-alive"))
		{
			wait_for_response = true;
		}
		else
		{
			wait_for_response = false;
		}

		if(path_is(header, "/github-push-event"))
		{
			char *command = stat_mem->path;
			command[0] = '\0';
			append_to_string(command, "cd ");
			append_to_string(command, memory.path_to_webroot);
			append_to_string(command, "../code && git pull ");
			memory.api.execute_shell_command(command);

			command[0] = '\0';
			append_to_string(command, "cd ");
			append_to_string(command, memory.path_to_webroot);
			append_to_string(command, "../code && make --silent build");
			memory.api.execute_shell_command(command);
			return;
		}

		if(http_is(header, "GET"))
		{
			append_to_string(stat_mem->path, memory.path_to_webroot);

			if(path_is_a_directory(header))
			{
				append_path_to_string(stat_mem->path, header);
				append_to_string(stat_mem->path, "index.html");
			}
			else if(path_containts_either_file_ending(header, ".html .js .css .ico "
																								".png .jpg .jpeg .gif .json "
																								".unityweb .ttf .otf .woff"))
			{
				append_path_to_string(stat_mem->path, header);
			}
			else if(file_is_blank(header))
			{
				append_path_to_string(stat_mem->path, header);
			}
			else
			{
				//TODO(bjorn): Respond with a 404 file not found page.
				return;
			}

			char header_out[1024] = {};

			loaded_file file = memory.api.open_file(stat_mem->path, 
																							data_out_size, data_out);

			if(file.total_file_size)
			{
				append_to_string(header_out, 
												 "HTTP/1.1 200 OK\r\n"
												 "Content-Length: ");

				{
					char size_in_text[256]; 
					int_to_string(file.total_file_size, size_in_text);
					append_to_string(header_out, size_in_text);
				}

				append_to_string(header_out, "\r\n");

				if(header_field_is(header, "connection", "keep-alive") && lives_left)
				{
					append_to_string(header_out, "Connection: keep-alive\r\n");
					--lives_left;
				}
				else
				{
					append_to_string(header_out, "Connection: close\r\n");
				}

				if(path_containts_either_file_ending(header, ".html") ||
					 path_is_a_directory(header))
				{
					append_to_string(header_out, "Content-Type: text/html; charset=utf-8\r\n");
				}
				else if(path_containts_either_file_ending(header, ".ico"))
				{
					append_to_string(header_out, "Content-Type: image/ico\r\n");
				}
				else if(path_containts_either_file_ending(header, ".css"))
				{
					append_to_string(header_out, "Content-Type: text/css; charset=utf-8\r\n");
				}
				else if(path_containts_either_file_ending(header, ".js"))
				{
					append_to_string(header_out, 
													 "Content-Type: application/javascript; charset=utf-8\r\n");
				}
				else if(path_containts_either_file_ending(header, ".png"))
				{
					append_to_string(header_out, "Content-Type: image/png\r\n");
				}
				else if(path_containts_either_file_ending(header, ".jpg .jpeg"))
				{
					append_to_string(header_out, "Content-Type: image/jpeg\r\n");
				}
				else if(path_containts_either_file_ending(header, ".gif"))
				{
					append_to_string(header_out, "Content-Type: image/gif\r\n");
				}
				else if(path_containts_either_file_ending(header, ".json"))
				{
					append_to_string(header_out, "Content-Type: application/json\r\n");
				}
				else if(path_containts_either_file_ending(header, ".unityweb"))
				{
					append_to_string(header_out, "Content-Type: application/vnd.unity\r\n");
				}
				else if(path_containts_either_file_ending(header, ".otf"))
				{
					append_to_string(header_out, 
													 "Content-Type: application/x-font-opentype\r\n");
				}
				else if(path_containts_either_file_ending(header, ".ttf"))
				{
					append_to_string(header_out, 
													 "Content-Type: application/x-font-truetype\r\n");
				}
				else if(path_containts_either_file_ending(header, ".woff"))
				{
					append_to_string(header_out, "Content-Type: application/font-woff\r\n");
				}
				else if(file_is_blank(header))
				{
				}
				else
				{
					memory.api.assert(false, "This path should never be reached.");
				}
				append_to_string(header_out, "\r\n");

				s32 header_out_size = string_size(header_out);

				memory.api.write_to_connection(connection_id, 
																			 (void *)header_out, header_out_size); 

				memory.api.write_to_connection(connection_id, 
																			 file.content, file.content_size); 

				{
					memory.api.log_string("\n%s\n", header_out);
					if(path_containts_either_file_ending(header, ".html .css .js") ||
						 path_is_a_directory(header))
					{
						memory.api.log_string("\n%.*s\n", 
																	256, (char *)file.content);
					}
				}

				while(file.there_is_more_content)
				{
					file = memory.api.get_next_part_of_file(file);

					memory.api.write_to_connection(connection_id, 
																				 file.content, file.content_size); 
				}
			}
			else
			{
				//TODO(bjorn): 404 File Not Found.
			}
		}
		else if(http_is(header, "POST"))
		{
		}
		else
		{
			//TODO(bjorn): Respond with 500 Internal Server Error or something.
		}

		u8 *eraser = (u8 *)memory.storage;
		for(s32 eraser_index = 0;
				eraser_index < memory.storage_size;
				++eraser_index)
		{
			eraser[eraser_index] = 0;
		}
	}
}

