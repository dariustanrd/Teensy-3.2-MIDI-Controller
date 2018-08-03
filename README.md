# Teensy 3.2 MIDI Controller / Macropad
A custom MIDI Controller / Macropad based on the [Teensy 3.2 microcontroller](https://www.pjrc.com/store/teensy32.html) for [MIDI2LR](https://github.com/rsjaffe/MIDI2LR/) integration and multi-keystroke macro execution.

Images of the final product can be seen either in this [gallery](ADDLINKHERE) or at the end of this README.

### Inspiration for this project

Firstly, I was facing issues with editing a large number of images in Adobe Lightroom as it often takes a long time and it was not very intuitive to edit with my laptop's touchpad. Therefore, I stumbled upon MIDI2LR which uses MIDI Controllers normally used for music production to act as a physical device to change values in Lightroom.

Secondly, I wanted to create a macropad to perform tasks that would normally require multiple keypresses on the keyboard, such as while coding, CAD-ing etc., with a single press of a key.


Therefore, this controller serves 2 main purposes:
1. To send USB MIDI CC / NOTE messages in order to interface with MIDI2LR.
  
2. To send USB keyboard presses in order to chain multiple keypresses into a single macro key.

### Build Process
1. Mechanical & Electrical Fabrication
   
    I first purchased [this aluminium enclosure](https://item.taobao.com/item.htm?id=542717489532&_u=t2dmg8j26111) from Taobao, an online Chinese retailer, and milled out the holes for the encoders and the key switches to pass through. I designed a grid for the key switches in SolidWorks and fabricated it with a water jet machine out of 2mm Aluminium (which should have been 1.5mm Alu in hindsight). I then sand blasted the aluminium components and spray painted them black for aesthetic purposes.

   In order to cater for multiple functions on a single board, I added a SPDT Toggle Switch to connect to 2 digital pins on the Teensy to act as a Layer control mechanism to increase or decrease the current layer.

   As the Teensy has limited I/O slots and I wanted a large number of mechanical keyboard switches on my device to cater to the size of the enclosure I purchased, I wired up the Gateron Brown keyboard switches as well as the Encoder pushbuttons in a 4 x 7 matrix, with the help of the [Cherry MX breakout board](https://learn.sparkfun.com/tutorials/cherry-mx-switch-breakout-hookup-guide), similar to that of a regular keyboard matrix.

   Lastly, I wired all the components up to my Teensy 3.2, including 8 Incremental Rotary Encoders ([PEC11R-4215K-S0024](http://sg.element14.com/webapp/wcs/stores/servlet/ProductDisplay?catalogId=10001&langId=65&urlRequestType=Base&partNumber=2663524&storeId=10191)), 1 Potentiometer, 1 WS2812B LED strip, 1 SPDT toggle switch, my 4 x 7 switch matrix and 1 lonely Encoder pushbutton that could not fit on my switch matrix.

2. Code
   
   The first challenge that I tackled was interfacing with the Incremental Rotary Encoders. I used the [Teensy Encoder.h library](https://www.pjrc.com/teensy/td_libs_Encoder.html) for its ease of use and its interrupt capabilities since all digital pins on the Teensy 3.2 are interrupt capable.

   For the MIDI portion of the project, as my main purpose of using MIDI messages was to interface with MIDI2LR, I decided to send MIDI CC messages when I rotated my rotary encoders and MIDI NOTE messages when I pressed my mechanical key switches or encoder pushbuttons.

   Thus, when I rotated the encoders, which are mapped to specific CC numbers, the CC value would increment or decrement correspondingly on the MIDI channel that is defined by the current Layer that the device is in (controlled by the toggle switch).

   For the 4 x 7 keyswitch matrix, I setup a state machine in order to properly set the states of each individual switch be it IDLE, PRESSED, HOLD or RELEASED. Then, if the layer is currently on the lightroom layer, whether the switch is pressed or held, different MIDI NOTE messages will be sent. Otherwise, a keyboard macro will be sent instead. To send the keyboard press, the inbuilt [Teensy keyboard libary](https://www.pjrc.com/teensy/td_keyboard.html) is used. However, as there is no USB type for "Serial + MIDI + Keyboard", I edited the "Serial + MIDI" USB type in usb_desc.h to include keyboard functionality as well (inspiration from [here](https://forum.pjrc.com/threads/23942-Using-Serial-and-MIDI-USB-types-at-the-same-time).)

   Lastly, to be able to physically ascertain what layer the device is currently in, I changed the colour of the WS2812B LED strip using the [Teensy WS2812Serial.h library](https://github.com/PaulStoffregen/WS2812Serial) and added a cycling rainbow pattern for fun in the highest layer (which actually causes the teensy to be unable to perform other tasks, hence that layer is technically unusable but its just there for the sake of it).

### Images
Image ?:
![Image of ?](http://)

Image ?:
![Image of ?](http://)

