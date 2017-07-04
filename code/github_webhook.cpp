#include "connection_protocol.h"

internal_function b32
they_dont_differ(char *string, char *tag)
{
	while(*tag != '\0')
	{
		if(*string == '\0') { return false; }
		if(*tag++ - *string++)
		{
			return false;
		}
	}
	return true;
}

extern "C" SERVER_THIS_IS_MY_PROTOCOL(this_is_my_protocol)
{
	if(content_sniff_size >= 23)
	{
		return they_dont_differ((char *)content_sniff, "POST /github-push-event");
	}
	return false;
}

internal_function void 
append_to_string(char *string, char *appendix)
{
	while(*string != '\0'){ string++; }
	while(*appendix != '\0'){ *string++ = *appendix++; }
	*string = '\0';
}

extern "C" SERVER_HANDLE_CONNECTION(handle_connection)
{
	char *command = (char *)memory.storage;
	append_to_string(command, "cd ");
	append_to_string(command, memory.path_to_webroot);
	append_to_string(command, "../code && git pull && make build");
	memory.api.execute_shell_command(command);
  return;
}
