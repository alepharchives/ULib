#include "xbuffer.h" // G-WAN dynamic buffers
#include "gwan.h"    // G-WAN exported functions

int main(int argc, char *argv[])
{
  char *name="";
  get_arg("name=", &name, argc, argv);
  xbuf_ctx reply; get_reply(argv, &reply);
  xbuf_xcat(&reply, "<html><body>Hello %s</body></html>",
	                 escape_html(name, name, 20)?name:"");
  set_reply(argv, &reply);
  return(200);
}
