// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    macro.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBDBG_MACRO_H
#define ULIBDBG_MACRO_H 1

#include <ulib/base/macro.h>

// Design by contract - if (expr == false) then stop

#ifdef DEBUG
#  define U_ASSERT(expr)                 UTrace::suspend(); U_INTERNAL_ASSERT(expr);                UTrace::resume();
#  define U_ASSERT_MINOR(a,b)            UTrace::suspend(); U_INTERNAL_ASSERT_MINOR(a,b);           UTrace::resume();
#  define U_ASSERT_MAJOR(a,b)            UTrace::suspend(); U_INTERNAL_ASSERT_MAJOR(a,b);           UTrace::resume();
#  define U_ASSERT_EQUALS(a,b)           UTrace::suspend(); U_INTERNAL_ASSERT_EQUALS(a,b);          UTrace::resume();
#  define U_ASSERT_DIFFERS(a,b)          UTrace::suspend(); U_INTERNAL_ASSERT_DIFFERS(a,b);         UTrace::resume();
#  define U_ASSERT_POINTER(ptr)          UTrace::suspend(); U_INTERNAL_ASSERT_POINTER(ptr);         UTrace::resume();
#  define U_ASSERT_RANGE(a,x,b)          UTrace::suspend(); U_INTERNAL_ASSERT_RANGE(a,x,b);         UTrace::resume();

#  define U_ASSERT_MSG(expr,info)        UTrace::suspend(); U_INTERNAL_ASSERT_MSG(expr,info);       UTrace::resume();
#  define U_ASSERT_MINOR_MSG(a,b,info)   UTrace::suspend(); U_INTERNAL_ASSERT_MINOR_MSG(a,b,info)   UTrace::resume();
#  define U_ASSERT_MAJOR_MSG(a,b,info)   UTrace::suspend(); U_INTERNAL_ASSERT_MAJOR_MSG(a,b,info)   UTrace::resume();
#  define U_ASSERT_EQUALS_MSG(a,b,info)  UTrace::suspend(); U_INTERNAL_ASSERT_EQUALS_MSG(a,b,info)  UTrace::resume();
#  define U_ASSERT_DIFFERS_MSG(a,b,info) UTrace::suspend(); U_INTERNAL_ASSERT_DIFFERS_MSG(a,b,info) UTrace::resume();
#  define U_ASSERT_POINTER_MSG(ptr,info) UTrace::suspend(); U_INTERNAL_ASSERT_POINTER_MSG(ptr,info) UTrace::resume();
#  define U_ASSERT_RANGE_MSG(a,x,b,info) UTrace::suspend(); U_INTERNAL_ASSERT_RANGE_MSG(a,x,b,info) UTrace::resume();
#elif defined(U_TEST)
#  define U_ASSERT(expr)                                    U_INTERNAL_ASSERT(expr)
#  define U_ASSERT_MINOR(a,b)                               U_INTERNAL_ASSERT_MINOR(a,b)
#  define U_ASSERT_MAJOR(a,b)                               U_INTERNAL_ASSERT_MAJOR(a,b)
#  define U_ASSERT_EQUALS(a,b)                              U_INTERNAL_ASSERT_EQUALS(a,b)
#  define U_ASSERT_DIFFERS(a,b)                             U_INTERNAL_ASSERT_DIFFERS(a,b)
#  define U_ASSERT_POINTER(ptr)                             U_INTERNAL_ASSERT_POINTER(ptr)
#  define U_ASSERT_RANGE(a,x,b)                             U_INTERNAL_ASSERT_RANGE(a,x,b)

#  define U_ASSERT_MSG(expr,info)                           U_INTERNAL_ASSERT_MSG(expr,info)
#  define U_ASSERT_MINOR_MSG(a,b,info)                      U_INTERNAL_ASSERT_MINOR_MSG(a,b,info)
#  define U_ASSERT_MAJOR_MSG(a,b,info)                      U_INTERNAL_ASSERT_MAJOR_MSG(a,b,info)
#  define U_ASSERT_EQUALS_MSG(a,b,info)                     U_INTERNAL_ASSERT_EQUALS_MSG(a,b,info)
#  define U_ASSERT_DIFFERS_MSG(a,b,info)                    U_INTERNAL_ASSERT_DIFFERS_MSG(a,b,info)
#  define U_ASSERT_POINTER_MSG(ptr,info)                    U_INTERNAL_ASSERT_POINTER_MSG(ptr,info)
#  define U_ASSERT_RANGE_MSG(a,x,b,info)                    U_INTERNAL_ASSERT_RANGE_MSG(a,x,b,info)
#else
#  define U_ASSERT(expr)
#  define U_ASSERT_MINOR(a,b)
#  define U_ASSERT_MAJOR(a,b)
#  define U_ASSERT_EQUALS(a,b)
#  define U_ASSERT_DIFFERS(a,b)
#  define U_ASSERT_POINTER(ptr)
#  define U_ASSERT_RANGE(a,x,b)

