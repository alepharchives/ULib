// ============================================================================
// Benchmark framework used to compare the G-WAN Web App Server http://gwan.ch/
//                     with Apache/IIS/Nginx/Lighttpd/GlassFish/Jetty/Tomcat...
// ----------------------------------------------------------------------------
// See the How-To here: http://gwan.com/en_apachebench_httperf.html
// ----------------------------------------------------------------------------
// a.c: invoke Apache Benchmark (IBM) or HTTPerf (HP) on a concurrency
//      range and collect results in a CSV file suitable for LibreOffice
//      (http://www.documentfoundation.org/download/) charting.
//
//      Select your benchmarking tool below:

#define IBM_APACHEBENCH // the classic, made better by Zeus' author
//#define HP_HTTPERF // HTTPerf, from HP, less practical than ApacheBench

//      Modify the IP ADDRESS & PORT below to match your server values:

#define IP   "127.0.0.1"
#define PORT "8080"

//       100.html is a 100-byte file initially designed to avoid testing the
//       kernel (I wanted to compare the CPU efficiency of each Web server).
//
//       The ANSI C, C#, Java and PHP scripts used below are available from:
//       http://gwan.ch/source/
//
//       The ITER define can be set to 1 to speed up a test but in that case
//       values are not as reliable as when using more rounds (and using a
//       low ITER[ations] value usually gives lower performances):

#define FROM       0 // range to cover (1 - 1,000 concurrent clients)
#define TO      1000 // range to cover (1 - 1,000 concurrent clients)
#define STEP      10 // number of concurrency steps we actually skip
#define ITER      10 // number of iterations (3 for worse, average, best)

//       Select (uncomment) the URL that you want to test:
//
// ---- Static files ----------------------------------------------------------
#define URL "/100.html"

// ---- Apache/PHP ------------------------------------------------------------
//#define URL "/hello.php"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=1"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=10"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=50"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=100"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=150"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=500"
//#define URL "/loan.php?name=Eva&amount=10000&rate=3.5&term=800"

// ---- GlassFish/Java  -------------------------------------------------------
//#define URL "/hello"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=1"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=10"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=50"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=100"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=150"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=500"
//#define URL "/loan/loan/loan.jsp?name=Eva&amount=10000&rate=3.5&term=800"

// ---- G-WAN/C ---------------------------------------------------------------
//#define URL "/csp?hello"
//#define URL "/csp?hellox&name=Eva"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=1"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=10"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=50"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=100"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=150"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=500"
//#define URL "/csp?loan&name=Eva&amount=10000&rate=3.5&term=800"

// ---- IIS/ASP.Net C# --------------------------------------------------------
//#define URL "/asp/hello.aspx""
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=1"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=10"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=50"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=100"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=150"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=500"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3.5&term=800"

// your locale settings will need to use a comma or a point for 'rate'
// (using the wrong decimal separator will raise an exception in .Net)

//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=1"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=10"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=50"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=100"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=150"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=500"
//#define URL "/asp/loan.aspx?name=Eva&amount=10000&rate=3,5&term=800"

// ----------------------------------------------------------------------------
// Windows:
// ----------------------------------------------------------------------------
// usage: define _WIN32 below and use a C compiler to compile and link a.c

//#ifndef _WIN32
//# define _WIN32
//#endif
#ifdef _WIN32
# pragma comment(lib, "ws2_32.lib")
# define read(sock, buf, len) recv(sock, buf, len, 0)
# define write(sock, buf, len) send(sock, buf, len, 0)
# define close(sock) closesocket(sock)
#endif

