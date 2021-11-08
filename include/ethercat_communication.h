/*!=============================================================================
  ==============================================================================

  \file    ethercat_communication.h

  \author  Stefan Schaal
  \date    October 2021

  ==============================================================================

  supports ethercat_communication.cpp

  ============================================================================*/


#ifndef _ETHERCAT_COMMUNICTION_
#define _ETHERCAT_COMMUNICTION_

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// from SOEM package
#include "ethercat.h"

#define EC_TIMEOUTMON 500

namespace ethercat_communication {

  class EthercatCommunication {
  public:
    EthercatCommunication();

    virtual ~EthercatCommunication();

    int
    InitEthercat(char *ifname);

    bool active_;    //!< socket active or not
    int                expectedWKC_;
    int                Obytes_;
    int                Ibytes_;
    volatile int       wkc_;
    

    int
    RunEthercat();
    
  private:

    int
    CheckEthercat( void *time_fptr );

    char               ifname_[100];  //socket interface name
    char               IOmap_[4096];
    OSAL_THREAD_HANDLE thread1_;
    boolean            needlf_;
    uint8              currentgroup_ = 0;
    
  };

}

#endif  // _ETHERCAT_COMMUNICTION_
