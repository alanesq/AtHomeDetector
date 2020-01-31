/*
 *  AtHomeDetector 
 *  
 *     esp8266 project to detect if a person is at home - 18Jan20
 * 
 *     based on: https://www.hackster.io/ricardooliveira/esp8266-friend-detector-12542e
 * 
 *     enter '?' on serial for list of commands
 *              
 */


  #include "./esppl_functions.h"               // Promiscuous mode wifi

  #include <EEPROM.h>                          // settings are stored in eeprom

  #include "Nokia_5110.h"                      // Nokia LCD display


// ------------------------------------------- s e t t i n g s --------------------------------------------


const String sVersion = "2.1";               // Sketch version

bool AtHome = 1;                             // if flagged as at home when first powered on (0=out)

const byte defaultTriggertime = 10;          // default time phone not seen to decide person has gone out (15 second units, stored in eeprom)
byte TriggerTime; 

const byte controlPin = D1;                  // output pin (switches relay to signal person at home status)

const byte defaultContrast = 45;             // default lcd contrast (stored in eeprom)
byte lcdContrast;

// Nokia display pin assignments 
    #define RST D2
    #define CE D3
    #define DC D4
    #define DIN D5
    #define CLK D6


// Known MAC addresses 

    // number of MAC addresses in list 
    #define LIST_SIZE 2           

    // MAC address list    (top one should be the phone to be monitored)
    uint8_t friendmac[LIST_SIZE][ESPPL_MAC_LEN] = { 
     {0x38, 0x4a, 0x3b, 0x46, 0xb4, 0x52} 
    ,{0x00, 0x15, 0xC4, 0x23, 0xDA, 0x0D} 
    }; 
 
    // name list 
    String friendname[LIST_SIZE] = { 
     "device1"
    ,"device2" 
    }; 


// --------------------------------------------------------------------------------------------------------


String lastDevice = "";                      // Last device seen

unsigned long lastSeenPerson = 0;             // time the persons phone was last seen

unsigned long lastSeenOther = 0;             // time any device was last seen

bool goneout = 0;                            // flag to show if the person has been flagged out at all since device started (handy for testing)

bool showMacs = 0;                           // output all received MAC addresses on serial port (1=enabled)

unsigned long LCDtimer = millis();           // counter for timing of updating the nokia display


Nokia_5110 lcd = Nokia_5110(RST, CE, DC, DIN, CLK);   // Nokia LCD display


// --------------------------------------------------------------------------------------------------------


void setup() { 

 // eeprom 
  EEPROM.begin(512);
  readEeprom();    // read in settings from EEPROM
  
 Serial.begin(115200); 
 ShowHelp();     // show commands available on serial port
 
 // nokia display
   lcd.setContrast(lcdContrast); // 60 is the default value set by the driver
   ShowText(1, 0, "");
   ShowText(0, 2, "AtHomeDetector");
   ShowText(0, 3, sVersion);
   ShowText(0, 5, "");
   delay(2000);
   lcd.clear();
   
// output at home status pin
    pinMode(controlPin, OUTPUT);
    digitalWrite(controlPin, AtHome);
   
 esppl_init(cb);            // wifi monitoring
 esppl_sniffing_start(); 

}


// -----------------------------------------------


void loop() { 

  handleSerial();     // check if a command on the serial port
  
  unsigned long Tnow = millis();
  unsigned long TimeSincePerson = (unsigned long)(Tnow - lastSeenPerson);      // time since person was last detected
  unsigned long TimeSinceOther = (unsigned long)(Tnow - lastSeenOther);      // time since other device was last detected
  
  // check at home status
    if (AtHome == 0) {
      // currently flagged as out
      if (TimeSincePerson < (TriggerTime * 15000)) {
        // person has come home
        AtHome = 1;
        digitalWrite(controlPin, AtHome);
        Serial.println("Person has come home");
      }
    } else {
      // currently flagged as in
      if (TimeSincePerson > (TriggerTime * 15000)) {
        // Person has gone out
        AtHome = 0;
        digitalWrite(controlPin, AtHome);
        Serial.println("Person has gone out");
        goneout = 1;    // flag that Person has been flagged as out at some point since sketch started
      }
    }

    // Update the LCD display every 250ms
    unsigned long currentMillis = millis();        // get current time
    if ( (unsigned long)(currentMillis - LCDtimer ) >= 250 ) {
        LCDtimer = millis();   // reset timer
        UpdateNokiaDisplay();
    }
      
   // step through all wifi channels looking for devices on wifi
   for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++ ) { 
     esppl_set_channel(i); 
     while (esppl_process_frames()) { 
       // wait for channel devices to be processed
     } 
   } 
 
} 



// ------- handle any command sent on serial port ---------

