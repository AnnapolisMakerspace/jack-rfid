/*
 * MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin			Pin
 *            Arduino Uno      Arduino Mega      SPARK			MFRC522 board
 * ---------------------------------------------------------------------------
 * Reset      9                5                 ANY (D2)		RST
 * SPI SS     10               53                ANY (A2)		SDA
 * SPI MOSI   11               51                A5				MOSI
 * SPI MISO   12               50                A4				MISO
 * SPI SCK    13               52                A3				SCK
 */

//#include <SPI.h>
#include "MFRC522.h"

#define SS_PIN A2  //SS
#define RST_PIN D2
#define DEVICE_PIN D7

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance.

String currentToken;
String currentRFID; //since we are dealing with webhooks, we need a way to pass around the rfid between listeners


void setup() {
	
	currentRFID = "";
	currentToken = "";
	
	pinMode(DEVICE_PIN, OUTPUT);
	
	mfrc522.setSPIConfig();
    mfrc522.PCD_Init();	// Init MFRC522 card
	
	Particle.subscribe("hook-response/getApricotToken", tokenResponseHandler, MY_DEVICES); //handler for the token response
	Particle.subscribe("hook-response/103getMemberByFilter", getMemberByFilterResponseHandler, MY_DEVICES); //handler for the memberlist response

	Particle.publish("RFID reader started up.");    //send this debug message to particle cloud to monitor startups remotely
	delay(250);
	/*DEBUG - SEND RFID TO LOOKUP
	byte debugRFID[] = { 0x25, 0x04, 0xA5, 0x60, 0xE4 };
	String rfid = String();
	for (byte i = 0; i<5; i++) {
        rfid = rfid + (debugRFID[i] < 0x64 ? "0" : "");
        rfid = rfid + (debugRFID[i] < 0x0A ? "0" : "");
        rfid = rfid + String(debugRFID[i], DEC);
    }
    Particle.publish("Debug rfid: " + String(rfid));
    delay(1000);
	currentRFID = rfid;
	getApricotToken();
	/*  END DEBUG */
}

void loop() {
	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

    //if we get here, an rfid device was read
    cardWasRead(mfrc522);
	delay(500);  // debounce
}

void cardWasRead(MFRC522 mfrc522){
    //need to convert bytes to string for web request
    String rfid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        rfid = rfid + (mfrc522.uid.uidByte[i] < 0x64 ? "0" : ""); //if less than 100 add 0
        rfid = rfid + (mfrc522.uid.uidByte[i] < 0x0A ? "0" : ""); //if less than 10 add another zero
        rfid = rfid + String(mfrc522.uid.uidByte[i], DEC);
    }
    currentRFID = rfid;
    getApricotToken();
}


void getApricotToken(){
    //the token is needed to make API calls, expires every 30 min
    //this webhook is created / modified in the particle portal
    Particle.publish("getApricotToken", PRIVATE); //data should not be needed in this call
}

void tokenResponseHandler(const char *event, const char *data) {
  //we have token, next step is to get the member
  currentToken = String(data);
  delay(250);
  Particle.publish("103getMemberByFilter", "{ \"token\": \"" + currentToken + "\", \"rfid\": \"" + currentRFID + "\"}", PRIVATE); //still need to setup this webhook in particle app
}

void getMemberByFilterResponseHandler(const char *event, const char *data) {
    char resp[100];
    strcpy(resp, data);
    
    
    char * name;
    char * email;
    
    //is sizeof always going to return 100? in that case we should test for NULL on strtok
    
    if((sizeof(resp)/sizeof(char)) < 2){
        //no result found in wild apricot
        Particle.publish("makerspaceNotFound(empty array)", currentRFID , PRIVATE);
    } else {
        //member was found (format is xxx@email.com~Lastname, Firstname)
        email = strtok(resp, "~");
        name = strtok(NULL, "~");
        
        if(email != NULL && name != NULL){
            Particle.publish("makerspaceDevice: ", String(email), PRIVATE);
            triggerDevice();
        } else {
            Particle.publish("makerspaceNotFound(null pointers)", currentRFID , PRIVATE);
        }
    }
    currentRFID = "";
    currentToken = "";
}


void triggerDevice(){
    bool deviceOn = false;              //con be configured differently depending on device
    if(deviceOn == false){
        digitalWrite(DEVICE_PIN, HIGH);
    }
}






          









