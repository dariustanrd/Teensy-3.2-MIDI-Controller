#include <Encoder.h>
#include <WS2812Serial.h>

#define SW_UP 32
#define SW_DWN 30
#define LED1 31
#define ENC1PB 12
#define TRIMPOT 22

#define RESET 0
#define NUM_ENC 8
#define NUM_LEDS 8
#define MAX_LAYER 5

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF

//Declarations for Encoder Functionality*******************************************************************************************************************//
long enc_pos_new[NUM_ENC]; //used to be -999
long enc_pos_old[NUM_ENC]; //used to be -999
int enc_updateCCflag[NUM_ENC];

Encoder enc1(0, 1);
Encoder enc2(2, 3);
Encoder enc3(4, 5);
Encoder enc4(6, 7);
Encoder enc5(8, 9);
Encoder enc6(10, 11);
Encoder enc7(15, 14);
Encoder enc8(17, 16);

//Declarations for MIDI Functionality*******************************************************************************************************************//
byte midiChannel = 0;
byte enc_CC[NUM_ENC];
byte enc_CCVal[NUM_ENC];

volatile int sensitivity_val = 0;
bool SENS_KEY = true;

//Declarations for Keypad Functionality*******************************************************************************************************************//
byte rows[] = {18,19,20,21};
const int rowCount = sizeof(rows)/sizeof(rows[0]);

byte cols[] = {33,29,28,27,26,25,24};
const int colCount = sizeof(cols)/sizeof(cols[0]);

byte cur_encPB;
byte prev_encPB;
bool button_encPB;
bool stateChanged_encPB;
unsigned long encPB_holdTimer = 0;

byte curKeys[rowCount][colCount] = {};
byte prevKeys[rowCount][colCount] = {};
bool button[rowCount][colCount] = {};
bool stateChanged[rowCount][colCount] = {};
bool multiPress = false;

typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;

unsigned long holdTimer = 0;
unsigned long holdTime = 1000; //hard code 1s

KeyState kstate[rowCount][colCount] = {};
KeyState state_encPB;

const int keymap[rowCount][colCount] = {
  {00,01,02,03,04,05,06},
  {10,11,12,13,14,15,16},
  {20,21,22,23,24,25,26},
  {30,31,32,33,34,35,36}
};

//Other Declarations*******************************************************************************************************************//
volatile int layer = 1;
bool curDWN = HIGH;
bool prevDWN = HIGH;
bool curUP = HIGH;
bool prevUP = HIGH;
int curTrim = 0;
int prevTrim = 0;
int smoothVal = 0;

byte drawingMemory[NUM_LEDS*3];         //  3 bytes per LED
DMAMEM byte displayMemory[NUM_LEDS*12]; // 12 bytes per LED

WS2812Serial leds(NUM_LEDS, displayMemory, drawingMemory, LED1, WS2812_GRB);

//Main Functions*******************************************************************************************************************//
void setup() {
  // Serial.begin(9600);
  SetupPin();
  SetupCC();
  SetupKeys();
  reset_enc();

  leds.begin();
}

void loop() {
  layerSelect();
  volumeSelect();
  switch(layer) {
      case 1:
        macropad();
        //Serial.print("Macropad Mode\n");
        setColour(BLUE);
      break;
      
      case 2:
        lightroom();
        setColour(PINK);
      break;

      case 3:
        //Premiere Pro Edit Layer?
        setColour(GREEN);
      break;

      case 4:
        //Coding Layer? 
        setColour(YELLOW);
      break;

      case 5:
        //end layer
        rainbowCycle(5);
      break;
      default:
        macropad();
    }
}

