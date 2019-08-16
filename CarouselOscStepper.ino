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


int inputState = 0;
int stepsMotor;
//stepper stuff
const int dirPin = 2;//8
const int stepPin = 3;//9
const int sleepPin= 4;//3
const int stepsPerRevolution = 200;
int stepCountA;// the count where i am
int stepTargetA; // the target to go to

//outgoing messages
char *oscAddresses[] = {"/reached", "/alert"};

//multiplexer
const int channel[] = {21, 22, 23};
//the input pin - demux output
const int inputPin = 20;

//temporary for test
int dirBtnPin = 6;
int goBtnPin = 7;


OSCBundle bundleOUT;
bool sendTargetA;//we reached the target
//converts the pin to an osc address



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
  sendTargetA = false;
  //now the multiplex
  pinMode(inputPin,INPUT);
  pinMode(channel[0],OUTPUT);
  pinMode(channel[1],OUTPUT);
  pinMode(channel[2],OUTPUT);

  //for test buttons
  pinMode(dirBtnPin,INPUT);
  pinMode(goBtnPin,INPUT);
}

//reads and dispatches the incoming message
void loop()
{ 
  //we dont use osc for the test with the two buttons on 6 and 7
  /*
   OSCBundle bundleIN;
   int size;
 
   if( (size = Udp.parsePacket())>0)
   {
     while(size--)
       bundleIN.fill(Udp.read());
       //Serial.println("received"); for debug
       if(!bundleIN.hasError()) bundleIN.route("/input", InputReceived);
  }
  goOneStep();
  //check the multiplexer if a microswitch is hit
  for (int muxChannel = 0; muxChannel < 8; muxChannel++) 
  {
    //set the channel pins based on the channel you want
    //LED on - high - 1000 milliseconds delay
    muxWrite(muxChannel);
    if(muxChannel == 0 && digitalRead(inputPin) == true)
    {
      //send osc
      sendOsc(1,1);
      Serial.println("alert");
    }
  }*/
  
  //testwise button and microswitch
  
  //if microswitch hits something we end the loop
  bool nodStop = false;
  for (int muxChannel = 0; muxChannel < 8; muxChannel++) 
  {
    muxWrite(muxChannel);
    if(digitalRead(inputPin) == true)
    {
      nodStop = true;
      //Serial.println("alert");
    }
  }
  
  if(digitalRead(goBtnPin) && !nodStop) goButton(digitalRead(dirBtnPin));
}

void sendOsc(int addresID,int value)
{
      //send osc
      OSCBundle bndl;
      bndl.add(oscAddresses[addresID]).add(value);//which switch is reached
      Udp.beginPacket(Udp.remoteIP(), destPort);
          bndl.send(Udp); // send the bytes to the SLIP stream
      Udp.endPacket(); // mark the end of the OSC Packet
      bndl.empty(); // empty the bundle to free room for a new one
}

void muxWrite(int whichChannel) 
{
  for (int inputPin = 0; inputPin < 3; inputPin++) 
  {
    int pinState = bitRead(whichChannel, inputPin);
    // turn the pin on or off:
    digitalWrite(channel[inputPin],pinState);
  }
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
  bool direct = false;//the stepper direction
  if(stepTargetA == stepCountA)
  {
    if(sendTargetA == false)
    {
      //implement osc feedback
      //implement that it ios only once not forever!!!!!
      sendOsc(0,stepTargetA);
    }
    sendTargetA == true;
    return;
  }
  
  if(stepTargetA < stepCountA)
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
  
  sendTargetA == false; // we have a moving
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

void goButton(bool dirIn)
{
  digitalWrite(sleepPin, HIGH); //enable
  digitalWrite(dirPin, dirIn);
  for(int i = 0;i < 10;i++) //at least 10 steps otherwise the interval for on off is to short
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(sleepPin, LOW); //disable
}
