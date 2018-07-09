#include <Encoder.h>

#define SW_UP 32
#define SW_DWN 30
#define LED1 31
#define ENC1PB 12
#define TRIMPOT 22

#define RESET 0

//Declarations for Encoder Functionality*******************************************************************************************************************//
const int num_enc = 8;

long enc_pos_new[num_enc]; //used to be -999
long enc_pos_old[num_enc]; //used to be -999
int enc_updateCCflag[num_enc];

Encoder enc1(0, 1);
Encoder enc2(2, 3);
Encoder enc3(4, 5);
Encoder enc4(6, 7);
Encoder enc5(8, 9);
Encoder enc6(10, 11);
Encoder enc7(15, 14);
Encoder enc8(17, 16);

//Declarations for MIDI Functionality*******************************************************************************************************************//
const int num_keys = 29;

byte midiChannel = 0;
byte enc_CC[num_enc];
byte enc_CCVal[num_enc];
byte key_CC[num_keys];
byte key_CCVal[num_keys];

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

//Main Functions*******************************************************************************************************************//
void setup() {
  Serial.begin(9600);
  SetupPin();
  SetupCC();
  SetupKeys();
  reset_enc();
}

void loop() {
  delay(50); //to see the serial
  layerSelect();
  volumeSelect();
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

//Setup Functions*******************************************************************************************************************//
void SetupPin() {
  pinMode(SW_UP, INPUT_PULLUP);
  pinMode(SW_DWN, INPUT_PULLUP); 
  pinMode(ENC1PB, INPUT_PULLUP);
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
    if (layer != 10) {
      layer++;
    } else {
      layer = 1;
    }
    Serial.println(layer);
  } 
  prevUP = curUP;
  
  if (curDWN == LOW && prevDWN == HIGH) {
    if (layer != 1) {
      layer--;
    } else {
      layer = 10;
    }
    Serial.println(layer);
  }
  prevDWN = curDWN;

  midiChannel = layer;
}
//Volume (TRIMPOT) Function*******************************************************************************************************************//
void volumeSelect() {
  int trimVal = analogRead(TRIMPOT);
  float smoothing = 0.3;
  
  smoothVal = (smoothing*trimVal) + ((1-smoothing)*smoothVal);
  
  curTrim = map(smoothVal, 0, 1023, 0, 100);
  
  if (curTrim != prevTrim) {
    Serial.println(curTrim);
    if (curTrim <= 5 ) {
    // Serial.println("Mute");
      Keyboard.press(KEY_MEDIA_MUTE);
      Keyboard.release(KEY_MEDIA_MUTE);
    } 
    else {
      if (curTrim > prevTrim) {
        // Serial.println("Increasing");
        // if (curTrim % 20 == 0){
        //   Serial.println("Press Vol UP");
        // }
        Keyboard.press(KEY_MEDIA_VOLUME_INC);
        Keyboard.release(KEY_MEDIA_VOLUME_INC);
      } 
      else if (curTrim < prevTrim) {
        // Serial.println("Decreasing");
        // if (curTrim % 20 == 0){
        //   Serial.println("Press Vol DWN");
        // }
        Keyboard.press(KEY_MEDIA_VOLUME_DEC);
        Keyboard.release(KEY_MEDIA_VOLUME_DEC);
      }
    }
    prevTrim = curTrim;
  }
}

//Encoder Functions*******************************************************************************************************************//
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
    state_encPBMachine();

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
void state_encPBMachine() {
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
        Serial.print("encPB");
        Serial.println(" IDLE");
      break;

      case PRESSED:
        Serial.print("encPB");
        Serial.println(" PRESSED");
        keyPress_func(99); //hardcode value for the encPB
      break;

      case HOLD:
        Serial.print("encPB");
        Serial.println(" HOLD");
        keyHold_func(99); //hardcode value for the encPB
      break;

      case RELEASED:
        Serial.print("encPB");
        Serial.println(" RELEASED");
      break;
    }
  }
  //for single key presses in matrix
  for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
    for (int colIndex=0; colIndex < colCount; colIndex++) {
      if (stateChanged[rowIndex][colIndex] == true){
        switch (kstate[rowIndex][colIndex]){
          case IDLE:
            Serial.print(keymap[rowIndex][colIndex]);
            Serial.println(" IDLE");
          break;

          case PRESSED:
            Serial.print(keymap[rowIndex][colIndex]);
            Serial.println(" PRESSED");
            keyPress_func(keymap[rowIndex][colIndex]);
          break;

          case HOLD:
            Serial.print(keymap[rowIndex][colIndex]);
            Serial.println(" HOLD");
            keyHold_func(keymap[rowIndex][colIndex]);
          break;

          case RELEASED:
            Serial.print(keymap[rowIndex][colIndex]);
            Serial.println(" RELEASED");
          break;
        }
      }
    }
  }
  //for multi of 4 corner keys
  if ((stateChanged[0][0] || stateChanged[0][6] || stateChanged[2][0] || stateChanged[2][6]) == true) {
    //will only occur after HOLD state entered, due to time delay
    if (((kstate[0][0] && kstate[0][6] && kstate[2][0] && kstate[2][6]) == PRESSED) 
    && (multiPress == false)) {
      Serial.println("multi PRESS");
      //is there any way to reset the serial? mimic a power cycle
      multiPress = true;
    }
    if (((kstate[0][0] == RELEASED) || (kstate[0][6] == RELEASED) ||
    (kstate[2][0] == RELEASED) || (kstate[2][6] == RELEASED)) && (multiPress == true)) {
      Serial.println("multi RELEASE");
      multiPress = false;
    }
  }
}

void keyPress_func(int key) {
  if (layer == 1) { //for macropad
    switch (key) {
      case 0:
        Keyboard.press(KEY_MEDIA_VOLUME_DEC);
        Keyboard.release(KEY_MEDIA_VOLUME_DEC);
      break;
      case 1:
        Keyboard.press(KEY_MEDIA_PREV_TRACK);
        Keyboard.release(KEY_MEDIA_PREV_TRACK);        
      break;
      case 2:
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
        //code
      break;
      case 5:
        //code
      break;
      case 6:
        //code
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
  read_enc();
  send_enc_CC();
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
