#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    

#include <OSCBundle.h>
#include <OSCBoards.h>

EthernetUDP Udp;
IPAddress ip(192, 168, 12, 230);//ip @ stichting
const unsigned int inPort  = 10000;
const unsigned int destPort = 11000;
byte mac[]                 = { 0x04, 0xE9, 0xE5, 0x03, 0x94, 0x5E }; // you can find this written on the board of some Arduino Ethernets or shields, for Teensy we have custom made code - implement it
int inputState = 0;

void setup() {
  //setup ethernet part
      // Open serial communications and wait for port to open:
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  digitalWrite(9, HIGH);   // end reset pulse

  //
  
  Ethernet.begin(mac,ip);
  

  delay(1000);
  Serial.begin(9600);
  Serial.println("Receive OSC");

  if(Udp.begin(inPort)){
    Serial.println("Succesful connection");
  }
  else{
    Serial.println("there are no sockets available to use");
    }

}

//reads and dispatches the incoming message
void loop(){ 
   OSCBundle bundleIN;
   int size;
 
   if( (size = Udp.parsePacket())>0)
   {
     while(size--)
       bundleIN.fill(Udp.read());

      if(!bundleIN.hasError())
        bundleIN.route("/input", InputReceived);
  }
}

void InputReceived(OSCMessage &msg, int addrOffset){
  inputState =  msg.getInt(0);
  //OSCMessage msgOUT("/input");
  Serial.print("Input received: "); Serial.println(inputState);
}


