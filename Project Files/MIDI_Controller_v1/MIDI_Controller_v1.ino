#include <Encoder.h>

#define MATCOL1 24
#define MATCOL2 25
#define MATCOL3 26
#define MATCOL4 27
#define MATCOL5 28
#define MATCOL6 29
#define SW_UP 30
#define SW_DWN 31
#define LED1 32
#define MATCOL7 33

#define RESET 0

const int num_enc = 8;
const int num_keys = 29;

long enc_pos_new[num_enc]; //used to be -999
long enc_pos_old[num_enc]; //used to be -999
int enc_updateCCflag[num_enc];

byte midiChannel = 0;
byte enc_CC[num_enc];
byte enc_CCVal[num_enc];
byte key_CC[num_keys];
byte key_CCVal[num_keys];

volatile int layer = 1;
volatile int sensitivity_val = 0;
bool SENS_KEY = true;

Encoder enc1(0, 1);
Encoder enc2(2, 3);
Encoder enc3(4, 5);
Encoder enc4(6, 7);
Encoder enc5(8, 9);
Encoder enc6(10, 11);
Encoder enc7(14, 15);
Encoder enc8(16, 17);

//Main Functions*******************************************************************************************************************//
void setup() {
  Serial.begin(9600);
  SetupPin();
  SetupCC();
  reset_enc();
}

void loop() {
  delay(50); //to see the serial
  layerSelect();
  
  switch(layer) {
    //switching cases should change function of enc and keypad
    //and function just changes the values for the enc & keypad routines
      case 1:
        macropad();
        //Serial.print("Macropad Mode\n");
      break;
      
      case 2:
        lightroom();
      break;

      case 3:
        //Premiere Pro Edit Layer
      break;

      case 4:
        //Coding Layer
      break;

      default:
        macropad();
    }

  //send all the CC values here? or if not in indiv functions 
  //--> send in indiv functions cos diff case will have diff cc val attached
}

//Functions Below*******************************************************************************************************************//
void SetupPin() {
  pinMode(SW_UP, INPUT);
  pinMode(SW_DWN, INPUT); //should they be INPUT_PULLUP?
  //digitalWrite(REDLED, LOW);
}
void SetupCC(){
  for (int i = 0; i < num_enc; i++) {
    enc_CC[i] = i;
    enc_CCVal[i] = 64; //set CCVal to be middle upon init.
  }
  for (int i = 0; i < num_keys; i++) {
    key_CC[i] = i + num_enc;
  }
}
void layerSelect() {
  //check if SW_up press, layer++. if SW_dwn press, layer--
  if (SW_UP) {
    if (layer != 10) {
      layer++;
    } else {
      layer = 1;
    }
    //Serial.print("Layer Up\n");
  }
  if (SW_DWN) {
    if (layer != 1) {
      layer--;
    } else {
      layer = 10;
    }
    //Serial.print("Layer Down\n");
  }
  midiChannel = layer;
}
void reset_enc(){
  enc1.write(RESET);
  enc2.write(RESET); //enc1.write(newPosition); //Use this for reset enc Set the accumulated position to a new number.
  enc3.write(RESET);
  enc4.write(RESET);
  enc5.write(RESET);
  enc6.write(RESET);
  enc7.write(RESET);
  enc8.write(RESET);
  Serial.print("ENC Reset\n");
}

void read_enc() {
  enc_pos_new[0] = enc1.read();
  enc_pos_new[1] = enc2.read();
  enc_pos_new[2] = enc3.read();
  enc_pos_new[3] = enc4.read();
  enc_pos_new[4] = enc5.read();
  enc_pos_new[5] = enc6.read();
  enc_pos_new[6] = enc7.read();
  enc_pos_new[7] = enc8.read();

  //check if old and new positions of each encoder need to change CC val
  for (int i = 0; i < num_enc; i++) {
    enc_updateCCflag[i] = check_pos(enc_pos_new[i], enc_pos_old[i]);
    enc_pos_old[i] = enc_pos_new[i];
    enc_CCVal[i] = update_CCVal(enc_updateCCflag[i], enc_CCVal[i]);
  }
}
int check_pos(long encPos_new, long encPos_old) {
  if (encPos_new == encPos_old) {
    return 0;
  }
  if(encPos_new %4 != 0) { //MIDIEncoder.cpp uses 4, more sensitive than if use 5/detent --> turn more to get same change
    return 0;
  }
  if (encPos_new > encPos_old) {
    return 1; //positive direction, flag increase
  } else {
    return -1; //negative direction, flag decrease
  }
}
byte update_CCVal(long flag, byte CCVal) {
  SENS_KEY = true; //HARDCODE, CHANGE WHEN GOT KEYPAD
  check_sens();

  if (sensitivity_val == 1){
    if (flag == 1) {
      if (CCVal != 127) {
        CCVal += sensitivity_val;
      } else {
        CCVal = 0;
      }
    }
    else if (flag == -1) {
      if (CCVal != 0) {
        CCVal -= sensitivity_val;
      } else {
        CCVal = 127;
      }
    }
  } else {
    if (flag == 1) {
      if (CCVal <= 127) {
        CCVal += sensitivity_val;
      } else {
        CCVal = 0;
      }
    }
    else if (flag == -1) {
      if ((CCVal <= 127 )) {
        CCVal -= sensitivity_val;
      } else {
        CCVal = 127;
      }
    }
  }

  return CCVal;
}

void send_enc_CC() {
  //send CC values for all enc if they have an update
  for (int i = 0; i < num_enc; i++) {
    if (enc_updateCCflag[i]!=0) {
      usbMIDI.sendControlChange(enc_CC[i], enc_CCVal[i], midiChannel);
      Serial.print("CC Change = ");
      Serial.print(enc_CCVal[i]);
      Serial.println();
    }
  }
}

void check_sens(){
  if (SENS_KEY) {
    sensitivity_val = 1;
  } else {
    sensitivity_val = 5;
  }
}

void macropad() {
  read_enc();
  send_enc_CC();
  //keypad_routine(/*input the values to change here?*/);
  //change LED
  //change enc CC value?
  //change keymapping
  //change pot?
  //keypad can use usbMIDI.sendNoteOn(note, velocity, channel) or usbMIDI.sendNoteOff(note, velocity, channel)
  //usbMIDI.sendControlChange(control number, value, channel)
}

void lightroom() {
  read_enc();
  send_enc_CC();
}

void keypad_routine() {
  //if row and col at the same time, then that button pressed
  //do i need to set either row or col to be ON as output, then other to detect if high or low as input?
  //this function will be polled
}
