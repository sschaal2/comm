// system includes
#include <cstring>
#include <iostream>

// local includes
#include "ethercat_communication.h"

using ethercat_communication::EthercatCommunication;

// minmal test of class methods
bool EthercatCommunicationTest () {
  EthercatCommunication ethercat_mod;

  ethercat_mod.InitEthercat("enp2s0");

  // a simple communication loop

  for(int i = 1; i <= 100; i++) {
    ethercat_mod.RunEthercat();

    if(ethercat_mod.wkc_ >= ethercat_mod.expectedWKC_) {

      printf("Processdata cycle %4d, WKC %d , O:", i, ethercat_mod.wkc_);
      
      for(int j = 0 ; j < ethercat_mod.Obytes_; j++) {
	printf(" %2.2x", *(ec_slave[0].outputs + j));
      }

      printf(" I:");
      for(int j = 0 ; j < ethercat_mod.Ibytes_; j++) {
	printf(" %2.2x", *(ec_slave[0].inputs + j));
      }
      printf("\n");

    }
    osal_usleep(1000);
  }
  
  return true;
}

int
main(int argc, char**argv) {
  return EthercatCommunicationTest();
}

