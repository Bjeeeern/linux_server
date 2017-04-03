#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "handmade.h"

s32 main()
{
  int FileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if(FileDescriptor != -1)
  {
    struct sockaddr_in Address = {};
    //TODO(bjorn): Read this from a settings text file.
    Address.sin_family = AF_INET;
    Address.sin_port = 8000;
    Address.sin_addr.s_addr = INADDR_ANY;
    if(bind(FileDescriptor, (struct sockaddr *)&Address, sizeof(sockaddr)) == 0)
    {
      if(listen(FileDescriptor, 10) == 0)
      {

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
