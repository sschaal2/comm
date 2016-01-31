/*!=============================================================================
  ==============================================================================

  \file    udp_main.c

  \author  Stefan Schaal
  \date    May 2004

  ==============================================================================
  \remarks
  
  test program to start a server or client udp communication
  
  ============================================================================*/
  
/* global headers */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"

/* local headers */
#include "utility.h"
#include "udp_communication.h"
  
/* local functions */

  
/*!*****************************************************************************
 *******************************************************************************
 \note  main
 \date  July 1998
 
 \remarks 
 
 entry program
 
 *******************************************************************************
 Function Parameters: [in]=input,[out]=output
 
 \param[in]     argc : number of elements in argv
 \param[in]     argv : array of argc character strings
 
 ******************************************************************************/
int 
main(int argc, char**argv)

{
  int  i,j;
  int  n_bytes = 10000;
  char name[100];

  if (argc == 2 && argv[1][1] == 's') {
    name[0]='\0';
  } else if (argc < 3) {
    printf("Usage: xudpTest [-s | -c] [hostName | hostIP] [n_bytes]\n");
    return FALSE;
  } else {
    strcpy(name,&(argv[2][0]));
  }

  switch (argv[1][1]) {
  case 's':
    testUDPServer(name);
    break;

  case 'c':
    if (argc == 4)
      sscanf(&(argv[3][0]),"%d",&n_bytes);
    testUDPClient(n_bytes,name);
    break;

  default:
    printf("Pass -s for server, or -c for client communication\n");
  }
	
  return TRUE;
}