#  define U_ASSERT_MSG(expr,info)
#  define U_ASSERT_MINOR_MSG(a,b,info)
#  define U_ASSERT_MAJOR_MSG(a,b,info)
#  define U_ASSERT_EQUALS_MSG(a,b,info)
#  define U_ASSERT_DIFFERS_MSG(a,b,info)
#  define U_ASSERT_POINTER_MSG(ptr,info)
#  define U_ASSERT_RANGE_MSG(a,x,b,info)
#endif

// Some useful macros for conditionally compiling debug features...

#ifdef DEBUG
// Manage test for memory corruption

#  define U_MEMORY_TEST             UMemoryError memory;
#  define U_MEMORY_TEST_COPY(o)                  memory = o.memory;
#  define U_CHECK_MEMORY            U_CHECK_MEMORY_CLASS(memory);
#  define U_CHECK_MEMORY_OBJ(o)     U_CHECK_MEMORY_CLASS((o).memory);

// Manage info on execution of program

#  define U_TRACE(level,args...)    UTrace utr(level , ##args);

// NB: U_DUMP(), U_SYSCALL(), U_RETURN() sono vincolati alla presenza di U_TRACE()

#  define U_INTERNAL_DUMP(args...) { if (utr.active)                      u_trace_dump(args); }
#  define          U_DUMP(args...) { if (utr.active) { UTrace::suspend(); u_trace_dump(args); \
                                                       UTrace::resume(); } }

#  define U_SYSCALL(name,format,args...)        (utr.trace_syscall("::"#name"("format")" , ##args), \
                                                 utr.trace_sysreturn_type(::name(args)))

#  define U_SYSCALL_NO_PARAM(name)              (utr.trace_syscall("::"#name"()",NullPtr), \
                                                 utr.trace_sysreturn_type(::name()))

#  define U_SYSCALL_VOID(name,format,args...)   {utr.trace_syscall("::"#name"("format")" , ##args); \
                                                 name(args); utr.trace_sysreturn(false,NullPtr);}

#  define U_SYSCALL_VOID_NO_PARAM(name)         {utr.trace_syscall("::"#name"()",NullPtr); \
                                                 name(); utr.trace_sysreturn(false,NullPtr);}

#  define U_RETURN(r)                  return (utr.trace_return_type((r)))
#  define U_RETURN_STRING(str)         return (utr.trace_return("%.*S",U_STRING_TO_TRACE((str))),(str))
#  define U_RETURN_OBJECT(obj)         return (utr.trace_return(  "%O",U_OBJECT_TO_TRACE((obj))),(obj))
#  define U_RETURN_POINTER(ptr,type)   return ((type*)utr.trace_return_type((void*)(ptr)))
#  define U_RETURN_SUB_STRING(substr)  U_RETURN_STRING(substr)

// A mechanism that allow all objects to be registered with a central in-memory "database"
// that can dump the state of all live objects