//          Unless you target a localhost test, don't use a Windows machine as
//          the client (to run ab) as the performances are really terrible (ab
//          does not use the 'IO completion ports' Windows proprietary APIs and
//          pure BSD sockets are much slower under Windows than on Linux).
//
//          G-WAN for Windows upgrades Registry system values to remove some
//          artificial limits (original values are just renamed), you need to
//          reboot after you run G-WAN for the first time to load those values.
//          Rebooting for each test has an effect on Windows (you are faster),
//          like testing after IIS 7.0 was tested (you are even faster), and
//          the Windows Vista 64-bit TCP/IP stack is 10% faster (for all) if
//          ASP.Net is *not* installed.
//
//          Run gwan like this:
//
//              C:\gwan> gwan -b
//
//          The -b flag (optional) disables G-WAN's denial of service shield,
//          this gives better raw performances (this is mandatory for tests
//          under Windows because the overhead of the Denial of Service Shield
//          is breaking the benchmarks).
// ----------------------------------------------------------------------------
// Linux:
// ----------------------------------------------------------------------------
// usage: ./gwan -r a.c   (a new instance of G-WAN will run this C source code)
//
//          Linux Ubuntu 8.1 did not show significant boot-related side-effects
//          but here also I have had to tune the system (BOTH on the server and
//          client sides).                               ^^^^
//
//          The modification below works after a reboot (if an user is logged):
//          sudo gedit /etc/security/limits.conf
//              * soft nofile = 200000
//              * hard nofile = 200000
//
//          If you are logged as 'root' in a terminal, type (instant effect):
//              ulimit -HSn 200000
//
//          sudo gedit /etc/sysctl.conf
//              fs.file-max = 200000
//              net.ipv4.ip_local_port_range = 1024 65535
//              net.ipv4.ip_forward = 0
//              net.ipv4.conf.default.rp_filter = 1
//              net.core.rmem_max = 262143
//              net.core.rmem_default = 262143
//              net.core.netdev_max_backlog = 32768
//              net.core.somaxconn = 2048
//              net.ipv4.tcp_rmem = 4096 131072 262143
//              net.ipv4.tcp_wmem = 4096 131072 262143
//              net.ipv4.tcp_sack = 0
//              net.ipv4.tcp_dsack = 0
//              net.ipv4.tcp_fack = 0
//              net.ipv4.tcp_fin_timeout = 30
//              net.ipv4.tcp_orphan_retries = 0
//              net.ipv4.tcp_keepalive_time = 120
//              net.ipv4.tcp_keepalive_probes = 3
//              net.ipv4.tcp_keepalive_intvl = 10
//              net.ipv4.tcp_retries2 = 15
//              net.ipv4.tcp_retries1 = 3
//              net.ipv4.tcp_synack_retries = 5
//              net.ipv4.tcp_syn_retries = 5
//              net.ipv4.tcp_timestamps = 0
//              net.ipv4.tcp_max_tw_buckets = 32768
//              net.ipv4.tcp_moderate_rcvbuf = 1
//              kernel.sysrq = 0
//              kernel.shmmax = 67108864
//
//          Use 'sudo sysctl -p /etc/sysctl.conf' to update your environment
//          -the command must be typed in each open terminal for the changes
//          to take place (same effect as a reboot).
//
//          As I was not able to make the 'open files limit' persist for G-WAN
//          after a reboot, G-WAN attemps to setup this to an 'optimal' value
//          depending on the amount of RAM available on your system:
//
//              fd_max=(256*(totalram/4)<200000)?256*(total/4):1000000;
//
//          For this to work, you have to run gwan as 'root':
//
//              ~/Desktop/gwan# ./gwan
//              or
//              ~/Desktop/gwan$ sudo ./gwan
//
//          And, if you don't run gwan as 'root', Linux will restrict the
//          number of CPUs/Cores that gwan can use (on a 8-Core machine,
//          gwan could only use 1 Core because hyper-threading would make
//          2 'Cores' only address one physical Core). The only relevant
//          documentation I have found on the subject is Linux SETCPU(7).
//          Even with 'sudo gwan', the number of "allowed CPUs" is random
//          (between 8 and 128) but at least it covered all my 8 Cores.
// ----------------------------------------------------------------------------
//          NB: this test was up to 2x faster when the a.c client was running
//              (on Linux 64-bit and the server on Linux 32-bit) via a gigabit
//              LAN (but absolute performances are less interesting me than
//              relative server performances, hence the localhost test).
//
//              Experiments demonstrate that, for a 100-byte static file, IIS
//              and Apache use 90-100% of the CPU at high concurrencies while
//              being much slower than G-WAN (which uses "0%" of the CPU on a
//              gigabit LAN).
//
//              A low CPU usage matters because leaving free CPU resources
//              available for other tasks allows G-WAN to:
//
//                - achieve better performances by not starving the system;
//                - make room to generate dynamic contents (C servlets);
//                - make room for a database, proxy, email or virtual server;
//                - save energy (CPUs consume more energy under high loads);
//                - save money (do [20 - 200,000]x more on all your servers).
//
//              For a small static file such as the 100.html file, if your test
//              on a LAN is slower than on localhost then your environment is
//              the bottleneck (NICs, switch, client CPU, client OS...).
// ----------------------------------------------------------------------------
// History
// v1.0.4 changes: initial release to test the whole 1-1,000 concurrency range;
// v1.0.5 changes: added support for non-2xx response codes and trailing stats;
// v1.0.6 changes: corrected 64-bit platform issues and added support for gzip,
//                 dumped a non-2xx reply on stderr for further investigations.
// 2.1.20 changes: added support for HTTPerf as an alternative to ApacheBench.
// 2.4.20 changes: detect & report open (ab.txt output) file permission errors.
// ----------------------------------------------------------------------------
// This program is left in the public domain.
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
# include <winsock2.h>
# include <process.h>
# include <windows.h>
  typedef unsigned __int64 u64;
