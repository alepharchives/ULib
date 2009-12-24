// common2.cpp

   // init log

   UProcess proc;
   UString name(U_CAPACITY), tmp(U_CAPACITY);

   tmp.snprintf("%s/%s.log", directory.c_str(), log_name);

   ULog log(tmp);

   if (log.isOpen())
      {
      log.setShared();

      log.fmt    = "%P|%4D|%s  %N\n";
      log.prefix = "%P|%4D|";

      log.init();
      }

   (void) log.ready();

   time_t tm_session = u_now.tv_sec;

   bool esito;
   USemaphore sem;
   UString request;
   const char* keypub;
   const char* keypriv;
   char response_buffer[4096 * 4];
   unsigned request_size, response_size;

   bool bIPv6 = false;
   USSHSocket sock(bIPv6);

   sock.setVerbosity(); // no msg on stderr...
   sock.setUser(username.c_str());
