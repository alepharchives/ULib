#include "xbuffer.h" // G-WAN dynamic buffers
#include "gwan.h"    // G-WAN exported functions

int main(int argc, char *argv[])
{
  char *name="";
  char szName[20]={0};
  get_arg("name=", &name, argc, argv);
  xbuf_ctx reply; get_reply(argv, &reply);
  xbuf_xcat(&reply, "<h1>Hello %s</h1>",
                  escape_html(szName, name, sizeof(szName)-1)?szName:"");
  set_reply(argv, &reply);
  return(200);
}
