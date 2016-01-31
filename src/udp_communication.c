/*!=============================================================================
  ==============================================================================

  \file    udp_communication.c

  \author  Stefan Schaal
  \date    May 2004

  ==============================================================================
  \remarks
  
  A general implementation of udp communication, with switch between different
  operating systems. The code is adapted from vxWorks examples and Gordon 
  Cheng's udp communication with robotic systems. Both client and server code
  are combined here.

  Note: there is a limit to the size of UDP message, usually 9216 bytes,
        as otherwise the socket buffer is exhausted.
  
  ============================================================================*/

// general UNIX headers
#ifdef VX
#include "vxWorks.h" 
#include "sockLib.h" 
#include "inetLib.h" 
#include "stdioLib.h" 
#include "strLib.h" 
#include "ioLib.h" 
#include "fioLib.h" 
#include "hostLib.h" 
#include "taskLib.h" 
#include "sysLib.h" 
#include "time.h" 
#include "ioctl.h"
#define  socklen_t int
#else
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/socket.h"
#include "sys/time.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "strings.h"
#include "string.h"
#include "netdb.h"
#ifdef powerpc
#include "sys/filio.h"
#endif
#ifdef sparc
#include "sys/socket.h"
#include "sys/filio.h"
#endif
#include "sys/fcntl.h"
#include "time.h"
#endif

#include "netinet/in.h"
#include "sys/ioctl.h"
#include "fcntl.h"


#include "netdb.h"

// my utilities library
#include "utility.h"

// special timer for sleeping
#include "timer.h"

// udp library
#include "udp_communication.h"

// sleep or not?  If yes, make sure the timer has proper low resolution,
// but also that the system does not block due to too much polling. Note
// that this option is only needed for the test functions, and not for
//! using the regular API functions.
#define USE_SLEEP TRUE

// local functions


/*!*****************************************************************************
 *******************************************************************************
 \note  createUDPSocket
 \date  May 2004
 
 \remarks 
 
 A socket is created that can subsequently be used in communication. The 
 socket can be either a server of client socket.
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in]     serverPortNum     : which port to use
 \param[in]     serverName        : name or IP address of server -- pass "" to use
                        localhost
 \param[in]     serverFlag        : TRUE/FALSE if socket is for server or not            
 \param[out]    ss                : structure of server socket information

 returns TRUE if all OK, otherwise FALSE
 
 ******************************************************************************/
int
createUDPSocket(int serverPortNum, 
		char *serverName, 
		int serverFlag,
		UDPSocket *ss)
{ 
  int            sockAddrSize;   // size of socket address structure
  unsigned int   opts;           // socket options

  // set up the local address 
  sockAddrSize = sizeof (struct sockaddr_in); 
  bzero ((char *) &ss->serverAddr, sockAddrSize); 
#ifdef VX
  ss->serverAddr.sin_len     = (u_char) sockAddrSize; 
#endif
#ifdef powerpc
  ss->serverAddr.sin_len     = (u_char) sockAddrSize; 
#endif
  ss->serverAddr.sin_family  = AF_INET; 
  ss->serverAddr.sin_port    = htons (serverPortNum); 

  if (strcmp(serverName,"")==0) {
    // use localhost as server
    ss->serverAddr.sin_addr.s_addr = htonl (INADDR_ANY); 
  } else {
    // determine server from serverName
#ifdef VX
    if (((ss->serverAddr.sin_addr.s_addr = inet_addr(serverName)) == ERROR ) && 
	((ss->serverAddr.sin_addr.s_addr = hostGetByName (serverName)) == ERROR )) { 
      printf("Error: unknown server name\n"); 
      return FALSE; 
    }
#else
    struct hostent *host_ent;
    struct in_addr *addr_ptr;

    if ((host_ent = (struct hostent *) gethostbyname (serverName)) == NULL) {
      if ((ss->serverAddr.sin_addr.s_addr = inet_addr(serverName)) == ERROR) { 
	printf("Error: unknown server name\n"); 
	return FALSE; 
      }
    } else {
      addr_ptr = (struct in_addr *) *host_ent->h_addr_list;
      serverName = inet_ntoa (*addr_ptr);
      ss->serverAddr.sin_addr.s_addr = inet_addr(serverName);
    }
#endif
  } 

  // create a UDP-based socket
  if ((ss->sFd = socket (AF_INET, SOCK_DGRAM, 0)) == ERROR) { 
    printf("Error: could not create socket\n"); 
    return FALSE; 
  } 

  // make sure the socket uses blocking mode
  opts = FALSE;
  ioctl(ss->sFd,FIONBIO,(unsigned long) (&opts));

  /*
#else 

  fcntl(ss->sFd,F_GETFL,&opts);
  //printf("socket opts = 0x%x\n",opts & O_NONBLOCK);
  //printf("socket opts = 0x%x\n",opts & O_NDELAY);
  opts = opts & ~O_NONBLOCK & ~O_NDELAY;
  fcntl(ss->sFd,F_SETFL,&opts);
  //fcntl(ss->sFd,F_GETFL,&opts);
  //printf("socket opts = 0x%x\n",opts & O_NONBLOCK);
  //printf("socket opts = 0x%x\n",opts & O_NDELAY);

#endif
  */
  
  if (serverFlag) {
    // bind socket to local address
    if (bind (ss->sFd, (struct sockaddr *) &ss->serverAddr, sockAddrSize) == ERROR) { 
      printf("Error: couldn't bind to socket address\n"); 
      close (ss->sFd); 
      return FALSE; 
    } 
  }

  ss->active = TRUE;

  return TRUE;

}

