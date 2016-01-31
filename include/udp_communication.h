/*!=============================================================================
  ==============================================================================

  \file    udp_communication.h

  \author  Stefan Schaal
  \date    May 2004

  ==============================================================================
  \remarks
  
  header file for udp_communication.c
  
  ============================================================================*/

#ifndef __udp_communication__
#define __udp_communication__

// defines
#ifndef ERROR
#define ERROR (-1)
#endif


#define CLMCPORT1     5003
#define CLMCPORT2     5004
#define CLMCPORT3     5005
#define CLMCPORT4     5006


typedef struct {
  int                 active;          //!< socket active or not
  struct sockaddr_in  serverAddr;      //!< server's socket address
  int                 sFd;             //!< socket file descriptor
} UDPSocket;


// function definitions
int
createUDPSocket(int serverPortNum, 
		char *serverName, 
		int serverFlag,
		UDPSocket *ss);

int
readUDPSocket(UDPSocket *ss, 
	      char *buf, 
	      int   bufLen,
	      char *inetAddr);

int
closeUDPSocket(UDPSocket *ss);

int
writeUDPSocket(UDPSocket *ss, 
	       char *buf, 
	       int   bufLen);

int
checkUDPSocket(UDPSocket *ss);

void
testUDPServer(char *name);

void
testUDPClient(int n_bytes, char *name);


#endif // __udp_communication__
