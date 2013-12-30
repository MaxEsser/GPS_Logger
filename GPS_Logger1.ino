#include <SD.h>
#include <AltSoftSerial.h>

const int buttonPin = 17;     // Declare PIN 17 for charging control
int buttonState = 0;          // State button int

// Set the pins used 
#define led2Pin 15 //Get data from GPS
#define pin16 16  //PILOT CHARGING
ADCSRA = 0;   // shut off ADC

AltSoftSerial gpsSerial;

const int chipSelect = 0;

#define BUFFSIZE 90
char buffer[BUFFSIZE];
uint8_t bufferidx = 0;
bool fix = false; // current fix data
bool gotGPRMC;    //true if current data is a GPRMC strinng
//bool gotGPVTG;    //true if current data is a GPRMC strinng
//bool gotGPGGA;    //true if current data is a GPRMC strinng
uint8_t i;
File logfile;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
/*
  if (SD.errorCode()) {
    putstring("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  */
  while(1) {
    for (i=0; i<errno; i++) {
      digitalWrite(led2Pin, HIGH);
      delay(100);
      digitalWrite(led2Pin, LOW);
      delay(100);
    }
    for (; i<10; i++) {
      delay(200);
    }
  }
}

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(4800);
  gpsSerial.print("$PSRF103,04,00,255,01*25\r\n"); // 
  //gpsSerial.print("$PSRF103,00,01,60,00*25"); // 
    
  Serial.println("\r\nStarting...");
  pinMode(led2Pin, OUTPUT);
  pinMode(pin16, OUTPUT);
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(0, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card init. failed!");
    error(1);
  }

  strcpy(buffer, "GPSLOG00.TXT");
  for (i = 0; i < 100; i++) {
    buffer[6] = '0' + i/10;
    buffer[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(buffer)) {
      break;
    }
  }

  logfile = SD.open(buffer, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create"); 
    Serial.println(buffer);
    error(3);
  }
  Serial.print("Starting to log on ..."); 
  Serial.println(buffer);
  pinMode(buttonPin, INPUT_PULLUP); // Set input buttonpin & pull-up on analog pin 17
}

void loop() {
  //Serial.println(Serial.available(), DEC);
  char c;
  uint8_t sum;

  // read one 'line'
  if (gpsSerial.available()) {
    c = gpsSerial.read();

    if (bufferidx == 0) {
      while (c != '$')
        c = gpsSerial.read(); // wait till we get a $
    }
    buffer[bufferidx] = c;

    if (c == '\n') {

      buffer[bufferidx+1] = 0; // terminate it

      if (buffer[bufferidx-4] != '*') {
        // no checksum?
        Serial.print('*');
        bufferidx = 0;
        return;
      }
      // get checksum
      sum = parseHex(buffer[bufferidx-3]) * 16;
      sum += parseHex(buffer[bufferidx-2]);

      // check checksum
      for (i=1; i < (bufferidx-4); i++) {
        sum ^= buffer[i];
      }
      if (sum != 0) {
        //putstring_nl("Cxsum mismatch");
        Serial.print('~');
        bufferidx = 0;
        return;
      }
      // got good data!

      gotGPRMC = strstr(buffer, "GPRMC");
      if (gotGPRMC)
      {
        // find out if we got a fix
        char *p = buffer;
        p = strchr(p, ',')+1;
        p = strchr(p, ',')+1;       // skip to 3rd item
        
        if (p[0] == 'A') {
          fix = false;
        } else {
          fix = true;
        }
      }      
      
      // rad. lets log it!
      
      Serial.print(buffer);    //first, write it to the serial monitor
      Serial.print('#');
      
      if (gotGPRMC)      //If we have a GPRMC string
      {
        // Bill Greiman - need to write bufferidx + 1 bytes to getCR/LF
        bufferidx++;
        delay(10000); //Write the sentence every 10 sec
        digitalWrite(led2Pin, HIGH);      // Turn on LED 2 (indicates write to SD)

        logfile.write((uint8_t *) buffer, bufferidx);    //write the string to the SD file
        logfile.flush();
        /*
        if( != bufferidx) {
           putstring_nl("can't write!");
           error(4);
        }
        */

        digitalWrite(led2Pin, LOW);    //turn off LED2 (write to SD is finished)
        bufferidx = 0;    //reset buffer pointer
        return;
      }//if (gotGPRMC)
      
    }
    bufferidx++;
    if (bufferidx == BUFFSIZE-1) {
       Serial.print('!');
       bufferidx = 0;
    }
  } else {

  }
  
    buttonState = digitalRead(buttonPin); // Read button state
    if (buttonState == LOW) {
    digitalWrite(pin16, HIGH); // Enabling charging mode to LTC4054
    }

}
/* End code */
