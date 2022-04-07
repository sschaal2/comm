 /*!=============================================================================
  ==============================================================================

  \file    ethercat_communication.cpp

  \author  Stefan Schaal
  \date    October 2021

  ==============================================================================
  \remarks

  Based on ethercat master package SOEM, this is a very lightweight ethercat
  driver, and for this purpose not very generally programmed.

  ============================================================================*/


#include <iostream>
#include <cstdlib>
#include "ethercat_communication.h"

// local variables 

// global variables 

// local functions

namespace ethercat_communication {

/*!*****************************************************************************
 *******************************************************************************
 \note  EthercatCommuniction
 \date  October 2021

 \remarks

 Prepares ethercat communication. Initialization is done by initEthercat().


 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 none

 ******************************************************************************/
EthercatCommunication::
EthercatCommunication()
{

  // the ethercat communication is not active until properly initialized
  active_ = false;
  
}

/*!*****************************************************************************
 *******************************************************************************
 \note  ~EthercatCommuniction
 \date  October 2021

 \remarks

 Closes the ethercat socket


 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 none

 ******************************************************************************/
EthercatCommunication::
~EthercatCommunication()
{

  if (active_) {
    // stop SOEM, close socket
    ec_close();
    active_ = false;
  }
}

/*!*****************************************************************************
 *******************************************************************************
\note  CheckEthercat
\date  Oct 2021
   
\remarks 

Function to check ethercat operations (copied from SOEM examples).

*******************************************************************************
Function Parameters: [in]=input,[out]=output

\param[in]     time_fptr:  pointer to timing function to be used

******************************************************************************/
int
EthercatCommunication::CheckEthercat( void *time_fptr )
{
  int slave;

  if( active_ && ((wkc_ < expectedWKC_) || ec_group[currentgroup_].docheckstate)) {
    
    if (needlf_) {
      needlf_ = FALSE;
      printf("\n");
    }
    
    // one ore more slaves are not responding 
    ec_group[currentgroup_].docheckstate = FALSE;
    ec_readstate();
    for (slave = 1; slave <= ec_slavecount; slave++) {
      
      if ((ec_slave[slave].group == currentgroup_) && (ec_slave[slave].state != EC_STATE_OPERATIONAL)) { 
	ec_group[currentgroup_].docheckstate = TRUE;
	
	if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR)) {
	  
	  printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
	  ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
	  ec_writestate(slave);
	  
	} else if (ec_slave[slave].state == EC_STATE_SAFE_OP) {
	  
	  printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
	  ec_slave[slave].state = EC_STATE_OPERATIONAL;
	  ec_writestate(slave);
	  
	} else if(ec_slave[slave].state > EC_STATE_NONE) {
	  
	  if (ec_reconfig_slave(slave, EC_TIMEOUTMON)) {
	    ec_slave[slave].islost = FALSE;
	    printf("MESSAGE : slave %d reconfigured\n",slave);
	  }
	  
	} else if(!ec_slave[slave].islost) {
	  
	  // re-check state
	  ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
	  if (ec_slave[slave].state == EC_STATE_NONE) {
	    ec_slave[slave].islost = TRUE;
	    printf("ERROR : slave %d lost\n",slave);
	  }
	  
	}
      }
      
      
      if (ec_slave[slave].islost) {
	
	if(ec_slave[slave].state == EC_STATE_NONE) {
	  
	  if (ec_recover_slave(slave, EC_TIMEOUTMON)) {
	    ec_slave[slave].islost = FALSE;
	    printf("MESSAGE : slave %d recovered\n",slave);
	  }
	  
	} else {
	  
	  ec_slave[slave].islost = FALSE;
	  printf("MESSAGE : slave %d found\n",slave);
	  
	}
	
      }
      
    }
    
    if(!ec_group[currentgroup_].docheckstate){
      printf("OK : all slaves resumed OPERATIONAL.\n");
    } else {
      return FALSE;
    }
    
  }

  return TRUE;

}

