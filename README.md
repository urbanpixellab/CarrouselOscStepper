# CarrouselOscStepper

## implemented
* receiving osc for target of stepper
* spinning to this position in loop per 10 steps, otherwise the moter gets stuck if i loop every step
* the motor checks self if hi is on target
* through this we can always send him a position and he goes as fast to this position

* multiplexing the 8 microswitches is working
* sending osc feedback if target reached or microswitch is pressed

## osc messages by now for control
* address "/input" followed by int for position in range 0-100
* teensy osc receives on port 10000 and sends on port 11000 to receive osc ip(master)

## teensy pin mapping
* final after pcb design
* 




## Some Thoughts about OSC messages
* the address is followed by 4 int values for the steppers 
* "/p" for position		
* "/c" for calibrate



* also maybe including the temp sensor to give information about the tempinside the box

## Some Links
* circuits ... are followinh
* https://lastminuteengineers.com/drv8825-stepper-motor-driver-arduino-tutorial/
* https://www.makerguides.com/drv8825-stepper-motor-driver-arduino-tutorial/
* https://github.com/CNMAT/OSC
* https://www.google.com/search?q=teensy+wiz820io&source=lnms&tbm=isch&sa=X&ved=0ahUKEwiooqeDwbfjAhVDDewKHfJNA8EQ_AUIECgB&biw=1487&bih=886#imgrc=dpOUuCkrrY9njM:
* https://forum.arduino.cc/index.php?topic=415724.0
* 
## by now we are using the easydriver v4 but it will be replaced by the drv8825
* another version from urbanpixellab https://github.com/urbanpixellab/ProtoDraai2019


