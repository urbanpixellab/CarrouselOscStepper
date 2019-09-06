// Desigened to run on Teensy 3.2 @ 72 MHz
// Copyright Stichting z25.org

// IMPORTANT: Do not disconnect motors or microswitches from the controller board
// as it will unrepairably destroy the motor controller hardware.

// Note: due to time constraints, this implementation is in some
// parts very verbose and could optimized. Advantage is that last
// minute debugging requires less advanced programming skills.

// The value of forward has to be set each time command is received.
// The value of forward at the end of a command is not to be used.

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

#include <OSCBundle.h>
#include <OSCBoards.h>

/* Retrieve Ethernet MAC from Teensy 3 */
class mac_addr : public Printable {
  public:
    uint8_t m[6];

    mac_addr() : m( {
      0
    }) {
      read(0xe, 0);
      read(0xf, 3);
    }

    void read(uint8_t word, uint8_t loc) {
      FTFL_FCCOB0 = 0x41;
      FTFL_FCCOB1 = word;
      FTFL_FSTAT = FTFL_FSTAT_CCIF;
      while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) {
      }
      *(m + loc) =   FTFL_FCCOB5;
      *(m + loc + 1) = FTFL_FCCOB6;
      *(m + loc + 2) = FTFL_FCCOB7;
    }

    virtual size_t printTo(Print & p) const {
      size_t count = 0;
      for (uint8_t i = 0; i < 6; ++i) {
        if (i != 0) count += p.print(":");
        count += p.print((*(m + i) & 0xF0) >> 4, 16);
        count += p.print(*(m + i) & 0x0F, 16);
      }
      return count;
    }

    /* Get integer for last four bytes of MAC address */
    virtual unsigned int to_int() const {
      unsigned int value = 0;
      //NOTE More than four bytes cannot be stored in an integer.
      for (uint8_t i = 2; i < 6; ++i) {
        value += *(m + i);
        if ( i != 5) value = value << 8;
      }
      return value;
    }

    virtual const uint8_t* get_array() const {
      return m;
    }
};

// Lookup table from last four bytes of MAC Addressmac address to last byte of IP address and controller ID.
unsigned int tuple1[] = {0xE50907BB, 35, 1};
unsigned int tuple2[] = {0xE508E6B9, 36, 2};
unsigned int tuple3[] = {0xE5070842, 37, 3};
unsigned int tuple4[] = {0xE5070839, 38, 4};
unsigned int tuple5[] = {0xE5070899, 39, 5};
unsigned int tuple6[] = {0xE5070819, 40, 6};

unsigned int *ipLookup[] = {tuple1, tuple2, tuple3, tuple4, tuple5, tuple6};

EthernetUDP udp;
#define inPort 9000

int inputState = 0;
int stepsMotor;
//stepper stuff

#define dirPinA   2
#define stepPinA  3
#define sleepPinA 4

#define dirPinB   5
#define stepPinB  6
#define sleepPinB 7

#define dirPinC   17
#define stepPinC  18
#define sleepPinC 19

#define dirPinD   14
#define stepPinD  15
#define sleepPinD 16

// at least 10 steps otherwise the interval for on off is to short
#define stepsInLoop 12
#define delayInStep 1000

#define STOP   0 // stop
#define INIT   1 // initialize (go to min, go to max, go to min and determine range)
#define GOMIN  2 // go to minumum (also uninitialized)
#define GOMAX  3 // go to maximum (also uninitialized)
#define GOTOP  4 // go to percentage
#define OSCIL  5 // oscillate linearly with percentage

#define UNINITIALIZED 10
#define INITIALIZING1 11
#define INITIALIZING2 12
#define INITIALIZING3 13
#define INITIALIZED   14

int positionA;
int positionB;
int positionC;
int positionD;

int statusA = UNINITIALIZED;
int statusB = UNINITIALIZED;
int statusC = UNINITIALIZED;
int statusD = UNINITIALIZED;

int commandA = STOP;
int commandB = STOP;
int commandC = STOP;
int commandD = STOP;

int rangeA = 0;
int rangeB = 0;
int rangeC = 0;
int rangeD = 0;

int destinationA;
int destinationB;
int destinationC;
int destinationD;

int oscillationBeginA;
int oscillationBeginB;
int oscillationBeginC;
int oscillationBeginD;

int oscillationEndA;
int oscillationEndB;
int oscillationEndC;
int oscillationEndD;