# define FMTU64 "I64u"
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <ctype.h>
# include <unistd.h>
  typedef unsigned long long u64;
# define FMTU64 "llu"
#endif

static int http_req(char *request);

// -------------------------
// STEFANO
// -------------------------
#undef IP
#undef URL
#undef PORT
#undef FROM
#undef TO
static const char* IP;  // = (argv[1]?	    argv[1] :"localhost");
static const char* URL; // = (argv[2]?	    argv[2] :"/index.html");
static int PORT;			// = (argv[3]?atoi(argv[3]):80);
static int FROM;			// = (argv[4]?atoi(argv[4]):0);
static int TO;				// = (argv[5]?atoi(argv[5]):1000);
// -------------------------

// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
// -------------------------
// STEFANO
// -------------------------
IP   = (argv[1]?argv[1]:"localhost");
URL  = (argv[2]?argv[2]:"/index.html");
PORT = (argv[3]?atoi(argv[3]):80);
FROM = (argv[4]?atoi(argv[4]):0);
TO   = (argv[5]?atoi(argv[5]):1000);

// #define U_SSL
// #define GWAN_28x
// #define U_KEEP_ALIVES
// -------------------------

   int    i, j, nbr, max_rps, min_rps, ave_rps;
   char   str[256], buff[4070];
   FILE  *f, *fo;
   time_t st = time(NULL);
   u64 tmax_rps = 0, tmin_rps = 0, tave_rps = 0;

   sprintf(str, "%s/test.txt", IP);
   fo = fopen(str, "w+b");

   //fprintf(stderr, "URL=%s\n", URL);
   //fprintf(stderr, "ret=%d\n", http_req(URL));
   //exit(0);
   
#ifndef _WIN32
   if(!fo)
   {
      perror("can't open output file"); // "Permission denied"
      return 1;
   }
#endif

   for(i = FROM; i <= TO; i += STEP)
   {

#ifdef IBM_APACHEBENCH
       // ApacheBench makes it straight for you since you can directly tell
       // the 'concurrency' and 'duration' you wish:
       sprintf(str, "ab -n 1000000 -c %d -S -d -t 1 "	
#	ifdef U_KEEP_ALIVES
						  "-k "												/* KEEP-ALIVES */
#	elif defined(GWAN_28x)
						  "-H \"Connection: close\" "					/* GWAN 2.8.[8-14] need this if NO Keep-Alives */
#	endif
                    "-H \"Accept-Encoding: gzip,deflate\" " /* HTTP compression */
#	ifdef U_SSL
                    "\"https://%s" ":%d" "%s" "\"" " > %s/ab.txt", i?i:1, IP, PORT, URL, IP);
#	else
                    "\"http://%s" ":%d" "%s" "\"" " > %s/ab.txt", i?i:1, IP, PORT, URL, IP);
