#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "handmade.h"

s32 main()
{
  s32 SocketHandle = socket(AF_INET, SOCK_STREAM, 0);
  if(SocketHandle != -1)
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
          while(accept(SocketHandle, (struct sockaddr *)&ClientAddress, &ClientAddressLenght) == -1)
          {
            usleep(1000);
          }
          
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
