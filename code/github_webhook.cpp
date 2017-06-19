#include "connection_protocol.h"

internal_function b32
they_dont_differ(char *string, char *tag)
{
	while(*tag != '\0')
	{
		if(*tag++ - *string++)
		{
			return false;
		}
	}
	return *tag == *string;
}

SERVER_THIS_IS_MY_PROTOCOL(this_is_my_protocol)
{
	return they_dont_differ(url, "/github-push-event");
}
