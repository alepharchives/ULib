// ============================================================================
// Benchmark framework to test the (free) G-WAN Web App. Server http://gwan.ch/
// ----------------------------------------------------------------------------
// a.c: invoke Apache Benchmark (ab) for a concurrency range and collect 
//      results in a file suitable for OpenOffice.org SpreadSheet charting.
// 
//      Modify the IP ADDRESS & PORT below to match your server values:

#define IP   "192.168.200.88"
//#define IP   "127.0.0.1"
#define PORT "8080"
//#define PORT "80"

//       100.html is a 100-byte file initially designed to avoid testing the
//       kernel (I wanted to compare the CPU efficiency of each Web server).
//
//       The ITER define can be set to 1 to speed up a test but in that case
//       values are not as reliable as when using more rounds (and using a 
//       low ITER[ations] value usually gives lower performances):

#define FROM    0 // the range to cover (1-1000 concurrent clients)
#define TO   1000 // the range to cover (1-1000 concurrent clients)
#define STEP   10 // the number of concurrency steps we actually skip
#define ITER   10 // the number of iterations (worse, average, best)

//       Select (uncomment) the URL that you want to test:
//
// ---- Static files ----------------------------------------------------------
//#define URL "/1000.html"
#define URL "/100.html"
//#define URL "/10.html"

// -------------------------
// STEFANO
// -------------------------
#undef IP
#undef URL
#undef PORT
#undef FROM
static const char* IP;  // = (argv[1]?	  argv[1] :"localhost");
static const char* URL; // = (argv[2]?	  argv[2] :"/index.html");
static int PORT;			// = (argv[3]?atoi(argv[3]):80);
static int FROM;			// = (argv[4]?atoi(argv[4]):0);
// -------------------------

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
// (using the wrong decimal separator will raise an exception in ASP.Net C#)

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
//          this gives better raw performances (mandatory under Windows).
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
//              ~/Desktop/gwan# ./gwan -b
//              or
//              ~/Desktop/gwan$ sudo ./gwan -b
//
//          The -b flag (optional) disables G-WAN's denial of service shield,
//          this gives better raw performances (mandatory under Windows).
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
//                - save money (doing 20-200,000x more on each of your server).
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

static int http_req(const char* request);

// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
   int    i, j, nbr, max_rps, min_rps, ave_rps;
   char   str[256], buff[4000];
   FILE  *f, *fo=fopen("test.txt", "w+b");
   time_t st=time(NULL);
   u64    tmax_rps=0, tmin_rps=0, tave_rps=0;

	// -------------------------
	// STEFANO
	// -------------------------
#undef IP
#undef URL
#undef PORT
#undef FROM
	const char* IP  = (argv[1]?	  argv[1] :"localhost");
	const char* URL = (argv[2]?	  argv[2] :"/index.html");
	int PORT			 = (argv[3]?atoi(argv[3]):80);
	int FROM			 = (argv[4]?atoi(argv[4]):0);
	// -------------------------

   //fprintf(stderr, "URL=%s\n", URL);
   //fprintf(stderr, "ret=%d\n", http_req(URL));
   //exit(0);

   for(i=FROM; i<=TO; i+=STEP)
   {
       sprintf(str, "ab -n 1000000 -c %d -S -d -t 1 -k "		 // KEEP-ALIVES
//     sprintf(str, "ab -n 1000000 -c %d -S -d -t 1 "			 // NO Keep-Alives
                    "-H \"Accept-Encoding: gzip,deflate\" "  // HTTP compression
                    "\"http://%s" ":%d" "%s" "\"" " > ab.txt", i?i:1, IP, PORT, URL);

      for(max_rps=0, ave_rps=0, min_rps=0xffff0, j=0; j<ITER; j++)
      {
         system(str);
#ifdef _WIN32
         // Windows needs to take its breath after system() calls (this is not
         // giving any advantage to Windows as all the tests have shown that 
         // this OS platform is -by far- the slowest and less scalable of all)
         Sleep(4000);
#endif         
         // get the information we need from res.txt
         if(!(f=fopen("ab.txt", "rb")))
         {
            printf("Can't open file\n");
            return 1;
         }
         memset(buff, 0, sizeof(buff)-1);
         fread (buff, 1, sizeof(buff)-1, f);
         fclose(f);

         nbr=0;
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
            
            char *p=strstr(buff, "Non-2xx responses:");
            if(p) // "Non-2xx responses:      50130"
            {
               char *n;
               p+=sizeof("Non-2xx responses:");
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
          
            p=strstr(buff, "Requests per second:");
            if(p) // "Requests per second:    16270.00 [#/sec] (mean)"
            {
               char *n;
               p+=sizeof("Requests per second:");
               while(*p==' '||*p=='\t') p++; n=p; 
               while(*p>='0'&&*p<='9') p++; *p=0;
               nbr=atoi(n);
            }
            else
               puts("* 'Requests per second' not found!");
         }
         if(max_rps<nbr) max_rps=nbr;
         if(min_rps>nbr) min_rps=nbr;
         ave_rps+=nbr;
      }
      ave_rps/=ITER;
      tmin_rps+=min_rps;
      tmax_rps+=max_rps;
      tave_rps+=ave_rps;

      // display data for convenience and save it on disk
      printf("##### %4d,%5d,%5d,%5d #####\n", i?i:1, min_rps, ave_rps, max_rps);
      fprintf(fo, "%d,%d,%d,%d\n", i?i:1, min_rps, ave_rps, max_rps);
      fflush(fo); // in case we interrupt the test
   }

end:
   st = time(NULL)-st;
   strftime(str, sizeof(str)-1, "%X", gmtime(&st));
  
   printf("\nScore:%"FMTU64",%"FMTU64",%"FMTU64" Time:%d second(s) [%s]\n",
          tmin_rps, tave_rps, tmax_rps, st, str);

   fprintf(fo, "\nScore:%"FMTU64",%"FMTU64",%"FMTU64" Time:%d second(s)"
               " [%s]\n", tmin_rps, tave_rps, tmax_rps, st, str);
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
static int http_req(const char* request)
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