bool forwardA;
bool forwardB;
bool forwardC;
bool forwardD;

bool beginA = false;
bool beginB = false;
bool beginC = false;
bool beginD = false;

bool endA = false;
bool endB = false;
bool endC = false;
bool endD = false;

//multiplexer
const int channel[] = {21, 22, 23};
//the input pin - demux output
const int inputPin = 20;

mac_addr mac;
bool initialized = false;
unsigned int mac_int = mac.to_int();

void setup() {
  delay(1000);
  Serial.begin(9600);

  uint8_t ip_address[4] = {192, 168, 12, 0};
  uint8_t controllerID = 0;
  for (uint8_t i = 0; i < 6; ++i) { //FIXME warning max index ipLookup is hardcoded
    if (ipLookup[i][0] == mac_int) {
      ip_address[3] = ipLookup[i][1];
      controllerID = ipLookup[i][2];
      initialized = true;
      break;
    }
  }

  if (initialized) {
    Serial.print("INFO: For MAC address ");
    Serial.print(mac);
    Serial.print(" found IP address ");
    Serial.print(ip_address[0]);
    Serial.print(".");
    Serial.print(ip_address[1]);
    Serial.print(".");
    Serial.print(ip_address[2]);
    Serial.print(".");
    Serial.print(ip_address[3]);
    Serial.print(" for controller ");
    Serial.println(controllerID);
  } else {
    Serial.print("ERROR: No IP address found for MAC address ");
    Serial.println(mac);
    initialized = false;
    return;

  }

  // Configure ethernet
  // Open serial communications and wait for port to open:
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);   // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // de-select WIZ820io
  digitalWrite(9, HIGH);  // end reset pulse

  Ethernet.begin(mac.get_array(), IPAddress(ip_address[0], ip_address[1], ip_address[2], ip_address[3]));
  delay(1000);

  if (udp.begin(inPort)) {
    Serial.println("INFO: Configured OSC connection");
  }
  else {
    Serial.println("ERROR: No sockets available to use");
    initialized = false;
    return;
  }

  // Configre stepper
  pinMode(stepPinA,  OUTPUT);
  pinMode(dirPinA,   OUTPUT);
  pinMode(sleepPinA, OUTPUT);

  pinMode(stepPinB,  OUTPUT);
  pinMode(dirPinB,   OUTPUT);
  pinMode(sleepPinB, OUTPUT);

  pinMode(stepPinC,  OUTPUT);
  pinMode(dirPinC,   OUTPUT);
  pinMode(sleepPinC, OUTPUT);

  pinMode(stepPinD,  OUTPUT);
  pinMode(dirPinD,   OUTPUT);
  pinMode(sleepPinD, OUTPUT);

  digitalWrite(sleepPinB, LOW); // disable
  digitalWrite(sleepPinD, LOW); // disable
  digitalWrite(sleepPinA, LOW); // disable
  digitalWrite(sleepPinC, LOW); // disable

  // Configure multiplex
  pinMode(inputPin, INPUT);
  pinMode(channel[0], OUTPUT);
  pinMode(channel[1], OUTPUT);
  pinMode(channel[2], OUTPUT);
}

void loop()
{
  if (!initialized) {
    delay(10000);
    Serial.print("ERROR: No IP address found for MAC address ");
    Serial.println(mac);
    return;
  }

  OSCBundle bundleIN;
  int size;

  if ((size = udp.parsePacket()) > 0) {
    while (size--) {
      bundleIN.fill(udp.read());
    }
    //    Serial.println("INFO: Received data"); //for debug
    if (!bundleIN.hasError()) {
      bundleIN.route("/input", InputReceived);
      bundleIN.route("/27", MobileInputReceived);
    }
  }

  for (int muxChannel = 0; muxChannel < 8; muxChannel++) {
    muxWrite(muxChannel);
    if (muxChannel == 0) {
      endA = digitalRead(inputPin);
    } else if (muxChannel == 1) {
      endC = digitalRead(inputPin);
    } else if (muxChannel == 2) {
      endB = digitalRead(inputPin);
    } else if (muxChannel == 3) {
      endD = digitalRead(inputPin);
    } else if (muxChannel == 4) {
      beginA = digitalRead(inputPin);
    } else if (muxChannel == 5) {
      beginC = digitalRead(inputPin);
    } else if (muxChannel == 6) {
      beginB = digitalRead(inputPin);
    } else if (muxChannel == 7) {
      beginD = digitalRead(inputPin);
    }
  }

  goOneStep();
}

