/*-------------------------------------------------------------
 * Triffic Light
 * 
 *  Author: Matthew Hengeveld
 *  Date:   Oct 8, 2015
 *  
 *  Github:       
 *  Instructable: https://www.instructables.com/member/Hengy/
 *  
 *  Verison:  1.2   Oct 12, 2017
 *  
 *  MIT License
 *  
 *  Copyright (c) [year] [fullname]
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */


/*-------------------------------------------------------------
 * Software Options
 *  - DEBUG (serial output)
 *  - Battery stay alive function
 */

#define DEBUG // debug mode; outputs debug messages to Serial port

#define BATSA // Battery stay alive; enables hardware to keep 'smart'
              // USB batterys from turning off due to low current.
              // NOTE: Settings may have to be adjusted for specific 
              // batteries.

/*-------------------------------------------------------------
 * Macros
 */

#ifdef DEBUG
  #define DEBUG_PRINT(msg)  Serial.println(msg)
#endif

/*-------------------------------------------------------------
 * Pins
 */

// LED pins
const uint8_t redLight = 5;
const uint8_t yellowLight = 6;
const uint8_t greenLight = 9;

// switch pins
const uint8_t redBttn = A4;
const uint8_t yellowBttn = A3;
const uint8_t greenBttn = A2;
const uint8_t modeBttn = A5;
const uint8_t brightKnob = A0;

// USB battery stay alive pins
const uint8_t saDriver = 11;

/*-------------------------------------------------------------
 * Variables
 */

// brightness of LEDs
uint8_t brightness = 255;

// relative brightness of LEDs
float redComp = 0.93;
float yellowComp = 1.0;
float greenComp = 0.85;

// poteniometer value
int potVal;

// mode; 0 - traffic light; 1 - manual; 2 - disco
uint8_t mode = 0; // default mode is 0
uint8_t lightOn = 1;

// state - used for modes 0, 2
uint8_t state = 0;

// button checking variables
uint8_t modeBttnState = 0;

// timing variables
unsigned long timeStart = 0;
unsigned long timeCurr = 0;
unsigned long timeElapsed = 0;

// manual mode switch variables
uint8_t redBttnState = 0;
uint8_t yellowBttnState = 0;
uint8_t greenBttnState = 0;

// disco mode timing variables
short beats[4] = {600, 420, 210, 125};
unsigned short discoDelayIndex = 2; // change to new colour every 500ms
unsigned long discoCurr = 0;
unsigned long discoElapsed = 0;
unsigned long discoPrev = 0;

// disco mode variables
short discoLightMode = 0;  // 0 - constant; 1 - pulse
uint8_t currCol = 0; // current colour; 0 - green; 1 - yellow; 2 - red
uint8_t newCol = 1;

/*
 * USB battery stay alive
 *  This feature draws a certain amount of current for a few millisends
 *  (defined by saTH, below), and waits a number of milliseconds (defined
 *  by saTL, below), then repeats. This draws an average current sufficent
 *  enough to keep a 'smart' USB battery on when the traffic light LEDs &
 *  uC don't use a lot. The amount of current drawn depends on the hardware,
 *  specifically a resistor. The longer the current is drawn, the higher
 *  the average current drawn. Not all USB batteries are alike. Some may
 *  require more or less current drawn at more or less frequent intervals
 *  in order to 'keep alive'. You must experiemnt to find the right
 *  values for your battery.
 */

// USB battery stay alive variables
uint8_t saState = 0;  // on or off
unsigned long saCurr = 0;
unsigned long saNext = 0;
// USB battery stay alive OPTIONS
#define   saTH  2     // time (ms) to draw current
#define   saTL  497   // time (ms) to wait (no current draw)


/*-------------------------------------------------------------
 * Arduino standard setup function
 */
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  #ifdef BATSA
    // initialize digital pins for optional USB battery stay alive
    pinMode(saDriver, OUTPUT);
    digitalWrite(saDriver, LOW);
  
    // initialize values for USB stay alive
    saCurr = millis();
    saNext = saCurr + saTL;
  #endif
  
  // initialize digital pins for LEDs
  pinMode(redLight, OUTPUT);
  pinMode(yellowLight, OUTPUT);
  pinMode(greenLight, OUTPUT);

  // initialize digital pins for switches
  pinMode(redBttn, INPUT);
  pinMode(yellowBttn, INPUT);
  pinMode(greenBttn, INPUT);
  pinMode(modeBttn, INPUT);

  // 'randomize' seed for random numbers
  randomSeed(analogRead(A0));
}



/*-------------------------------------------------------------
 * Arduino standard loop function
 */
void loop() {
  #ifdef BATSA
    stayAlive();
  #endif
  
  getBrightness();

  checkModeBttn();

  // if lights are on
  if (lightOn) {
    // select mode
    if (mode == 1) {
      mode1();  // disco mode
    } else if (mode == 2) {
      mode2();  // manual mode
    } else {
      mode0();  // normal mode; default
    }
  }
}



/*-------------------------------------------------------------
 * getBrightness function
 *  calculates brightness from pot
 */
void getBrightness() {
  potVal = analogRead(brightKnob);  // get analog reading
  brightness = (uint8_t)( ( (potVal/32)*(potVal/32) )/4 );  // convert pot value to brightness
                                                            // using an exponential scale
  if (brightness < 4) {
    brightness = 0;
  }
  DEBUG_PRINT(brightness);
}