#	endif
#else
       // HTTPerf does not let you specify the 'concurrency'rate:
       //
       //    rate    : number of TCP  connections per second
       //    num-con : number of TCP  connections
       //    num-call: number of HTTP requests
       //
       // If we want 100,000 HTTP requests, we have to calculate how many
       // '--num-conn' and '--num-call' to specify for a given '--rate':
       //
       //   nbr_req = rate * num-call
       //
       //   'num-conn' makes it last longer, but to get any given 'rate'
       //   'num-conn' must always be >= to 'rate'
       //
       // HTTPerf creates new connections grogressively and only collects
       // statistics after 5 seconds (to let servers 'warm-up' before they
       // are tested). This is NOT reflecting real-life situations where
       // clients send requests on short but intense bursts.
       //
       // Also, HTTPerf's looooong shots make the TIME_WAIT state become a
       // problem if you do any serious concurrency test.
       //
       // Finally, HTTPerf is unable to test client concurrency: if 'rate'
       // is 1 but num-conn is 2 and num-call is 100,000 then you are more
       // than likely to end with concurrent connections because not all
       // requests are processed when the second connection is launched.
       //
       // If you use a smaller num-call value then you are testing the TCP
       // /IP stack rather than the user-mode code of the server.
       //
       // As a result, HTTPerf can only be reliably used without Keep-Alives
       // (with num-call=1)
       //
       sprintf(str, "httperf --server=%s --port=%d "
                    "--rate=%d "
                    "--num-conns=%d --num-calls 100000 " // KEEP-ALIVES
//                  "--num-conns=1000000 --num-calls 1 " // NO Keep_Alives
                    "--timeout 5 --hog --uri=\""
                    "%s\"" " > %s/ab.txt", IP, PORT, i?i:1, i?i:1, URL, IP);