/*!*****************************************************************************
 *******************************************************************************
\note  InitEthercat
\date  Oct 2021
   
\remarks 

initializes the running ethercat communication

*******************************************************************************
Function Parameters: [in]=input,[out]=output

\param[in]     ifname: interface name to use for socket

******************************************************************************/
int EthercatCommunication::
InitEthercat(const char *ifname)
{

  int i, j, chk;  

  // initialise SOEM, bind socket to ifname
  if (ec_init(ifname)) {

    printf("ec_init on %s succeeded.\n",ifname);
    strcpy(ifname_,ifname);

    // find and auto-config slaves
    if ( ec_config_init(FALSE) > 0 ) {
      printf("%d slaves found and configured.\n",ec_slavecount);
      
      ec_config_map(&IOmap_);

      ec_configdc();

      printf("Slaves mapped, state to SAFE_OP.\n");
      /* wait for all slaves to reach SAFE_OP state */
      ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4);

      Obytes_ = ec_slave[0].Obytes;
      Ibytes_ = ec_slave[0].Ibytes;

      printf("Request operational state for all slaves\n");
      expectedWKC_ = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
      printf("Calculated workcounter %d\n", expectedWKC_);
      ec_slave[0].state = EC_STATE_OPERATIONAL;
      
      // send one valid process data to make outputs in slaves happy
      ec_send_processdata();
      ec_receive_processdata(EC_TIMEOUTRET);

      // request OP state for all slaves */
      ec_writestate(0);
      chk = 200;
      
      // wait for all slaves to reach OP state
      do {
	ec_send_processdata();
	ec_receive_processdata(EC_TIMEOUTRET);
	ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
      }  while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

      
	
      if (ec_slave[0].state == EC_STATE_OPERATIONAL ) {

	printf("Operational state reached for all slaves.\n");
	active_ = TRUE;

      } else {

	active_ = FALSE;
	printf("Not all slaves reached operational state.\n");
	ec_readstate();
	for(i = 1; i<=ec_slavecount ; i++) {
	  if(ec_slave[i].state != EC_STATE_OPERATIONAL) {
	    printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
		   i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
	  }
	}
	
      }

    } else {

      printf("No slaves found!\n");
      
    }

  } else {

      ec_adaptert * adapter = NULL;

      printf ("\nAvailable interfaces:\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL) {
	printf ("    - %s  (%s)\n", adapter->name, adapter->desc);
	adapter = adapter->next;
      }
      ec_free_adapters(adapter);
      
      printf("No socket connection on %s\nDo you have the correct permissions on the interface?\n",ifname);

  }

  
  return active_;
}

/*!*****************************************************************************
 *******************************************************************************
\note  RunEthercat
\date  Oct 2021
   
\remarks 

runs one send/receive on the ethercat

*******************************************************************************
Function Parameters: [in]=input,[out]=output

none

******************************************************************************/
int EthercatCommunication::
RunEthercat()
{

  if (active_) {
    
    // ethercat I/O (assumes that the input to the ethercat communication
    // has been configured before appropriately
    ec_send_processdata();
    wkc_ = ec_receive_processdata(EC_TIMEOUTRET);
    
    if(wkc_ >= expectedWKC_) {
      
      return TRUE;
      
    } else {
      
      // check error handling
      if (!EthercatCommunication::CheckEthercat((void*) &ctime)) {
	
	return FALSE;
	
      } else {
	
	// try again
	ec_send_processdata();
	wkc_ = ec_receive_processdata(EC_TIMEOUTRET);
	
	if(wkc_ >= expectedWKC_)
	  return TRUE;
	else
	  return FALSE;
      }
      
    }
    
  } else {

    printf("Ethercat on interface >%s< is not active\n",ifname_);
    return FALSE;

  }
  

}
/*!*****************************************************************************
 *******************************************************************************
\note  SendEthercat
\date  Oct 2021
   
\remarks 

runs a send on the ethercat

*******************************************************************************
Function Parameters: [in]=input,[out]=output

none

******************************************************************************/
int EthercatCommunication::
SendEthercat()
{

  if (active_) {
    
    // ethercat I/O (assumes that the input to the ethercat communication
    // has been configured before appropriately
    ec_send_processdata();

    return TRUE;
    
  } else {

    printf("Ethercat on interface >%s< is not active\n",ifname_);
    return FALSE;

  }
  

}
/*!*****************************************************************************
 *******************************************************************************
\note  ReceiveEthercat
\date  Oct 2021
   
\remarks 

receives the information of the previous send command

*******************************************************************************
Function Parameters: [in]=input,[out]=output

none

******************************************************************************/
int EthercatCommunication::
ReceiveEthercat()
{

  if (active_) {
    
    wkc_ = ec_receive_processdata(EC_TIMEOUTRET);
    
    if(wkc_ >= expectedWKC_) {
      
      return TRUE;
      
    } else {
      
      // check error handling
      if (!EthercatCommunication::CheckEthercat((void*) &ctime)) {
	
	return FALSE;
	
      } else {
	
	// try again
	ec_send_processdata();
	wkc_ = ec_receive_processdata(EC_TIMEOUTRET);
	
	if(wkc_ >= expectedWKC_)
	  return TRUE;
	else
	  return FALSE;
      }
      
    }
    
  } else {

    printf("Ethercat on interface >%s< is not active\n",ifname_);
    return FALSE;

  }
  

}

}

