/*!=============================================================================
  ==============================================================================

  \file    UDP_communication.h

  \author  Stefan Schaal
  \date    May 2004, ported to CPP Jan, 2016

  ==============================================================================
  \remarks

  header file for UPD_communication.cpp

  ============================================================================*/

#ifndef UDP_COMMUNICATION_H_
#define UDP_COMMUNICATION_H_

#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <string.h>


// defines
#ifndef ERROR
#define ERROR (-1)
#endif


#define CLMCPORT1     5003
#define CLMCPORT2     5004
#define CLMCPORT3     5005
#define CLMCPORT4     5006


namespace udp_communication {

void
testUDPServer(char *name);

void
testUDPClient(int n_bytes, char *name);


class UDP_communication {
public:
	UDP_communication();

	virtual ~UDP_communication();

	int
	readUDPSocket(char *buf,
			int   bufLen,
			char *inetAddr);

	int
	closeUDPSocket(void);

	int
	writeUDPSocket(char *buf,
			int   bufLen);

	int
	checkUDPSocket(void);

	int
	makeUDPServer(int serverPortNum, char *serverName);

	int
	makeUDPClient(int socketPortNum, char *clientName);


	bool                active;          //!< socket active or not


private:
	struct sockaddr_in  socketAddr;      //!< server's socket address
	bool				is_server;
	int                 sFd;             //!< socket file descriptor


};

}

#endif /* UDP_COMMUNICATION_H_ */

