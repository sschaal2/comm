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
	UDP_communication(int serverPortNum,
			char *serverName,
			int serverFlag);

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

	int                 active;          //!< socket active or not


private:
	struct sockaddr_in  serverAddr;      //!< server's socket address
	int                 sFd;             //!< socket file descriptor


};

}

#endif /* UDP_COMMUNICATION_H_ */

