// ==============================================================================================
// C servlet sample
// ----------------------------------------------------------------------------------------------
// v8.c: how to execute javascript code
// ==============================================================================================

#include <ulib/internal/csp_interface.h>

#pragma link "libv8.so"

int main(int argc, char* argv[])
{
   char* reply = get_reply();

   (void) u_snprintf(reply, get_reply_capacity(), "<h1>%s</h1>", runv8("'Hello'+' world!'"));

   return 200;
}
// ============================================================================
// End of Source Code
// ============================================================================