void muxWrite(int whichChannel)
{
  for (int inputPin = 0; inputPin < 3; inputPin++)
  {
    int pinState = bitRead(whichChannel, inputPin);
    // turn the pin on or off:
    digitalWrite(channel[inputPin], pinState);
  }
}

void MobileInputReceived(OSCMessage &msg, int addrOffset) {
  int v = msg.getInt(0);
  Serial.print("INFO: Received mobile OSC v=");
  Serial.println(v); // note that other values are used! only 0, 1 and 3.

  if (v == 0) {
    if (commandA != STOP) {
      commandA = STOP;
      Serial.println("INFO: Motor 1 is stopped");
    } else {
      Serial.println("INFO: Motor 1 was already stopped");
    }
    if (commandB != STOP) {
      commandB = STOP;
      Serial.println("INFO: Motor 2 is stopped");
    } else {
      Serial.println("INFO: Motor 2 was already stopped");
    }
    if (commandC != STOP) {
      commandC = STOP;
      Serial.println("INFO: Motor 3 is stopped");
    } else {
      Serial.println("INFO: Motor 3 was already stopped");
    }
    if (commandD != STOP) {
      commandD = STOP;
      Serial.println("INFO: Motor 4 is stopped");
    } else {
      Serial.println("INFO: Motor 4 was already stopped");
    }
  } else if (v == 1) {
    destinationA = 0;
    forwardA = false;
    if (beginA) {
      positionA = destinationA; // realign
      commandA = STOP;
      Serial.println("INFO: Motor 1 was already at minimum");
    } else {
      commandA = GOMIN;
    }
    destinationB = 0;
    forwardB = false;
    if (beginB) {
      positionB = destinationB; // realign
      commandB = STOP;
      Serial.println("INFO: Motor 2 was already at minimum");
    } else {
      commandB = GOMIN;
    }
    destinationC = 0;
    forwardC = false;
    if (beginC) {
      positionC = destinationC; // realign
      commandC = STOP;
      Serial.println("INFO: Motor 3 was already at minimum");
    } else {
      commandC = GOMIN;
    }
    destinationD = 0;
    forwardD = false;
    if (beginD) {
      positionD = destinationD; // realign
      commandD = STOP;
      Serial.println("INFO: Motor 4 was already at minimum");
    } else {
      commandD = GOMIN;
    }
  } else if (v == 3) {
    destinationA = 100;
    forwardA = true;
    if (endA) {
      positionA = destinationA; // realign
      commandA = STOP;
      Serial.println("INFO: Motor 1 was already at maximum");
    } else {
      commandA = GOMAX;
    }
    destinationB = 100;
    forwardB = true;
    if (endB) {
      positionB = destinationB; // realign
      commandB = STOP;
      Serial.println("INFO: Motor 2 was already at maximum");
    } else {
      commandB = GOMAX;
    }
    destinationC = 100;
    forwardC = true;
    if (endC) {
      positionC = destinationC; // realign
      commandC = STOP;
      Serial.println("INFO: Motor 3 was already at maximum");
    } else {
      commandC = GOMAX;
    }
    destinationD = 100;
    forwardD = true;
    if (endD) {
      positionD = destinationD; // realign
      commandD = STOP;
      Serial.println("INFO: Motor 4 was already at maximum");
    } else {
      commandD = GOMAX;
    }
  }
}

