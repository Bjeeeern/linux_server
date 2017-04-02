#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "handmade.h"

s32 main()
{
  int FileDescriptor;
  FileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if(FileDescriptor != -1)
  {
    sockaddr Address = {};
    //TODO(bjorn): Read this from a settings text file.
    Address.sin_port = 8000;
    Address.sin_ddr.s_addr = INADDR_ANY;
    if(bind(FileDescriptor, &Address, sizeof(sockaddr)) == 0)
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

  return 0;
}