void handleSerial() {
 while (Serial.available() > 0) {
   char incomingCharacter = Serial.read();
   switch (incomingCharacter) {
     case '+':
        // increase contrast
        if (lcdContrast <= 68) lcdContrast += 2;
        Serial.println("Contrast set to " + String(lcdContrast));
        lcd.setContrast(lcdContrast);
        EEPROM.write(101,lcdContrast);  
        EEPROM.commit();
      break;
 
     case '-':
        // decrease contrast
        if (lcdContrast >= 32) lcdContrast -= 1;
        Serial.println("Contrast set to " + String(lcdContrast));
        lcd.setContrast(lcdContrast);  
        EEPROM.write(101,lcdContrast);   
        EEPROM.commit();    
      break;

     case 'u':
        // increase trigger time
        if (TriggerTime < 255) TriggerTime += 1;
        Serial.println("Trigger time set to " + String(TriggerTime*15) + " seconds"); 
        EEPROM.write(102,TriggerTime);   
        EEPROM.commit();    
      break;

     case 'd':
        // decrease trigger time
        if (TriggerTime > 1) TriggerTime -= 1;
        Serial.println("Trigger time set to " + String(TriggerTime*15) + " seconds"); 
        EEPROM.write(102,TriggerTime);   
        EEPROM.commit();    
      break;

     case 'r':
        // reset all to defaults
        if (TriggerTime >= 2) TriggerTime -= 1;
        Serial.println("\nResetting all to defaults\n"); 
        showMacs = 0;
        lcdContrast = defaultContrast;
        TriggerTime = defaultTriggertime;
        goneout = 0;
        EEPROM.write(101,lcdContrast); 
        EEPROM.write(102,TriggerTime); 
        EEPROM.commit();    
        ShowHelp();
      break;

     case 's':
        // Show status
        Serial.println("\n------- System Status --------");
        Serial.println("AtHomeDetector version " + sVersion);
        Serial.println("Trigger time = " + String(TriggerTime * 15) + " seconds");
        Serial.println("LCD Contrast = " + String(lcdContrast));
        Serial.print("Person is ");
          if (AtHome) Serial.println("In");
          else Serial.println("Out"); 
        Serial.print("MAC Address display is ");
          if (showMacs) Serial.println("enabled"); 
          else Serial.println("disabled");
        Serial.println("------------------------------");   
      break;

      case '0':
        // do not show received mac addresses on serial port
        Serial.println("Not showing received MAC address info.");
        showMacs = 0;
      break;

      case '1':
        // show received mac addresses on serial port
        Serial.println("Showing received MAC address info.");
        showMacs = 1;
      break;

      case '?':
        // show help info.
        ShowHelp();
      break;

//      default:
//        // invalid command
//        Serial.println("Error: invalid command");
//        ShowHelp();
//      break;
      
    }
 }
}




// ----------- Update the Nokia LCD display --------------


void UpdateNokiaDisplay(){ 
  
     unsigned long Tnow = millis();
     unsigned long TimeSincePerson = (unsigned long)(Tnow - lastSeenPerson);      // time since person was last detected
     unsigned long TimeSinceOther = (unsigned long)(Tnow - lastSeenOther);      // time since other device was last detected
   
     ShowText(1,0,"AtHomeDetector");
     
     // show if Person has been flagged out at all since device was started
       if (goneout == 1) {
         ShowText(0, 1, "    <Been Out>");
       }

     // display at home status
      if (AtHome) {
        ShowText(0, 2, ">Person is home");    // show on Nokia display
      } else {
        ShowText(0, 2, ">Person is out ");    // show on Nokia display
      }
      ShowText(0, 3, "  " + String((TimeSincePerson/1000)) + "   ");
  
     // display last device seen info.
       ShowText(0, 4, ">" + lastDevice);
       ShowText(0, 5, "  " + String((TimeSinceOther/1000)) + "   ");
}

       
/* 
   display line of text on Nokia display
     parameters:
        clear = clear display first 1 or 0
        line = which line to start text on
        TheString = text to display
*/
void ShowText(bool Clear, byte Line, String TheText) {
    if (Clear) lcd.clear();
    lcd.setCursor(0, Line);    // x , y
    lcd.println(TheText);
}


// --------- read stored settings from eeprom ------------

void readEeprom() {
  byte val;

  // lcd contrast
    val = EEPROM.read(101);
    if (val < 20 || val > 60) val = defaultContrast;
    lcdContrast = val;

  // triggertime
    val = EEPROM.read(102);
    if (val < 1 || val > 250) val = defaultTriggertime;
    TriggerTime = val;
    
}


// --------- Show help ---------------------- ------------

void ShowHelp() {
  
 Serial.println("-------------------------------");
 Serial.println("\nAtHomeDetector\n");
 Serial.println("Commands:");
 Serial.println("   1 / 0 = show received mac addresses on/off");
 Serial.println("   + / - = lcd contrast");
 Serial.println("   u / d = trigger time adjust");
 Serial.println("       r = reset all to defaults");
 Serial.println("       s = show system status\n");
 Serial.println("-------------------------------");

}


// -------------------------------------------------------------------


// search list of friends for matching MAC address

void cb(esppl_frame_info *info) { 
 // show mac address 
   if (showMacs) {
     Serial.print("MAC: source:");
     for (int i = 0; i < 6; i++) Serial.printf("%02x", info->sourceaddr[i]);
     Serial.print(" receive:");
     for (int i = 0; i < 6; i++) Serial.printf("%02x", info->receiveraddr[i]);
     Serial.println("");    // new line
   }
 for (int i=0; i<LIST_SIZE; i++) { 
   if (maccmp(info->sourceaddr, friendmac[i]) || maccmp(info->receiveraddr, friendmac[i])) { 
    
     // device found
       if (showMacs) Serial.println("Known: " + friendname[i]);

     // log the event
       lastDevice = friendname[i];              // store last device seen
       lastSeenOther = millis();  
       if (i == 0) lastSeenPerson = millis();    // if it is Person's phone
       
   }
 } 
} 


// compare two mac addresses 
bool maccmp(uint8_t *mac1, uint8_t *mac2) { 
 for (int i=0; i < ESPPL_MAC_LEN; i++) { 
   if (mac1[i] != mac2[i]) { 
     return false; 
   } 
 } 
 return true; 
} 


// -------------------- e n d --------------------
