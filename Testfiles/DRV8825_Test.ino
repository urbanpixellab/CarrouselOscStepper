// Define pin connections & motor's steps per revolution
const int dirPin = 8;
const int stepPin = 9;
const int dirInputPin = 4; // the pin for the direction
const int stepInputPin = 7;//the pins for step and direction
const int sleepPin= 3;

const int stepsPerRevolution = 200;

void setup()
{
  // Declare pins as Outputs
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  pinMode(sleepPin, OUTPUT);
  pinMode(dirInputPin, INPUT);
  pinMode(stepInputPin, INPUT); //go
  Serial.begin(9600);
}
void loop()
{
  // Set motor direction clockwise
  digitalWrite(dirPin, HIGH);
  digitalWrite(sleepPin, HIGH); //enable

  //Serial.print(digitalRead(microPinA));
  //Serial.print(" : ");
  //Serial.println(digitalRead(microPinB));

  // Spin motor slowly
  for(int x = 0; x < stepsPerRevolution * 3; x++)
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(2000);
  }

  
  delay(1000); // Wait a second
  
  // Set motor direction counterclockwise
  digitalWrite(dirPin, LOW);


  // Spin motor quickly
  for(int x = 0; x < stepsPerRevolution * 3; x++)
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(sleepPin, LOW); //disable
  delay(5000); // Wait 5 second
}
