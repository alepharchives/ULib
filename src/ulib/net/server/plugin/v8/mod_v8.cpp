// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_v8.cpp - this is a wrapper of Google V8 JavaScript Engine
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/string.h>

#include <v8.h>
#include <string.h>

using namespace v8;

// http://code.google.com/apis/v8/get_started.html#intro
// ------------------------------------------------------------------------------
// compiles and executes javascript and returns the script return value as string

extern "C" {
extern U_EXPORT void runv8(UString& x);
       U_EXPORT void runv8(UString& x)
{
   U_TRACE(0, "::runv8(%.*S)", U_STRING_TO_TRACE(x))

   // Create a stack-allocated handle scope.
   HandleScope handle_scope;

   // Create a new context.
   Persistent<Context> context = Context::New();

   // Enter the created context for compiling and running the script.
   Context::Scope context_scope(context);

   // Create a string containing the JavaScript source code.
   Handle<String> source = String::New(U_STRING_TO_PARAM(x));

   // Compile the source code.
   Handle<Script> script = Script::Compile(source);

   // Run the script
   Handle<Value> result = script->Run();

   // Dispose the persistent context.
   context.Dispose();

   // return result as string, must be deallocated in cgo wrapper
   String::AsciiValue ascii(result);

   (void) x.replace(*ascii, ascii.length());
}
}
