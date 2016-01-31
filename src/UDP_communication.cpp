/*!=============================================================================
  ==============================================================================

  \file    udp_communication.c

  \author  Stefan Schaal
  \date    May 2004, update Jan 2016 to CPP

  ==============================================================================
  \remarks

  A general implementation of udp communication, with switch between different
  operating systems. The code is adapted from vxWorks examples and Gordon
  Cheng's udp communication with robotic systems. Both client and server code
  are combined here.

  Note: there is a limit to the size of UDP message, usually 9216 bytes,
        as otherwise the socket buffer is exhausted.

  ============================================================================*/

#include <iostream>
#include <cstdlib>
#include "unistd.h"
#include "arpa/inet.h"
#include "string.h"
#include "sys/ioctl.h"
#include "netdb.h"

// my utilities library
#include "utility.h"

#include "UDP_communication.h"

// sleep or not?  If yes, make sure the timer has proper low resolution,
// but also that the system does not block due to too much polling. Note
// that this option is only needed for the test functions, and not for
//! using the regular API functions.
#define USE_SLEEP TRUE


namespace udp_communication {

/*!*****************************************************************************
 *******************************************************************************
 \note  Concstructor
 \date  Jan 2016

 \remarks

 A socket is created that can subsequently be used in communication. The
 socket can be either a server of client socket.

 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 \param[in]     serverPortNum     : which port to use
 \param[in]     serverName        : name or IP address of server -- pass "" to use
                                    localhost
 \param[in]     serverFlag        : TRUE/FALSE if socket is for server or not

 returns TRUE if all OK, otherwise FALSE

 ******************************************************************************/
UDP_communication::
UDP_communication(int serverPortNum,
		char *serverName,
		int serverFlag)
{

	int            sockAddrSize;   // size of socket address structure
	unsigned int   opts;           // socket options

	// if something goes wrong, the socket is inactive
	active = FALSE;

	// set up the local address
	sockAddrSize = sizeof (struct sockaddr_in);
	bzero ((char *) &serverAddr, sockAddrSize);
	serverAddr.sin_family  = AF_INET;
	serverAddr.sin_port    = htons (serverPortNum);

	if (strcmp(serverName,"")==0) {
		// use localhost as server
		serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	} else {
		// determine server from serverName
		struct hostent *host_ent;
		struct in_addr *addr_ptr;

		if ((host_ent = (struct hostent *) gethostbyname (serverName)) == NULL) {
			if ((serverAddr.sin_addr.s_addr = inet_addr(serverName)) == (unsigned int) ERROR) {
				printf("Error: unknown server name\n");
				return;
			}
		} else {
			addr_ptr = (struct in_addr *) *host_ent->h_addr_list;
			serverName = inet_ntoa (*addr_ptr);
			serverAddr.sin_addr.s_addr = inet_addr(serverName);
		}
	}

	// create a UDP-based socket
	if ((sFd = socket (AF_INET, SOCK_DGRAM, 0)) == ERROR) {
		printf("Error: could not create socket\n");
		return;
	}

	// make sure the socket uses blocking mode
	opts = FALSE;
	ioctl(sFd,FIONBIO,(unsigned long) (&opts));

	/*
#else

  fcntl(sFd,F_GETFL,&opts);
  //printf("socket opts = 0x%x\n",opts & O_NONBLOCK);
  //printf("socket opts = 0x%x\n",opts & O_NDELAY);
  opts = opts & ~O_NONBLOCK & ~O_NDELAY;
  fcntl(sFd,F_SETFL,&opts);
  //fcntl(sFd,F_GETFL,&opts);
  //printf("socket opts = 0x%x\n",opts & O_NONBLOCK);
  //printf("socket opts = 0x%x\n",opts & O_NDELAY);

#endif
	 */

	if (serverFlag) {
		// bind socket to local address
		if (bind (sFd, (struct sockaddr *) &serverAddr, sockAddrSize) == ERROR) {
			printf("Error: couldn't bind to socket address\n");
			close (sFd);
			return;
		}
	}

	active = TRUE;


}

/*!*****************************************************************************
 *******************************************************************************
 \note  Destructor
 \date  Jan 2016

 \remarks

 Close the socket.

 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 \param[in]     serverPortNum     : which port to use
 \param[in]     serverName        : name or IP address of server -- pass "" to use
                                    localhost
 \param[in]     serverFlag        : TRUE/FALSE if socket is for server or not

 returns TRUE if all OK, otherwise FALSE

 ******************************************************************************/
UDP_communication::
~UDP_communication() {

	if (active) {
		closeUDPSocket();
	}

}



/*!*****************************************************************************
 *******************************************************************************
 \note  readUDPSocket
 \date  May 2004

 \remarks

 Read from a previously created socket.

 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 \param[in]     bufLen          : length of data buffer
 \param[out]    buf             : data buffer
 \param[out]    inetAddr        : inet address from where data was received,
                      allocate as "char inetAddr[INET_ADDRSTRLENT]" for unix.
                      NOTE: pass NULL to avoid returning this string -- for
                            unix it is costly due to memory allocation.

 returns the number of bytes received

 ******************************************************************************/
int UDP_communication::
readUDPSocket(char *buf,
		int   bufLen,
		char *inetAddr)
{
	socklen_t           sockAddrSize;            // size of socket address structure
	struct sockaddr_in  clientAddr;              // client's socket address
	int                 bufLenReceived;
	char               *inetAddrTemp;

	if (!active) {
		printf("Socket not initialized\n");
		return FALSE;
	}

	// read data
	sockAddrSize = sizeof (struct sockaddr_in);
	if ((bufLenReceived = recvfrom (sFd, buf, bufLen, 0,
			(struct sockaddr *) &clientAddr,
			&sockAddrSize)) == ERROR) {
		printf("Error when reading from socket\n");
		return FALSE;
	}

	// convert inet address to dot notation
	if (inetAddr != NULL) {
		inetAddrTemp = inet_ntoa (clientAddr.sin_addr);
		strcpy(inetAddr,inetAddrTemp);
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

 	 none

 returns number of bytes available for reading

 ******************************************************************************/
int UDP_communication::
checkUDPSocket(void)
{
	int n_bytes;

	if (!active) {
		printf("Socket not initialized\n");
		return FALSE;
	}

	ioctl(sFd, FIONREAD, (unsigned long) &n_bytes);

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

	none

 returns TRUE if all OK, otherwise FALSE

 ******************************************************************************/
int UDP_communication::
closeUDPSocket(void)
{

	if (!active) {
		printf("Socket not initialized\n");
		return FALSE;
	}

	active = FALSE;
	if (close(sFd) == ERROR)
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

 \param[in]     bufLen          : length of data buffer
 \param[in]     buf             : data buffer

 returns the number of bytes written

 ******************************************************************************/
int UDP_communication::
writeUDPSocket(char *buf,
		int   bufLen)
{
	int bufLenSent;
	int sockAddrSize;

	if (!active) {
		printf("Socket not initialized\n");
		return FALSE;
	}

	// send request to server
	sockAddrSize = sizeof (struct sockaddr_in);
	if ((bufLenSent = sendto (sFd, (caddr_t) buf, bufLen, 0,
			(struct sockaddr *) &serverAddr, sockAddrSize)) == ERROR) {
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
	struct timespec ns;
	int save_sys_clk_rate;
	UDP_communication udp(TESTPORT,name,TRUE);

	// check whether socket creation was successful
	if (!udp.active) {
		printf("Failed to create UDP Server -- aborted\n");
		return;
	}

	// receive message until user hits keyboard
	printf("Hit any key to terminate server ....");
	fflush(stdout);

	// empty input buffer such that keyboard interupt works
	ioctl(0 , FIONREAD, (unsigned long) (&n_bytes));
	for (i=1; i<=n_bytes; ++i)
		getchar();

	while (buf.ibuf[0] != -1) {

		// read data as much as availabe
		while ((n_bytes_ready=udp.checkUDPSocket())) {
			n_bytes = udp.readUDPSocket(buf.cbuf,bufLen,NULL);
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
			nanosleep(&ns,NULL);
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
	udp.closeUDPSocket();

	// print statistics
	printf("Package Statistics:\n");
	printf("     received          : %d\n",count_packages);
	printf("     errors            : %d\n",error_packages);
	printf("     ave.message size  : %f\n",average_message_size);
	printf("     ave.ready size    : %f\n",average_ready_size);

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
	struct timespec ns;
	int save_sys_clk_rate;
	UDP_communication udp(TESTPORT,name,FALSE);


	// test for active socket
	if (!udp.active) {
		printf("Failed to create UDP Client\n");
		return;
	}

	while (count <  n_bytes) {
		buf.ibuf[0] = ++count;
		if (udp.writeUDPSocket(buf.cbuf,4) != 4) {
			printf("Couldn't write all bytes\n");
		}

		// wait a moment
		if (USE_SLEEP) {
			ns.tv_sec=0;
			ns.tv_nsec=100000;
			nanosleep(&ns,NULL);
		}
	}

	// a server termination message
	buf.ibuf[0] = -1;
	udp.writeUDPSocket(buf.cbuf,4);

	// close down the server
	udp.closeUDPSocket();


}

} // end of namespace
