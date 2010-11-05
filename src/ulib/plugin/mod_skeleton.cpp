// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_skeleton.cpp - this is a skeleton for a userver plugin
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/plugin/mod_skeleton.h>
#include <ulib/net/server/client_image.h>

U_CREAT_FUNC(mod_skel, USkeletonPlugIn)

// Server-wide hooks

int USkeletonPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USkeletonPlugIn::handlerConfig(%p)", &cfg)

   int result = U_PLUGIN_HANDLER_GO_ON;

   U_RETURN(result);
}

int USkeletonPlugIn::handlerInit()
{
   U_TRACE(0, "USkeletonPlugIn::handlerInit()")

   int result = U_PLUGIN_HANDLER_GO_ON;

   U_RETURN(result);
}

// Connection-wide hooks

int USkeletonPlugIn::handlerREAD()
{
   U_TRACE(0, "USkeletonPlugIn::handlerREAD()")

   int result = U_PLUGIN_HANDLER_GO_ON;

   U_RETURN(result);
}

int USkeletonPlugIn::handlerRequest()
{
   U_TRACE(0, "USkeletonPlugIn::handlerRequest()")

   int result = U_PLUGIN_HANDLER_GO_ON;

   U_RETURN(result);
}

int USkeletonPlugIn::handlerReset()
{
   U_TRACE(0, "USkeletonPlugIn::handlerReset()")

   int result = U_PLUGIN_HANDLER_GO_ON;

   U_RETURN(result);
}
