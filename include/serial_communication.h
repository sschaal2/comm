/*!=============================================================================
  ==============================================================================

  \file    serial_communication.h

  \author  Stefan Schaal
  \date    Sept. 2011, ported to cpp Aug. 2020

  ==============================================================================

  supports serial_communication.cpp

  ============================================================================*/


#ifndef _SERIAL_COMMUNICTION_
#define _SERIAL_COMMUNICTION_

#include "termios.h"

#define BAUD9K    B9600
#define BAUD19K   B19200
#define BAUD38K   B38400
#define BAUD115K  B115200

#define SERIALPORT1 "/dev/ttyS0"
#define SERIALPORT2 "/dev/ttyS1"
#define SERIALPORT3 "/dev/ttyS2"
#define SERIALPORT4 "/dev/ttyS3"

namespace serial_communication {

  class SerialCommunication {
  public:
    SerialCommunication(char *fname,
			int   baud,
			int   mode);

    virtual ~SerialCommunication();

    int
    clearSerial();
    
    int
    readSerial(int   n_bytes,
		char *buffer);

    int
    writeSerial(int  n_bytes,
		char *buffer);

    int
    checkSerial();

  private:

    int  fd_;
    int  baud_;
    int  mode_;
    bool active_;    //!< serial port active or not

  };

}

#endif  // _SERIAL_COMMUNICTION_