#endif

      for(max_rps = 0, ave_rps = 0, min_rps = 0xffff0, j = 0; j < ITER; j++)
      {
         system(str);

#ifdef _WIN32
         // Windows needs to take its breath after system() calls (this is not
         // giving any advantage to Windows as all the tests have shown that
         // this OS platform is -by far- the slowest and less scalable of all)
         Sleep(4000);
#endif
         // get the information we need from ab.txt
			sprintf(buff, "%s/ab.txt", IP);
			sync();
         if(!(f = fopen(buff, "rb")))
         {
            printf("Can't open ab.txt output\n");
            return 1;
         }
         //memset(buff, 0, sizeof(buff) - 1);
         *buff = 0;
         nbr = fread(buff, 1, sizeof(buff) - 1, f);
         if(nbr <= 0)
         {
            printf("Can't read ab.txt output\n");
            return 1;
         }
         fclose(f);
         *(buff + nbr) = 0;

         nbr = 0;
         if(*buff)
         {
            // IIS 7.0 quickly stops serving loans and sends error 503 (Service
            // unavailable) at a relatively high rate. If we did not detect it
            // this would be interpreted as a 'boost' in performance while, in
            // fact, IIS is dying. Soon, IIS would really die and we would have
            // to reboot the host: a complete IIS stop/restart has no effect).

            // Other issues to catch here are error 30x (redirects) or 404
            // (not found) on badly configured servers that make users report
            // that their application server is fast when this is not the case.
#ifdef IBM_APACHEBENCH
            char *p = strstr(buff, "Non-2xx responses:");
            if(p) // "Non-2xx responses:      50130"
            {
               char *n;
               p += sizeof("Non-2xx responses:");
               while(*p == ' ' || *p == '\t') p++; n = p;
               while(*p >= '0' && *p <= '9')  p++;
               *p = 0;
               nbr = atoi(n);
               if(nbr)
               {
                  printf("* Non-2xx responses:%d\n", nbr);
                  fprintf(fo, "* Non-2xx responses:%d\n", nbr);

                  // dump the server reply on stderr for examination
                  http_req(URL);
                  goto end;
               }
            }

            p = strstr(buff, "Requests per second:");
            if(p) // "Requests per second:    16270.00 [#/sec] (mean)"
            {
               char *n;
               p += sizeof("Requests per second:");
               while(*p == ' ' || *p == '\t') p++; n = p;
               while(*p >= '0' && *p <= '9')  p++; *p = 0;
               nbr = atoi(n);
            }
            else
               puts("* 'Requests per second' not found!");
#else
            char *p=strstr(buff, "Reply status:");
            if(p) // "Reply status: 1xx=0 2xx=1000000 3xx=0 4xx=0 5xx=0"
            {
               char *n;
               p+=sizeof("Reply status: 1xx=") - 1;

               // we are not interested in "1xx" errors

               if(*p == '0') // pass "2xx=" if no errors
                   p = strstr(p, "3xx=");
               if(p && p[4] == '0') // pass "3xx="  if no errors
                   p = strstr(p, "4xx=");
               if(p && p[4] == '0') // pass "4xx="  if no errors
                   p = strstr(p, "5xx=");
               if(p && p[4] == '0') // pass "5xx="  if no errors
                  goto no_errors;

               p+=sizeof("5xx=");

               while(*p==' '||*p=='\t') p++; n=p;
               while(*p>='0'&&*p<='9') p++; *p=0;
               nbr=atoi(n);
               if(nbr)
               {
                  printf("* Non-2xx responses:%d\n", nbr);
                  fprintf(fo, "* Non-2xx responses:%d\n", nbr);

                  // dump the server reply on stderr for examination
                  http_req(URL);
                  goto end;
               }
            }
no_errors:
            // Reply rate [replies/s]: min 163943.9 avg 166237.2 max 167482.3
            // stddev 1060.4 (12 samples)
            p=strstr(buff, "Reply rate");
            if(p)
            {
               char *n;
               p+=sizeof("Reply rate [replies/s]: min");
               while(*p==' ' || *p=='\t') p++; n=p;
               while(*p>='0' && *p<='9') p++; *p++=0; p++;
               min_rps=atoi(n);

               while(*p<'0' || *p>'9') p++; // avg
               n=p;
               while(*p>='0' && *p<='9') p++; *p++=0; p++;
               ave_rps=atoi(n);

               while(*p<'0' || *p>'9') p++; // max
               n=p;
               while(*p>='0' && *p<='9') p++; *p++=0; p++;
               max_rps=atoi(n);
            }
            else
               puts("* 'Reply rate' not found!");

            // HTTPerf needs so many more requests than AB that it quickly
            // exhausts the [1-65,535] port space. There is no obvious
            // solution other than using several HTTPerf workers OR waiting
            /* a bit between each shot to let the system evacuate the bloat:
            if(!strcmp(IP, "127.0.0.1"))
            {
               int nop = 60;
               printf("waiting:"); fflush(0);
               while(nop--)
               {
                  printf("."); fflush(0);
                  sleep(1);
               }
               printf("\n"); fflush(0);
            }*/

            goto round_done;
#endif
         }
         if(max_rps < nbr) max_rps = nbr;
         if(min_rps > nbr) min_rps = nbr;
         ave_rps += nbr;
      }
      ave_rps /= ITER;
#ifndef IBM_APACHEBENCH
round_done:
#endif
      tmin_rps += min_rps;
      tmax_rps += max_rps;
      tave_rps += ave_rps;

      // display data for convenience and save it on disk
      nbr = sprintf(buff, "\t %d,\t %d,\t %d,\t %d\n",
                    i?i:1, min_rps, ave_rps, max_rps);
      printf("=> %s", buff);
      if(fwrite(buff, 1, nbr, fo) != nbr)
      {
         printf("fwrite(fo) failed");
         return 1;
      }
      fflush(fo); // in case we interrupt the test
   }