void InputReceived(OSCMessage &msg, int addrOffset) {
  int value = msg.getInt(0);
  int c = value / 10000;
  if (c < 0 || c > 5) c = 0;
  int m = (value / 1000) % 10;
  if (m < 0 || m > 4) m = 0;
  int a = value % 1000;
  if (a < 0) a = 0;
  if (a > 100) a = 100;
  Serial.print("INFO: Received OSC command=");
  Serial.print(c);
  Serial.print(" motor=");
  Serial.print(m);
  Serial.print(" argument=");
  Serial.println(a);

  if (c == STOP) {
    if (m == 0 || m == 1) {
      if (statusA != INITIALIZED && statusA != UNINITIALIZED) {
        Serial.println("INFO: Motor 1 initialization is aborted");
        statusA = UNINITIALIZED;
      }
      if (commandA != c) {
        commandA = c;
        Serial.println("INFO: Motor 1 is stopped");
      } else {
        Serial.println("INFO: Motor 1 was already stopped");
      }
    }
    if (m == 0 || m == 2) {
      if (statusB != INITIALIZED && statusB != UNINITIALIZED) {
        Serial.println("INFO: Motor 2 initialization is aborted");
        statusB = UNINITIALIZED;
      }
      if (commandB != c) {
        commandB = c;
        Serial.println("INFO: Motor 2 is stopped");
      } else {
        Serial.println("INFO: Motor 2 was already stopped");
      }
    }
    if (m == 0 || m == 3) {
      if (statusC != INITIALIZED && statusC != UNINITIALIZED) {
        Serial.println("INFO: Motor 3 initialization is aborted");
        statusC = UNINITIALIZED;
      }
      if (commandC != c) {
        commandC = c;
        Serial.println("INFO: Motor 3 is stopped");
      } else {
        Serial.println("INFO: Motor 3 was already stopped");
      }
    }
    if (m == 0 || m == 4) {
      if (statusD != INITIALIZED && statusD != UNINITIALIZED) {
        Serial.println("INFO: Motor 4 initialization is aborted");
        statusD = UNINITIALIZED;
      }
      if (commandD != c) {
        commandD = c;
        Serial.println("INFO: Motor 4 is stopped");
      } else {
        Serial.println("INFO: Motor 4 was already stopped");
      }
    }
  } else if (c == INIT) {
    if (m == 0 || m == 1) {
      if (statusA != INITIALIZED && statusA != UNINITIALIZED) {
        Serial.println("INFO: Motor 1 previously running initialization is aborted");
        statusA = UNINITIALIZED;
      }
      rangeA = 0;
      destinationA = 0;
      forwardA = false;
      statusA = INITIALIZING1;
      commandA = c;
    }
    if (m == 0 || m == 2) {
      if (statusB != INITIALIZED && statusB != UNINITIALIZED) {
        Serial.println("INFO: Motor 2 previously running initialization is aborted");
        statusB = UNINITIALIZED;
      }
      rangeB = 0;
      destinationB = 0;
      forwardB = false;
      statusB = INITIALIZING1;
      commandB = c;
    }
    if (m == 0 || m == 3) {
      if (statusC != INITIALIZED && statusC != UNINITIALIZED) {
        Serial.println("INFO: Motor 3 previously running initialization is aborted");
        statusC = UNINITIALIZED;
      }
      rangeC = 0;
      destinationC = 0;
      forwardC = false;
      statusC = INITIALIZING1;
      commandC = c;
    }
    if (m == 0 || m == 4) {
      if (statusD != INITIALIZED && statusD != UNINITIALIZED) {
        Serial.println("INFO: Motor 4 previously running initialization is aborted");
        statusD = UNINITIALIZED;
      }
      rangeD = 0;
      destinationD = 0;
      forwardD = false;
      statusD = INITIALIZING1;
      commandD = c;
    }
  } else if (c == GOMIN) {
    if (m == 0 || m == 1) {
      if (statusA != INITIALIZED && statusA != UNINITIALIZED) {
        Serial.println("INFO: Motor 1 initialization is aborted");
        statusA = UNINITIALIZED;
      }
      destinationA = 0;
      forwardA = false;
      if (beginA) {
        positionA = destinationA; // realign
        commandA = STOP;
        Serial.println("INFO: Motor 1 was already at minimum");
      } else {
        commandA = c;
      }
    }
    if (m == 0 || m == 2) {
      if (statusB != INITIALIZED && statusB != UNINITIALIZED) {
        Serial.println("INFO: Motor 2 initialization is aborted");
        statusB = UNINITIALIZED;
      }
      destinationB = 0;
      forwardB = false;
      if (beginB) {
        positionB = destinationB; // realign
        commandB = STOP;
        Serial.println("INFO: Motor 2 was already at minimum");
      } else {
        commandB = c;
      }
    }
    if (m == 0 || m == 3) {
      if (statusC != INITIALIZED && statusC != UNINITIALIZED) {
        Serial.println("INFO: Motor 3 initialization is aborted");
        statusC = UNINITIALIZED;
      }
      destinationC = 0;
      forwardC = false;
      if (beginC) {
        positionC = destinationC; // realign
        commandC = STOP;
        Serial.println("INFO: Motor 3 was already at minimum");
      } else {
        commandC = c;
      }
    }
    if (m == 0 || m == 4) {
      if (statusD != INITIALIZED && statusD != UNINITIALIZED) {
        Serial.println("INFO: Motor 4 initialization is aborted");
        statusD = UNINITIALIZED;
      }
      destinationD = 0;
      forwardD = false;
      if (beginD) {
        positionD = destinationD; // realign
        commandD = STOP;
        Serial.println("INFO: Motor 4 was already at minimum");
      } else {
        commandD = c;
      }
    }
  } else if (c == GOMAX) {
    if (m == 0 || m == 1) {
      if (statusA != INITIALIZED && statusA != UNINITIALIZED) {
        Serial.println("INFO: Motor 1 initialization is aborted");
        statusA = UNINITIALIZED;
      }
      destinationA = 100;
      forwardA = true;
      if (endA) {
        positionA = destinationA; // realign
        commandA = STOP;
        Serial.println("INFO: Motor 1 was already at maximum");
      } else {
        commandA = c;
      }
    }
    if (m == 0 || m == 2) {
      if (statusB != INITIALIZED && statusB != UNINITIALIZED) {
        Serial.println("INFO: Motor 2 initialization is aborted");
        statusB = UNINITIALIZED;
      }
      destinationB = 100;
      forwardB = true;
      if (endB) {
        positionB = destinationB; // realign
        commandB = STOP;
        Serial.println("INFO: Motor 2 was already at maximum");
      } else {
        commandB = c;
      }
    }
    if (m == 0 || m == 3) {
      if (statusC != INITIALIZED && statusC != UNINITIALIZED) {
        Serial.println("INFO: Motor 3 initialization is aborted");
        statusC = UNINITIALIZED;
      }
      destinationC = 100;
      forwardC = true;
      if (endC) {
        positionC = destinationC; // realign
        commandC = STOP;
        Serial.println("INFO: Motor 3 was already at maximum");
      } else {
        commandC = c;
      }
    }
    if (m == 0 || m == 4) {
      if (statusD != INITIALIZED && statusD != UNINITIALIZED) {
        Serial.println("INFO: Motor 4 initialization is aborted");
        statusD = UNINITIALIZED;
      }
      destinationD = 100;
      forwardD = true;
      if (endD) {
        positionD = destinationD; // realign
        commandD = STOP;
        Serial.println("INFO: Motor 4 was already at maximum");
      } else {
        commandD = c;
      }
    }
  } else if (c == GOTOP) {
    if (m == 0 || m == 1) {
      if (statusA < INITIALIZED) {
        Serial.println("WARNING: Motor 1 had not yet been initialized, hence did not move to percentage");
      } else {
        destinationA = (a * rangeA) / 100;
        if (positionA == destinationA) {
          commandA = STOP;
          Serial.print("INFO: Motor 1 was already at percentage ");
          Serial.println(a);
        } else {
          if (destinationA > positionA) {
            forwardA = true;
          } else {
            forwardA = false;
          }
          commandA = c;
        }
      }
    }
    if (m == 0 || m == 2) {
      if (statusB < INITIALIZED) {
        Serial.println("WARNING: Motor 2 had not yet been initialized, hence did not move to percentage");
      } else {
        destinationB = (a * rangeB) / 100;
        if (positionB == destinationB) {
          commandB = STOP;
          Serial.print("INFO: Motor 2 was already at percentage ");
          Serial.println(a);
        } else {
          if (destinationB > positionB) {
            forwardB = true;
          } else {
            forwardB = false;
          }
          commandB = c;
        }
      }
    }
    if (m == 0 || m == 3) {
      if (statusC < INITIALIZED) {
        Serial.println("WARNING: Motor 3 had not yet been initialized, hence did not move to percentage");
      } else {
        destinationC = (a * rangeC) / 100;
        if (positionC == destinationC) {
          commandC = STOP;
          Serial.print("INFO: Motor 3 was already at percentage ");
          Serial.println(a);
        } else {
          if (destinationC > positionC) {
            forwardC = true;
          } else {
            forwardC = false;
          }
          commandC = c;
        }
      }
    }
    if (m == 0 || m == 4) {
      if (statusD < INITIALIZED) {
        Serial.println("WARNING: Motor 4 has not yet been initialized, hence did not move to percentage");
      } else {
        destinationD = (a * rangeD) / 100;
        if (positionD == destinationD) {
          commandD = STOP;
          Serial.print("INFO: Motor 4 was already at percentage ");
          Serial.println(a);
        } else {
          if (destinationD > positionD) {
            forwardD = true;
          } else {
            forwardD = false;
          }
          commandD = c;
        }
      }
    }
  } else if (c == OSCIL) {
    if (m == 0 || m == 1) {
      if (statusA < INITIALIZED || commandA != STOP) {
        Serial.println("WARNING: Motor 1 has not yet been initialized or stopped, hence did not oscillate");
      } else {
        oscillationBeginA = positionA;
        oscillationEndA = (a * rangeA) / 100;
        if (oscillationBeginA == oscillationEndA) {
          commandA = STOP;
          Serial.println("WARNING: Motor 1 cannot oscillate on same position, hence did not oscillate");
        } else {
          destinationA = oscillationEndA;
          commandA = c;
        }
      }
    }
    if (m == 0 || m == 2) {
      if (statusB < INITIALIZED || commandB != STOP) {
        Serial.println("WARNING: Motor 2 has not yet been initialized or stopped, hence did not oscillate");
      } else {
        oscillationBeginB = positionB;
        oscillationEndB = (a * rangeB) / 100;
        if (oscillationBeginB == oscillationEndB) {
          commandB = STOP;
          Serial.println("WARNING: Motor 2 cannot oscillate on same position, hence did not oscillate");
        } else {
          destinationB = oscillationEndB;
          commandB = c;
        }
      }
    }
    if (m == 0 || m == 3) {
      if (statusC < INITIALIZED || commandC != STOP) {
        Serial.println("WARNING: Motor 3 has not yet been initialized or stopped, hence did not oscillate");
      } else {
        oscillationBeginC = positionC;
        oscillationEndC = (a * rangeC) / 100;
        if (oscillationBeginC == oscillationEndC) {
          commandC = STOP;
          Serial.println("WARNING: Motor 3 cannot oscillate on same position, hence did not oscillate");
        } else {
          destinationC = oscillationEndC;
          commandC = c;
        }
      }
    }
    if (m == 0 || m == 4) {
      if (statusD < INITIALIZED || commandD != STOP) {
        Serial.println("WARNING: Motor 4 has not yet been initialized or stopped, hence did not oscillate");
      } else {
        oscillationBeginD = positionD;
        oscillationEndD = (a * rangeD) / 100;
        if (oscillationBeginD == oscillationEndD) {
          commandD = STOP;
          Serial.println("WARNING: Motor 4 cannot oscillate on same position, hence did not oscillate");
        } else {
          destinationD = oscillationEndD;
          commandD = c;
        }
      }
    }
  }
}