/*!*****************************************************************************
 *******************************************************************************
 \note  readUDPSocket
 \date  May 2004
 
 \remarks 
 
 Read from a previously created socket.
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in]     ss              : structure of server socket information
 \param[in]     bufLen          : length of data buffer
 \param[out]    buf             : data buffer
 \param[out]    inetAddr        : inet address from where data was received, 
                      allocate as "char inetAddr[INET_ADDR_LEN]" for vxWorks,
                      and as "char inetAddr[INET_ADDRSTRLENT]" for unix.
                      NOTE: pass NULL to avoid returning this string -- for
                            unix it is costly due to memory allocation;
                            much cheaper for vxWorks as no memory allocation.

 returns the number of bytes received
 
 ******************************************************************************/
int
readUDPSocket(UDPSocket *ss, 
	      char *buf, 
 	      int   bufLen,
	      char *inetAddr)
{ 
  socklen_t           sockAddrSize;            // size of socket address structure
  struct sockaddr_in  clientAddr;              // client's socket address
  int                 bufLenReceived;
  char               *inetAddrTemp;

  if (!ss->active) {
    printf("Socket not initialized\n");
    return FALSE;
  }

  // read data 
  sockAddrSize = sizeof (struct sockaddr_in); 
  if ((bufLenReceived = recvfrom (ss->sFd, buf, bufLen, 0, 
				  (struct sockaddr *) &clientAddr, 
				  &sockAddrSize)) == ERROR) { 
    printf("Error when reading from socket\n");
    return FALSE; 
  } 

  // convert inet address to dot notation
  if (inetAddr != NULL) {
#ifdef VX
    inet_ntoa_b (clientAddr.sin_addr, inetAddr); 
#else
    inetAddrTemp = inet_ntoa (clientAddr.sin_addr); 
    strcpy(inetAddr,inetAddrTemp);
#endif
  }

  return bufLenReceived;

} 

/*!*****************************************************************************
 *******************************************************************************
 \note  checkUDPSocket
 \date  May 2004
 
 \remarks 
 
 checks for available data in a socket
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in]     ss              : structure of server socket information

 returns number of bytes available for reading
 
 ******************************************************************************/
int
checkUDPSocket(UDPSocket *ss)
{
  int n_bytes;

  if (!ss->active) {
    printf("Socket not initialized\n");
    return FALSE;
  }

  ioctl(ss->sFd, FIONREAD, (unsigned long) &n_bytes);

  return n_bytes;
}


/*!*****************************************************************************
 *******************************************************************************
 \note  closeUDPSocket
 \date  May 2004
 
 \remarks 
 
 Closes a socket 
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in,out] ss                : structure of server socket information

 returns TRUE if all OK, otherwise FALSE
 
 ******************************************************************************/
int
closeUDPSocket(UDPSocket *ss)
{

  if (!ss->active) {
    printf("Socket not initialized\n");
    return FALSE;
  }

  ss->active = FALSE;
  if (close(ss->sFd) == ERROR)
    return FALSE;
  else
    return TRUE;
}


/*!*****************************************************************************
 *******************************************************************************
 \note  writeUDPSocket
 \date  May 2004
 
 \remarks 
 
 Write to a previously created socket.
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in]     ss              : structure of server socket information
 \param[in]     bufLen          : length of data buffer
 \param[in]     buf             : data buffer

 returns the number of bytes written
 
 ******************************************************************************/
int
writeUDPSocket(UDPSocket *ss, 
	       char *buf, 
	       int   bufLen)
{
  int bufLenSent;
  int sockAddrSize;      

  if (!ss->active) {
    printf("Socket not initialized\n");
    return FALSE;
  }

  // send request to server
  sockAddrSize = sizeof (struct sockaddr_in); 
  if ((bufLenSent = sendto (ss->sFd, (caddr_t) buf, bufLen, 0, 
			    (struct sockaddr *) &ss->serverAddr, sockAddrSize)) == ERROR) { 
    printf("Error: could not write to socket\n"); 
    return FALSE; 
  } 
  
  return bufLenSent; 
}