/*-------------------------------------------------------------
 * checkModeBttn function
 *  checks for input from mode button
 *  sets defaults for each mode
 */
void checkModeBttn() {
  // read mode button
  modeBttnState = digitalRead(modeBttn);
  
  if (modeBttnState) {  // if mode button was pressed
    ++mode; // increment mode variable

    // turn off all LEDs at mode change
    analogWrite(redLight, 0);
    analogWrite(greenLight, 0);
    analogWrite(yellowLight, 0);

    // if mode variable is greater then 2, reset to 0
    if (mode > 2) {
      mode = 0;
    }

    // if mode is 2, initialize disco mode delay and reset to color 0
    if (mode == 2) {
      discoDelayIndex = 2;
      currCol = 0;
    }

    // if mode is 0 or 1, reset timing variables & reset to color 0
    if (mode < 2) {
      timeCurr = 0;
      timeStart = 0;
      currCol = 0;

      // if mode is 1, set color to red
      if (mode == 1) {
        analogWrite(redLight, brightness*redComp);
      }
    }

    // delay for a rough software debounce
    delay(200);
  }
}

/*-------------------------------------------------------------
 * mode0 function
 *  normal traffic light mode
 */
void mode0() {
  timeCurr = millis();  // get curent millisecond
  timeElapsed = timeCurr - timeStart; // get elapsed time from previous color change
  
  if (state == 1) { // yellow
    analogWrite(yellowLight, brightness*yellowComp);  // turn on yellow LED
    analogWrite(greenLight, 0); // turn off green LED

    if (timeElapsed > 4000) { // go to next color after so many milliseconds
      state = 2;  // go to red next
      timeStart = millis(); // 'save' time of color change
    }
    
  } else if (state == 2) { // red
    analogWrite(redLight, brightness*redComp);  // turn on red LED
    analogWrite(yellowLight, 0);  // turn off yellow LED

    
    if (timeElapsed > 8000) { // go to next color after so many milliseconds
      state = 0;  // go to green next
      timeStart = millis(); // 'save' time of color change
    }
    
  } else { // green (state 0)
    analogWrite(greenLight, brightness*greenComp);  // turn on green LED
    analogWrite(redLight, 0); // turn off red LED

    
    if (timeElapsed > 8000) { // go to next color after so many milliseconds
      state = 1;  // go to yellow next
      timeStart = millis(); // 'save' time of color change
    }
  }
}

/*-------------------------------------------------------------
 * mode2 function
 *  manual traffic light mode
 */
void mode1() {

  redBttnState = digitalRead(redBttn);  // read red button
  if (redBttnState) {
    // turn on red
    analogWrite(greenLight, 0);
    analogWrite(yellowLight, 0);
    analogWrite(redLight, brightness*redComp);
    
  } else {
    yellowBttnState = digitalRead(yellowBttn);    // read yellow button
    if (yellowBttnState) {
      // turn on yellow
      analogWrite(greenLight, 0);
      analogWrite(redLight, 0);
      analogWrite(yellowLight, brightness*yellowComp);
      
    } else {
      greenBttnState = digitalRead(greenBttn);    // read green button
      if (greenBttnState) {
        // turn on green
        analogWrite(yellowLight, 0);
        analogWrite(redLight, 0);
        analogWrite(greenLight, brightness*greenComp);
      }
    }
  }
}

/*-------------------------------------------------------------
 * mode3 function
 *  disco traffic light mode
 */
void mode2() {
  discoCurr = millis(); // get curent millisecond
  discoElapsed = discoCurr - discoPrev; // get elapsed time from previous color change
  
  if (discoElapsed >= beats[discoDelayIndex]) {
    do {
      newCol = (int)(random(1200)/400);
    } while (newCol == currCol);
    currCol = newCol;
    if ((currCol == 0) || (currCol == 3)) {
      analogWrite(yellowLight, 0);
      analogWrite(redLight, 0);
      analogWrite(greenLight, brightness*greenComp);
    } else if ((currCol == 1) || (currCol == 4)) {
      analogWrite(greenLight, 0);
      analogWrite(redLight, 0);
      analogWrite(yellowLight, brightness*yellowComp);
    } else {
      analogWrite(greenLight, 0);
      analogWrite(yellowLight, 0);
      analogWrite(redLight, brightness*redComp);
    }
    if (discoLightMode == 1) {
      delay(75);
      analogWrite(greenLight, 0);
      analogWrite(yellowLight, 0);
      analogWrite(redLight, 0);
    }
    discoPrev = millis();
  }

  redBttnState = digitalRead(redBttn);
  yellowBttnState = digitalRead(yellowBttn);
  greenBttnState = digitalRead(greenBttn);
  if (redBttnState) {
    if (discoLightMode == 0) {
      discoLightMode = 1;
    } else {
      discoLightMode = 0;
    }
    delay(150);
  }
  if (yellowBttnState) {
    --discoDelayIndex;
    if (discoDelayIndex < 1) {
      discoDelayIndex = 1;
    }
    delay(250);
  }
  if (greenBttnState) {
    ++discoDelayIndex;
    if (discoDelayIndex > 3) {
      discoDelayIndex = 3;
    }
    delay(250);
  }
}
void stayAlive() {
  saCurr = millis();
  if (saNext <= saCurr) {
    if (saState) {
      digitalWrite(saDriver, LOW);
      saNext = millis() + saTL;
      saState = 0;
    } else {
      digitalWrite(saDriver, HIGH);
      saNext = millis() + saTH;
      saState = 1;
    }
  }
}