//Setup Functions*******************************************************************************************************************//
void SetupPin() {
  pinMode(SW_UP, INPUT_PULLUP);
  pinMode(SW_DWN, INPUT_PULLUP); 
  pinMode(ENC1PB, INPUT_PULLUP);
}
void SetupCC(){
  for (int i = 0; i < NUM_ENC; i++) {
    enc_CC[i] = i;
    enc_CCVal[i] = 64; //set CCVal to be middle upon init.
  }
}
void SetupKeys(){
  for(int x=0; x<colCount; x++) {
        // Serial.print(cols[x]); 
        // Serial.println(" as input");
        pinMode(cols[x], INPUT);
    }
 
    for (int x=0; x<rowCount; x++) {
        // Serial.print(rows[x]); 
        // Serial.println(" as input-pullup");
        pinMode(rows[x], INPUT_PULLUP);
    }

    for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
        for (int colIndex=0; colIndex < colCount; colIndex++) {
            kstate[rowIndex][colIndex] = IDLE;
        }
    }
    state_encPB = IDLE;
}
//Layer Function*******************************************************************************************************************//
void layerSelect() {
  //check if SW_up press, layer++. if SW_dwn press, layer--
  curUP = digitalRead(SW_UP);
  curDWN = digitalRead(SW_DWN);

  if (curUP == LOW && prevUP == HIGH) {
    if (layer != MAX_LAYER) {
      layer++;
    } else {
      layer = 1;
    }
    // Serial.println(layer);
  } 
  prevUP = curUP;
  
  if (curDWN == LOW && prevDWN == HIGH) {
    if (layer != 1) {
      layer--;
    } else {
      layer = MAX_LAYER;
    }
    // Serial.println(layer);
  }
  prevDWN = curDWN;

  midiChannel = layer;
}
//Volume (TRIMPOT) Function*******************************************************************************************************************//
void volumeSelect() {
  int trimVal = analogRead(TRIMPOT);
  float smoothing = 0.2; //lower more stable
  
  smoothVal = (smoothing*trimVal) + ((1-smoothing)*smoothVal);
  
  curTrim = map(smoothVal, 0, 1023, 0, 100);
  
  if (curTrim != prevTrim) {
    // Serial.println(curTrim);
    if (curTrim <= 5 ) {
    // Serial.println("Mute");
      Keyboard.press(KEY_MEDIA_MUTE);
      Keyboard.release(KEY_MEDIA_MUTE);
    } 
    else {
      if (curTrim > prevTrim) {
        // Serial.println("Increasing");
        Keyboard.press(KEY_MEDIA_VOLUME_INC);
        Keyboard.release(KEY_MEDIA_VOLUME_INC);
      } 
      else if (curTrim < prevTrim) {
        // Serial.println("Decreasing");
        Keyboard.press(KEY_MEDIA_VOLUME_DEC);
        Keyboard.release(KEY_MEDIA_VOLUME_DEC);
      }
    }
    prevTrim = curTrim;
  }
}

//LED Functions*******************************************************************************************************************//
void setColour (int colour) {
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, colour);
    leds.show();
  }
}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) {
      c=Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      leds.setPixel(i, *c, *(c+1), *(c+2));
    }
    leds.show();
    delay(SpeedDelay);
  }
}

