#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    

#include <OSCBundle.h>
#include <OSCBoards.h>

EthernetUDP Udp;
IPAddress ip(192, 168, 12, 35);//ip @ stichting
const unsigned int inPort  = 10000;
const unsigned int destPort = 11000;
byte mac[]                 = { 0x04, 0xE9, 0xE5, 0x03, 0x94, 0x5E }; // you can find this written on the board of some Arduino Ethernets or shields, for Teensy we have custom made code - implement it
char ReplyBuffer[] = "onTarget";        // a string to send back


int inputState = 0;
int stepsMotor;
//stepper stuff
const int dirPin = 2;//8
const int stepPin = 3;//9
const int sleepPin= 4;//3
const int stepsPerRevolution = 200;
int stepCountA;// the count where i am
int stepTargetA; // the target to go to



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
  //stepper
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  stepCountA = 0;
  stepTargetA = 0;

}

//reads and dispatches the incoming message
void loop(){ 
   OSCBundle bundleIN;
   int size;
 
   if( (size = Udp.parsePacket())>0)
   {
     while(size--)
       bundleIN.fill(Udp.read());
//       IPAddress remote = Udp.remoteIP();
       //Serial.println("received"); for debug
       if(!bundleIN.hasError()) bundleIN.route("/input", InputReceived);
  }

  goOneStep();
  //Serial.print(stepCountA + " "); Serial.println(stepTargetA);
  

}

void InputReceived(OSCMessage &msg, int addrOffset){
  inputState =  msg.getInt(0);
  //Serial.println(inputState);
  //OSCMessage msgOUT("/input");
  Serial.print("Input received: "); Serial.println(inputState);
  //goSteps(inputState,TRUE);
  stepTargetA = inputState; //set the target
  Serial.print("Go TO :");Serial.println(stepTargetA);
}

void goOneStep()
{
  bool direct = false;
  if(stepTargetA == stepCountA)
  {
    //implement osc feedback
    return;

  }
  else if(stepTargetA < stepCountA)
  {
    digitalWrite(sleepPin, HIGH); //enable
    direct = false;
    stepCountA--;
  }
  else
  {
    digitalWrite(sleepPin, HIGH); //enable
    direct = true;
    stepCountA++;
  }
  
  digitalWrite(dirPin, direct);
  for(int i = 0;i < 10;i++) //at least 10 steps otherwise the interval for on off is to short
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(sleepPin, LOW); //disable
}


/*
void goSteps(int steps,bool dir)
{
  //this should go into the main loop, otherwise the step function is blocking everythinbg
  // best the main loop goes one step per run if the position is different than the other
  // and the sleep is toggled also if he hs to else off
  digitalWrite(sleepPin, HIGH); //enable
  //digitalWrite(dirPin, HIGH);
  digitalWrite(dirPin, dir);

  // Spin motor slowly
  for(int x = 0; x < steps; x++)
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(2000);
    stepCountA++;
  }
  digitalWrite(sleepPin, LOW); //disable
}
*/