/*!*****************************************************************************
 *******************************************************************************
 \note  various test functions to check UDP communication
 \date  May 2004
 
 \remarks 
 
  
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 
 
 ******************************************************************************/
#define TESTPORT     5002
#define BUFLEN       16
#define SYS_CLK_RATE 5000 //!< this seems to be the max vxWorks can take
void
testUDPServer(char *name)
{
  union {
    char cbuf[BUFLEN*4];
    int  ibuf[BUFLEN];
  } buf;

  int  i;
  int  n_bytes;
  int  n_bytes_ready;
  int  bufLen=BUFLEN*4;
  int  count_packages=0;
  int  error_packages=0;
  int  expected_message = 1;
  double average_ready_size=0;
  double average_message_size = 0;
  UDPSocket serverTest;
  struct timespec ns;
  int save_sys_clk_rate;

#ifdef VX
  // set the clock to sufficient resolution
  save_sys_clk_rate=sysClkRateGet();
  sysClkRateSet(SYS_CLK_RATE);
#endif

  // create the UDP server
  if (!createUDPSocket(TESTPORT,name,TRUE,&serverTest))
    printf("Failed to create UDP Server\n");

  // receive message until user hits keyboard
  printf("Hit any key to terminate server ....");
  fflush(stdout);

  // empty input buffer such that keyboard interupt works
  ioctl(0 , FIONREAD, (unsigned long) (&n_bytes));
  for (i=1; i<=n_bytes; ++i)
    getchar();

  while (buf.ibuf[0] != -1) {

    // read data as much as availabe
    while ((n_bytes_ready=checkUDPSocket(&serverTest))) {
	n_bytes = readUDPSocket(&serverTest,buf.cbuf,bufLen,NULL); 
	average_message_size += n_bytes;
	++count_packages;
	if (expected_message != buf.ibuf[0] && buf.ibuf[0] != -1) {
	  ++error_packages;
	  expected_message = buf.ibuf[0];
	}
	++expected_message;

	// this break statement add robustness in vxWorks
	average_ready_size += n_bytes_ready;
	if (n_bytes_ready - n_bytes <= 0)
	  break;
    }

    // wait a moment for new data
    if (USE_SLEEP) {
      ns.tv_sec=0;
      ns.tv_nsec=10000;
#ifdef VX
      taskDelay(1);
#else
      nanosleep(&ns,NULL);
#endif
    }

    // check for keyboard interaction
    ioctl(0 , FIONREAD, (unsigned long) (&n_bytes));
    if (n_bytes != 0)
      break;

  }

  if (count_packages > 0) {
    average_message_size /= (double) count_packages;
    average_ready_size  /= (double) count_packages;
  }

  // close down the server
  closeUDPSocket(&serverTest);

  // print statistics
  printf("Package Statistics:\n");
  printf("     received          : %d\n",count_packages);
  printf("     errors            : %d\n",error_packages);
  printf("     ave.message size  : %f\n",average_message_size);
  printf("     ave.ready size    : %f\n",average_ready_size);

#ifdef VX
  // reset clock rate
  sysClkRateSet(save_sys_clk_rate);
#endif

}
  
void
testUDPClient(int n_bytes, char *name)
{
  union {
    char cbuf[BUFLEN*4];
    int  ibuf[BUFLEN];
  } buf;

  int i,j;
  int count=0;
  int  bufLen=BUFLEN*4;
  UDPSocket clientTest;
  struct timespec ns;
  int save_sys_clk_rate;

#ifdef VX
  // set the clock to sufficient resolution
  save_sys_clk_rate=sysClkRateGet();
  sysClkRateSet(SYS_CLK_RATE);
#endif

  // create the UDP client
  if (!createUDPSocket(TESTPORT,name,FALSE,&clientTest))
    printf("Failed to create UDP Client\n");

  while (count <  n_bytes) {
    buf.ibuf[0] = ++count;
    if (writeUDPSocket(&clientTest,buf.cbuf,4) != 4) {
      printf("Couldn't write all bytes\n");
    }

    // wait a moment
    if (USE_SLEEP) {
      ns.tv_sec=0;
      ns.tv_nsec=100000;
#ifdef VX
      taskDelay(1);
#else
      nanosleep(&ns,NULL);
#endif
    }
  }

  // a server termination message
  buf.ibuf[0] = -1;
  writeUDPSocket(&clientTest,buf.cbuf,4);

  // close down the server
  closeUDPSocket(&clientTest);

#ifdef VX
  // reset clock rate
  sysClkRateSet(save_sys_clk_rate);
#endif

}