end:
   st = time(NULL) - st;
   strftime(str, sizeof(str) - 1, "%X", gmtime(&st));

   sprintf(buff, "min:%"FMTU64"   avg:%"FMTU64"   max:%"FMTU64
                 " Time:%ld second(s) [%s]\n",
                 tmin_rps, tave_rps, tmax_rps, st, str);
   puts  (buff);
   fputs (buff, fo);
   fclose(fo);
   return 0;
}
// ============================================================================
// A 'quick and (really) dirty' wget (don't use this code in production!)
// ----------------------------------------------------------------------------
// read a CRLF-terminated line of text from the socket
// return the number of bytes read, -1 if error
// ----------------------------------------------------------------------------
static int read_line(int fd, char *buffer, int max)
{
   char *p = buffer;
   while(max--)
   {
      if(read(fd, p, 1) <= 0)
         break;
      if(*p == '\r') continue;
      if(*p == '\n') break;
      p++;
  }
  *p = 0;
  return p-buffer;
}
// ----------------------------------------------------------------------------
// read 'len' bytes from the socket
// return the number of bytes read, -1 if error
// ----------------------------------------------------------------------------
static int read_len(int fd, char *buffer, int len)
{
  int ret;
  char *p = buffer;
  while(len > 0)
  {
      ret = read(fd, p, len);
      if(ret <= 0)
         return -1;
      p   += ret;
      len -= ret;
  }
  return p-buffer;
}
// ----------------------------------------------------------------------------
// connect to the server, send the HTTP request and dump the server reply
// return the HTTP status sent by the server, -1 if error
// ----------------------------------------------------------------------------
static int http_req(char *request)
{
   char   buf[4096], *p;
   int    ret=-1, s, len;
   struct hostent    *hp;
   struct sockaddr_in host;

#ifdef _WIN32
   WSADATA sa;
   WORD ver = MAKEWORD(2, 2);
   WSAStartup(ver, &sa);
#endif

   while((hp = gethostbyname(IP)))
   {
      memset((char*)&host,0, sizeof(host));
      memmove((char*)&host.sin_addr, hp->h_addr, hp->h_length);
      host.sin_family = hp->h_addrtype;
      host.sin_port   = htons((unsigned short)PORT);

      if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        break;

      if(connect(s, (struct sockaddr*)&host, sizeof(host)) < 0)
        break;

      len = sprintf(buf, "GET %s HTTP/1.1\r\n"
                    "Host: %s" ":%d" "\r\n"
                    "User-Agent: a.c\r\n"
                    "Accept-Encoding: gzip,deflate\r\n"
                    "Connection: close\r\n\r\n",
                    request, IP, PORT);

      if(write(s, buf, len) != len)
      {
         break;
      }
      else
      {
         len = read_line(s, buf, sizeof(buf)-1);
         fputs(buf, stderr);
         putc ('\n',stderr);
         if(len <= 0)
           break;
         else
         if(sscanf(buf, "HTTP/1.%*d %3d", (int*)&ret) != 1)
           break;
      }

      if(ret > 0) // ret is the HTTP status, parse the server reply
      {
         for(*buf=0;;)
         {
             int n = read_line(s, buf, sizeof(buf)-1);
             fputs(buf, stderr);
             putc ('\n',stderr);
             if(n <= 0)
                break;

             for(p = buf; *p && *p != ':'; p++)
                 *p = tolower(*p);

             sscanf(buf, "content-length: %d", &len);
        }

        len = (len > (sizeof(buf)-1)) ? (sizeof(buf)-1) : len;
        len = read_len(s, buf, len);
        if(len > 0)
        {
           buf[len] = 0;
           fputs(buf, stderr);
           putc ('\n',stderr);
        }
      }
      break;
   };

   close(s);
   return ret;
}
// ============================================================================
// End of Source Code
// ============================================================================