void goOneStep()
{
  //
  // A
  //
  if (commandA == INIT) {
    if (statusA == INITIALIZING1) {
      // first extreme: first minimum
      if (beginA) {
        statusA = INITIALIZING2;
        forwardA = true;
        destinationA = 100;
      }
    }
    if (statusA == INITIALIZING2) {
      // second extreme: first maximum
      if (endA) {
        statusA = INITIALIZING3;
        forwardA = false;
        destinationA = 0;
      }
    }
    if (statusA == INITIALIZING3) {
      // third extreme: second minimum, end of initialization
      if (beginA) {
        positionA = 0;
        rangeA--; // subtract one for error margin microswitches w.r.t. stepsInLoop
        Serial.print("INFO: Initialized motor 1, available range is ");
        Serial.println(rangeA);
        statusA = INITIALIZED;
        commandA = STOP;
      }
    }
    if (statusA == INITIALIZING3) {
      rangeA++;
    }
  } else if (commandA == GOMIN) {
    if (beginA) {
      positionA = destinationA;
      commandA = STOP;
      Serial.println("INFO: Moved to minimum");
    } else {
      if (statusA == INITIALIZED) {
        positionA--; // in case command is stopped
        if (positionA < 0) {
          positionA = 0;
          commandA = STOP;
        }
      }
    }
  } else if (commandA == GOMAX) {
    if (endA) {
      positionA = destinationA;
      commandA = STOP;
      Serial.println("INFO: Moved to maximum");
    } else {
      if (statusA == INITIALIZED) {
        positionA++; // in case command is stopped
        if (positionA > 100) {
          positionA = 100;
          commandA = STOP;
        }
      }
    }
  } else if (commandA == GOTOP) {
    if (positionA == destinationA) {
      commandA = STOP;
      Serial.println("INFO: Moved to percentage");
    } else {
      if (forwardA) {
        positionA++;
      } else {
        positionA--;
      }
    }
  } else if (commandA == OSCIL) {
    if (positionA == oscillationBeginA) {
      destinationA = oscillationEndA;
    } else if (positionA == oscillationEndA) {
      destinationA = oscillationBeginA;
    }
    if (destinationA > positionA) {
      forwardA = true;
      positionA++;
    } else {
      forwardA = false;
      positionA--;
    }
  }

  //
  // B
  //
  if (commandB == INIT) {
    if (statusB == INITIALIZING1) {
      // first extreme: first minimum
      if (beginB) {
        statusB = INITIALIZING2;
        forwardB = true;
        destinationB = 100;
      }
    }
    if (statusB == INITIALIZING2) {
      // second extreme: first maximum
      if (endB) {
        statusB = INITIALIZING3;
        forwardB = false;
        destinationB = 0;
      }
    }
    if (statusB == INITIALIZING3) {
      // third extreme: second minimum, end of initialization
      if (beginB) {
        positionB = 0;
        rangeB--; // subtract one for error margin microswitches w.r.t. stepsInLoop
        Serial.print("INFO: Initialized motor 1, available range is ");
        Serial.println(rangeB);
        statusB = INITIALIZED;
        commandB = STOP;
      }
    }
    if (statusB == INITIALIZING3) {
      rangeB++;
    }
  } else if (commandB == GOMIN) {
    if (beginB) {
      positionB = destinationB;
      commandB = STOP;
      Serial.println("INFO: Moved to minimum");
    } else {
      if (statusB == INITIALIZED) {
        positionB--; // in case command is stopped
        if (positionB < 0) {
          positionB = 0;
          commandB = STOP;
        }
      }
    }
  } else if (commandB == GOMAX) {
    if (endB) {
      positionB = destinationB;
      commandB = STOP;
      Serial.println("INFO: Moved to maximum");
    } else {
      if (statusB == INITIALIZED) {
        positionB++; // in case command is stopped
        if (positionB > 100) {
          positionB = 100;
          commandB = STOP;
        }
      }
    }
  } else if (commandB == GOTOP) {
    if (positionB == destinationB) {
      commandB = STOP;
      Serial.println("INFO: Moved to percentage");
    } else {
      if (forwardB) {
        positionB++;
      } else {
        positionB--;
      }
    }
  } else if (commandB == OSCIL) {
    if (positionB == oscillationBeginB) {
      destinationB = oscillationEndB;
    } else if (positionB == oscillationEndB) {
      destinationB = oscillationBeginB;
    }
    if (destinationB > positionB) {
      forwardB = true;
      positionB++;
    } else {
      forwardB = false;
      positionB--;
    }
  }

  //
  // C
  //
  if (commandC == INIT) {
    if (statusC == INITIALIZING1) {
      // first extreme: first minimum
      if (beginC) {
        statusC = INITIALIZING2;
        forwardC = true;
        destinationC = 100;
      }
    }
    if (statusC == INITIALIZING2) {
      // second extreme: first maximum
      if (endC) {
        statusC = INITIALIZING3;
        forwardC = false;
        destinationC = 0;
      }
    }
    if (statusC == INITIALIZING3) {
      // third extreme: second minimum, end of initialization
      if (beginC) {
        positionC = 0;
        rangeC--; // subtract one for error margin microswitches w.r.t. stepsInLoop
        Serial.print("INFO: Initialized motor 1, available range is ");
        Serial.println(rangeC);
        statusC = INITIALIZED;
        commandC = STOP;
      }
    }
    if (statusC == INITIALIZING3) {
      rangeC++;
    }
  } else if (commandC == GOMIN) {
    if (beginC) {
      positionC = destinationC;
      commandC = STOP;
      Serial.println("INFO: Moved to minimum");
    } else {
      if (statusC == INITIALIZED) {
        positionC--; // in case command is stopped
        if (positionC < 0) {
          positionC = 0;
          commandC = STOP;
        }
      }
    }
  } else if (commandC == GOMAX) {
    if (endC) {
      positionC = destinationC;
      commandC = STOP;
      Serial.println("INFO: Moved to maximum");
    } else {
      if (statusC == INITIALIZED) {
        positionC++; // in case command is stopped
        if (positionC > 100) {
          positionC = 100;
          commandC = STOP;
        }
      }
    }
  } else if (commandC == GOTOP) {
    if (positionC == destinationC) {
      commandC = STOP;
      Serial.println("INFO: Moved to percentage");
    } else {
      if (forwardC) {
        positionC++;
      } else {
        positionC--;
      }
    }
  } else if (commandC == OSCIL) {
    if (positionC == oscillationBeginC) {
      destinationC = oscillationEndC;
    } else if (positionC == oscillationEndC) {
      destinationC = oscillationBeginC;
    }
    if (destinationC > positionC) {
      forwardC = true;
      positionC++;
    } else {
      forwardC = false;
      positionC--;
    }
  }

  //
  // D
  //
  if (commandD == INIT) {
    if (statusD == INITIALIZING1) {
      // first extreme: first minimum
      if (beginD) {
        statusD = INITIALIZING2;
        forwardD = true;
        destinationD = 100;
      }
    }
    if (statusD == INITIALIZING2) {
      // second extreme: first maximum
      if (endD) {
        statusD = INITIALIZING3;
        forwardD = false;
        destinationD = 0;
      }
    }
    if (statusD == INITIALIZING3) {
      // third extreme: second minimum, end of initialization
      if (beginD) {
        positionD = 0;
        rangeD--; // subtract one for error margin microswitches w.r.t. stepsInLoop
        Serial.print("INFO: Initialized motor 1, available range is ");
        Serial.println(rangeD);
        statusD = INITIALIZED;
        commandD = STOP;
      }
    }
    if (statusD == INITIALIZING3) {
      rangeD++;
    }
  } else if (commandD == GOMIN) {
    if (beginD) {
      positionD = destinationD;
      commandD = STOP;
      Serial.println("INFO: Moved to minimum");
    } else {
      if (statusD == INITIALIZED) {
        positionD--; // in case command is stopped
        if (positionD < 0) {
          positionD = 0;
          commandD = STOP;
        }
      }
    }
  } else if (commandD == GOMAX) {
    if (endD) {
      positionD = destinationD;
      commandD = STOP;
      Serial.println("INFO: Moved to maximum");
    } else {
      if (statusD == INITIALIZED) {
        positionD++; // in case command is stopped
        if (positionD > 100) {
          positionD = 100;
          commandD = STOP;
        }
      }
    }
  } else if (commandD == GOTOP) {
    if (positionD == destinationD) {
      commandD = STOP;
      Serial.println("INFO: Moved to percentage");
    } else {
      if (forwardD) {
        positionD++;
      } else {
        positionD--;
      }
    }
  } else if (commandD == OSCIL) {
    if (positionD == oscillationBeginD) {
      destinationD = oscillationEndD;
    } else if (positionD == oscillationEndD) {
      destinationD = oscillationBeginD;
    }
    if (destinationD > positionD) {
      forwardD = true;
      positionD++;
    } else {
      forwardD = false;
      positionD--;
    }
  }

  // make the motherfucking steps, doe het alsjeblieft voor milo

  // using B, D, A, C in order to improve power distribution
  // each pin write is only 1 or 2 microseconds, no need to compensate

  if (commandB) digitalWrite(sleepPinB, HIGH); // enable
  if (commandD) digitalWrite(sleepPinD, HIGH); // enable
  if (commandA) digitalWrite(sleepPinA, HIGH); // enable
  if (commandC) digitalWrite(sleepPinC, HIGH); // enable

  if (commandB) digitalWrite(dirPinB, forwardB);
  if (commandD) digitalWrite(dirPinD, forwardD);
  if (commandA) digitalWrite(dirPinA, forwardA);
  if (commandC) digitalWrite(dirPinC, forwardC);

  for (int i = 0; i < stepsInLoop; i++) {
    if (commandB) digitalWrite(stepPinB, HIGH);
    if (commandD) digitalWrite(stepPinD, HIGH);
    if (commandA) digitalWrite(stepPinA, HIGH);
    if (commandC) digitalWrite(stepPinC, HIGH);
    delayMicroseconds(delayInStep);
    if (commandB) digitalWrite(stepPinB, LOW);
    if (commandD) digitalWrite(stepPinD, LOW);
    if (commandA) digitalWrite(stepPinA, LOW);
    if (commandC) digitalWrite(stepPinC, LOW);
    delayMicroseconds(delayInStep);
  }

  digitalWrite(sleepPinB, LOW); // disable
  digitalWrite(sleepPinD, LOW); // disable
  digitalWrite(sleepPinA, LOW); // disable
  digitalWrite(sleepPinC, LOW); // disable

}