#  define U_REGISTER_OBJECT_PTR(level,CLASS,ptr)  \
            if (UObjectDB::fd > 0 && \
                (level) >= UObjectDB::level_active) { \
                  UObjectDB::registerObject(new UObjectDumpable_Adapter<CLASS>(level, #CLASS, ptr)); }

#  define U_REGISTER_OBJECT(level,CLASS)  \
            if (UObjectDB::fd > 0 && \
                (level) >= UObjectDB::level_active) { \
                  UObjectDB::registerObject(new UObjectDumpable_Adapter<CLASS>(level, #CLASS, this)); }

#  define U_UNREGISTER_OBJECT(level,ptr) \
            if (UObjectDB::fd > 0 && \
                (level) >= UObjectDB::level_active) { \
                UObjectDB::unregisterObject(ptr); }

#  define U_TRACE_REGISTER_OBJECT(level,CLASS,format,args...) if (UObjectDB::flag_new_object == false) U_SET_LOCATION_INFO; \
                                                                  UObjectDB::flag_new_object =  false; \
                                                              U_REGISTER_OBJECT(level,CLASS) \
                                                              UTrace utr(level, #CLASS"::"#CLASS"("format")" , ##args);

#  define U_TRACE_UNREGISTER_OBJECT(level,CLASS)              U_UNREGISTER_OBJECT(level,this) \
                                                              UTrace utr(level, #CLASS"::~"#CLASS"()");

// Manage location info for object allocation

#  define U_NEW(args...)                  (U_SET_LOCATION_INFO, UObjectDB::flag_new_object = true, new args)
#  define U_NEW_VEC(n,args...)            (U_SET_LOCATION_INFO, UObjectDB::flag_new_object = true, new args[n])
#  define U_NEW_ULIB_OBJECT(obj,args...)  UObjectDB::flag_ulib_object = true, \
                                          obj = U_NEW(args), \
                                          UObjectDB::flag_ulib_object = false

#  define U_ALLOCA(args...)               U_SET_LOCATION_INFO; args

// Dump argument for exec()...

#  define U_DUMP_EXEC(argv, envp) { uint32_t _i; \
for (_i = 0; argv[_i]; ++_i) \
   { \
   U_INTERNAL_DUMP("argv[%2u] = %p %S", _i, argv[_i], argv[_i]) \
   } \
U_INTERNAL_DUMP("argv[%2u] = %p %S", _i, argv[_i], argv[_i]) \
if (envp) \
   { \
   for (_i = 0; envp[_i]; ++_i) \
      { \
      U_INTERNAL_DUMP("envp[%2u] = %p %S", _i, envp[_i], envp[_i]) \
      } \
   U_INTERNAL_DUMP("envp[%2u] = %p %S", _i, envp[_i], envp[_i]) \
   } }

// Dump attributes...

#  define U_DUMP_ATTRS(attrs) { uint32_t _i; for (_i = 0; attrs[_i]; ++_i) { U_INTERNAL_DUMP("attrs[%2u] = %S", _i, attrs[_i]) } }

#ifdef __MINGW32__
#  define U_FORK()                     -1
#  define U_VFORK()                    -1
#else
#  define U_FORK()                     u_debug_fork(  ::fork())
#  define U_VFORK()                    u_debug_vfork(::vfork())
#endif

#  define U_EXIT(exit_value)           u_debug_exit(exit_value), ::exit(exit_value)
#  define U_EXEC(pathname, argv, envp) u_debug_exec(pathname, argv, envp)

// Dump fd_set...

#ifndef __FDS_BITS
#  ifdef __MINGW32__
#     define __FDS_BITS(fd_set) ((fd_set)->fd_array)
#  else
#     define __FDS_BITS(fd_set) ((fd_set)->fds_bits)
#  endif
#endif

#else /* DEBUG */

#  define U_MEMORY_TEST
#  define U_MEMORY_TEST_COPY(o)
#  define U_CHECK_MEMORY
#  define U_CHECK_MEMORY_OBJ(o)

#  define U_TRACE(level,args...)
#  define U_DUMP(args...)
#  define U_INTERNAL_DUMP(args...)
#  define U_SYSCALL(name,format,args...)        ::name(args)
#  define U_SYSCALL_VOID(name,format,args...)   ::name(args)
#  define U_SYSCALL_NO_PARAM(name)              ::name()
#  define U_SYSCALL_VOID_NO_PARAM(name)         ::name()

#  define U_OBJECT_TO_TRACE(object)

#  define U_RETURN(r)                  return (r)
#  define U_RETURN_STRING(r)           return (r)
#  define U_RETURN_OBJECT(obj)         return (obj)
#  define U_RETURN_POINTER(ptr,type)   return ((type*)ptr)
#  define U_RETURN_SUB_STRING(r)       return (r)

#  define U_REGISTER_OBJECT(level,CLASS)
#  define U_REGISTER_OBJECT_PTR(level,CLASS,ptr)
#  define U_UNREGISTER_OBJECT(level,pointer)
#  define U_TRACE_UNREGISTER_OBJECT(level,CLASS)
#  define U_TRACE_REGISTER_OBJECT(level,CLASS,format,args...)

#  define U_NEW(args...)                        new args
#  define U_ALLOCA(args...)                         args
#  define U_NEW_VEC(n,args...)                  new args[n]
#  define U_NEW_ULIB_OBJECT(obj,args...)  obj = new args

#  define U_DUMP_ATTRS(attrs)
#  define U_DUMP_EXEC(argv, envp)

#ifdef __MINGW32__
#  define U_FORK()                     -1
#  define U_VFORK()                    -1
#else
#  define U_FORK()                      ::fork()
#  define U_VFORK()                    ::vfork()
#endif

#  define U_EXIT(exit_value)           ::exit(exit_value)
#  define U_EXEC(pathname, argv, envp) u_exec_failed = false; ::execve(pathname, argv, envp); u_exec_failed = true; \
                                       U_WARNING("::execve(%s,%p,%p) = -1%R", pathname, argv, envp, NULL); \
                                       ::_exit(EX_UNAVAILABLE)
#endif /* DEBUG */

#endif