byte * Wheel(byte WheelPos) {
  static byte c[3];
  
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }
  return c;
}
//Encoder Functions*******************************************************************************************************************//
void reset_enc(){
  enc1.write(RESET);
  enc2.write(RESET);
  enc3.write(RESET);
  enc4.write(RESET);
  enc5.write(RESET);
  enc6.write(RESET);
  enc7.write(RESET);
  enc8.write(RESET);
  // Serial.print("ENC Reset\n");
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
  for (int i = 0; i < NUM_ENC; i++) {
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
  for (int i = 0; i < NUM_ENC; i++) {
    if (enc_updateCCflag[i]!=0) {
      usbMIDI.sendControlChange(enc_CC[i], enc_CCVal[i], midiChannel);
      // Serial.print("CC Change = ");
      // Serial.print(enc_CCVal[i]);
      // Serial.println();
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
//Keypad Functions*******************************************************************************************************************//
void read_keys() {
    //read single encPB ENC1PB not on matrix
    cur_encPB = digitalRead(ENC1PB);
    if (cur_encPB == 0 && prev_encPB == 1) {
      button_encPB = 0;
      prev_encPB = cur_encPB;
    }
    if (cur_encPB == 1 && prev_encPB == 0) {
      button_encPB = 1;
      prev_encPB = cur_encPB;
    }
    encPB_stateMachine();

    // iterate the rows
    for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
        // row: set to output to low
        byte curRow = rows[rowIndex];
        pinMode(curRow, OUTPUT);
        digitalWrite(curRow, LOW);
        
        // col: interate through the cols
        for (int colIndex=0; colIndex < colCount; colIndex++) {
            byte rowCol = cols[colIndex];
            pinMode(rowCol, INPUT_PULLUP);
            delayMicroseconds(10); //to prevent overlap of readings
            curKeys[rowIndex][colIndex] = digitalRead(rowCol);
            
            
            //curKeys tracks the current status of the key. 1 == released, 0 == pressed
            //following 2 if statements ensure only single button press and release
            if (curKeys[rowIndex][colIndex] == 0 && prevKeys[rowIndex][colIndex] == 1) {
                button[rowIndex][colIndex] = 0;
                prevKeys[rowIndex][colIndex] = curKeys[rowIndex][colIndex];
            }
            if (curKeys[rowIndex][colIndex] == 1 && prevKeys[rowIndex][colIndex] == 0) {
                button[rowIndex][colIndex] = 1;
                prevKeys[rowIndex][colIndex] = curKeys[rowIndex][colIndex];
            }

            //function for key state changes -- state machine
            kstateMachine(rowIndex,colIndex);
            pinMode(rowCol, INPUT);
        }
        // disable the column
        pinMode(curRow, INPUT);
    }
}
void encPB_stateMachine() {
  stateChanged_encPB = false;
  switch (state_encPB) {
    case IDLE:
      if (button_encPB == 0) {
        state_encPB = PRESSED;
        stateChanged_encPB = true;
        encPB_holdTimer = millis();
      }
    break;
    case PRESSED:
      if ((millis()-encPB_holdTimer)>holdTime) {
        state_encPB = HOLD;
        stateChanged_encPB = true;
      } else if (button_encPB == 1) {
        state_encPB = RELEASED;
        stateChanged_encPB = true;
      }
    break;
    case HOLD:
      if (button_encPB == 1) {
        state_encPB = RELEASED;
        stateChanged_encPB = true;
      }
    break;
    case RELEASED:
      state_encPB = IDLE;
      stateChanged_encPB = true;
    break;
  }
}

void kstateMachine(int row, int col){
  stateChanged[row][col] = false;
  switch (kstate[row][col]) {
    case IDLE:
      if (button[row][col] == 0){
        kstate[row][col] = PRESSED;
        stateChanged[row][col] = true;
        holdTimer = millis();
      }
      break;
    case PRESSED:
      if ((millis()-holdTimer)>holdTime) {
        kstate[row][col] = HOLD;
        stateChanged[row][col] = true;
      } else if (button[row][col] == 1) {
        kstate[row][col] = RELEASED;
        stateChanged[row][col] = true;
      }
      break;
    case HOLD:
      if (button[row][col] == 1) {
        kstate[row][col] = RELEASED;
        stateChanged[row][col] = true;
      }
      break;
    case RELEASED:
      kstate[row][col] = IDLE;
      stateChanged[row][col] = true;
      break;
  }
}

void send_keys() {
  //for single enc key
  if(stateChanged_encPB == true) {
    switch (state_encPB) {
      case IDLE:
        // Serial.print("encPB");
        // Serial.println(" IDLE");
      break;

      case PRESSED:
        // Serial.print("encPB");
        // Serial.println(" PRESSED");
        keyPress_func(99); //hardcode value for the encPB
      break;

      case HOLD:
        // Serial.print("encPB");
        // Serial.println(" HOLD");
        keyHold_func(99); //hardcode value for the encPB
      break;

      case RELEASED:
        // Serial.print("encPB");
        // Serial.println(" RELEASED");
      break;
    }
  }
  //for single key presses in matrix
  for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
    for (int colIndex=0; colIndex < colCount; colIndex++) {
      if (stateChanged[rowIndex][colIndex] == true){
        switch (kstate[rowIndex][colIndex]){
          case IDLE:
            // Serial.print(keymap[rowIndex][colIndex]);
            // Serial.println(" IDLE");
          break;

          case PRESSED:
            // Serial.print(keymap[rowIndex][colIndex]);
            // Serial.println(" PRESSED");
            keyPress_func(keymap[rowIndex][colIndex]);
          break;

          case HOLD:
            // Serial.print(keymap[rowIndex][colIndex]);
            // Serial.println(" HOLD");
            keyHold_func(keymap[rowIndex][colIndex]);
          break;

          case RELEASED:
            // Serial.print(keymap[rowIndex][colIndex]);
            // Serial.println(" RELEASED");
          break;
        }
      }
    }
  }
  //for multi of 4 corner keys
  if ((stateChanged[0][0] || stateChanged[0][6] || stateChanged[2][0] || stateChanged[2][6]) == true) {
    if (((kstate[0][0] && kstate[0][6] && kstate[2][0] && kstate[2][6]) == PRESSED) 
    && (multiPress == false)) {
      // Serial.println("multi PRESS");
      //is there any way to reset the serial? mimic a power cycle?
      Keyboard.press(KEY_SYSTEM_SLEEP);
      Keyboard.release(KEY_SYSTEM_SLEEP);
      multiPress = true;
    }
    if (((kstate[0][0] == RELEASED) || (kstate[0][6] == RELEASED) ||
    (kstate[2][0] == RELEASED) || (kstate[2][6] == RELEASED)) && (multiPress == true)) {
      // Serial.println("multi RELEASE");
      multiPress = false;
    }
  }
}

void keyPress_func(int key) {
  if (layer == 1) { //for macropad see https://www.pjrc.com/teensy/td_keyboard.html
    switch (key) {
      case 0:
        Keyboard.press(KEY_MEDIA_VOLUME_DEC);
        Keyboard.release(KEY_MEDIA_VOLUME_DEC);
      break;
      case 1:
        Keyboard.press(KEY_MEDIA_PREV_TRACK);
        Keyboard.release(KEY_MEDIA_PREV_TRACK);        
      break;
      case 2: //open command palette
        Keyboard.set_modifier(MODIFIERKEY_CTRL | MODIFIERKEY_SHIFT);
        Keyboard.send_now();
        Keyboard.press(KEY_P);
        Keyboard.release(KEY_P);
        Keyboard.set_modifier(0);
        Keyboard.send_now();
      break;
      case 3:
        //code
      break;
      case 4:
        //winkey ctrl left see virtual desktop right
        Keyboard.set_modifier(MODIFIERKEY_GUI | MODIFIERKEY_CTRL);
        Keyboard.send_now();
        Keyboard.press(KEY_LEFT);
        Keyboard.release(KEY_LEFT);
        Keyboard.set_modifier(0);
        Keyboard.send_now();
      break;
      case 5:
        //winkey tab see task view
        Keyboard.set_modifier(MODIFIERKEY_GUI);
        Keyboard.send_now();
        Keyboard.press(KEY_TAB);
        Keyboard.release(KEY_TAB);
        Keyboard.set_modifier(0);
        Keyboard.send_now();
      break;
      case 6:
        //winkey ctrl right see virtual desktop right
        Keyboard.set_modifier(MODIFIERKEY_GUI | MODIFIERKEY_CTRL);
        Keyboard.send_now();
        Keyboard.press(KEY_RIGHT);
        Keyboard.release(KEY_RIGHT);
        Keyboard.set_modifier(0);
        Keyboard.send_now();
      break;
      case 10:
        Keyboard.press(KEY_MEDIA_MUTE);
        Keyboard.release(KEY_MEDIA_MUTE);
      break;
      case 11:
        Keyboard.press(KEY_MEDIA_PLAY_PAUSE);
        Keyboard.release(KEY_MEDIA_PLAY_PAUSE);
      break;
      case 12:
        //code
      break;
      case 13:
        //code
      break;
      case 14:
        //code
      break;
      case 15:
        //code
      break;
      case 16:
        //code
      break;
      case 20:
        Keyboard.press(KEY_MEDIA_VOLUME_INC);
        Keyboard.release(KEY_MEDIA_VOLUME_INC);
      break;
      case 21:
        Keyboard.press(KEY_MEDIA_NEXT_TRACK);
        Keyboard.release(KEY_MEDIA_NEXT_TRACK);
      break;
      case 22:
        //code
      break;
      case 23:
        //code
      break;
      case 24:
        //code
      break;
      case 25:
        //code
      break;
      case 26:
        //code
      break;
      case 30:
        //code
      break;
      case 31:
        //code
      break;
      case 32:
        //code
      break;
      case 33:
        //code
      break;
      case 34:
        //code
      break;
      case 35:
        //code
      break;
      case 36:
        //code
      break;
      case 99: //encPB case
        //code
      break;
    }
  } else if (layer == 2) { //for lightroom layer
    usbMIDI.sendNoteOn(key, 99, midiChannel);
    usbMIDI.sendNoteOff(key, 0, midiChannel);
    switch (key) { //to reset enc values for MIDI2LR
      case 3: //to coincide with MIDI2LR left and right keys resetting encoder values
        for (int i = 0; i < NUM_ENC; i++) {
          enc_CCVal[i] = 64;
        }
      break;
      case 5: //to coincide with MIDI2LR left and right keys resetting encoder values
        for (int i = 0; i < NUM_ENC; i++) {
          enc_CCVal[i] = 64;
        }
      break;
      case 6: //to coincide with MIDI2LR profile 3 key resetting encoder values
        for (int i = 0; i < NUM_ENC; i++) {
          enc_CCVal[i] = 64;
        }
      break;
      case 16: //to coincide with MIDI2LR left and right keys resetting encoder values
        for (int i = 0; i < NUM_ENC; i++) {
          enc_CCVal[i] = 64;
        }
      break;
      case 26: //to coincide with MIDI2LR left and right keys resetting encoder values
        for (int i = 0; i < NUM_ENC; i++) {
          enc_CCVal[i] = 64;
        }
      break;
      case 30:
        enc_CCVal[6] = 64;
      break;
      case 31:
        enc_CCVal[7] = 64;
      break;
      case 32:
        enc_CCVal[5] = 64;
      break;
      case 33:
        enc_CCVal[4] = 64;
      break;
      case 34:
        enc_CCVal[3] = 64;
      break;
      case 35:
        enc_CCVal[2] = 64;
      break;
      case 36:
        enc_CCVal[1] = 64;
      break;
      case 99: //encPB case
        enc_CCVal[0] = 64;
      break;
    }
  }
}

void keyHold_func(int key) {
  if (layer == 1) { //for macropad
    switch (key) {
      case 0:
        //code
      break;
      case 1:
        //code
      break;
      case 2:
        //code
      break;
      case 3:
        //code
      break;
      case 4:
        //code
      break;
      case 5:
        //code
      break;
      case 6:
        //code
      break;
      case 10:
        //code
      break;
      case 11:
        //code
      break;
      case 12:
        //code
      break;
      case 13:
        //code
      break;
      case 14:
        //code
      break;
      case 15:
        //code
      break;
      case 16:
        //code
      break;
      case 20:
        //code
      break;
      case 21:
        //code
      break;
      case 22:
        //code
      break;
      case 23:
        //code
      break;
      case 24:
        //code
      break;
      case 25:
        //code
      break;
      case 26:
        //code
      break;
      case 30:
        //code
      break;
      case 31:
        //code
      break;
      case 32:
        //code
      break;
      case 33:
        //code
      break;
      case 34:
        //code
      break;
      case 35:
        //code
      break;
      case 36:
        //code
      break;
      case 99: //encPB case
        //code
      break;
    }
  }  else if (layer == 2) { //for lightroom layer
    int holdKey = key + 28; //to push to new set of keys, non overlapping single press key value
    usbMIDI.sendNoteOn(holdKey, 99, midiChannel);
    usbMIDI.sendNoteOff(holdKey, 0, midiChannel);
  }
}
//Layer Functions*******************************************************************************************************************//
void macropad() {
//  read_enc();
//  send_enc_CC();
  read_keys();
  send_keys();
  
  //change LED
}

void lightroom() {
  read_enc();
  send_enc_CC();
  read_keys();
  send_keys();
}
