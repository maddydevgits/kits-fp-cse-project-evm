#include <Adafruit_Fingerprint.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

SoftwareSerial mySerial(10, 11);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

// Buttons
int enrollButton=2;
int identifyButton=3;
int partyAButton=4;
int partyBButton=5;
int resultButton=6;

int buzzer=13;

int partyA=0, partyB=0, partyA1=0,partyB1=0;

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void setup() {
  lcd.init();                    
  lcd.backlight();
  lcd.print("WELCOME TO ");
  lcd.setCursor(0,1);
  lcd.print("VOTING SYSTEM");
  
  pinMode(enrollButton,INPUT_PULLUP);
  pinMode(identifyButton,INPUT_PULLUP);
  pinMode(partyAButton,INPUT_PULLUP);
  pinMode(partyBButton,INPUT_PULLUP);
  pinMode(resultButton,INPUT_PULLUP);
  pinMode(buzzer,OUTPUT);

  Serial.begin(9600);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

}

void loop() {
  if(digitalRead(resultButton)==0) {
    Serial.println("Results are Out");
    digitalWrite(buzzer,1);
    while(digitalRead(resultButton)==0);
    digitalWrite(buzzer,0);
    partyA1=EEPROM.read(100);
    partyB1=EEPROM.read(101);
    Serial.print("Party A: ");
    Serial.print(partyA1);
    Serial.print(", Party B: ");
    Serial.print(partyB1);
    Serial.println();
    
    if(partyA1>partyB1) {
      lcd.clear();
      lcd.print("Party A Won");
      Serial.println("Party A Won");
    } else if(partyB1>partyA1){
      lcd.clear();
      lcd.print("Party B Won");
      Serial.println("Party B Won");
    } else {
      lcd.clear();
      lcd.print("Both Tie");
      Serial.println("Both Tie");
    }
    for(int i=0;i<255;i++) {
      EEPROM.write(i,0);
    }
    lcd.setCursor(0,1);
    lcd.print("Voting Completed");
  }
  if(digitalRead(enrollButton)==0) {
    lcd.clear();
    lcd.print("Enrolling");
    Serial.println("Enrolled");
    digitalWrite(buzzer,1);
    while(digitalRead(enrollButton)==0);
    digitalWrite(buzzer,0);
    enrollingFP();
    lcd.clear();
    lcd.print("Enrolled");
    delay(4000);
    lcd.clear();
    lcd.print("Please Vote");
  }
  if(digitalRead(identifyButton)==0){
    lcd.clear();
    lcd.print("Identifying");
    Serial.println("Identified");
    digitalWrite(buzzer,1);
    while(digitalRead(identifyButton)==0);
    digitalWrite(buzzer,0);
    getFingerprintID();
    lcd.clear();
    lcd.print("Identified");
    delay(4000);
    lcd.clear();
    lcd.print("Please Vote");
  }
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  
  if(EEPROM.read(finger.fingerID)==1) {
    lcd.clear();
    lcd.print("You have voted Already");
    Serial.println("You have voted already");
  } else {
    lcd.clear();
    lcd.print("Voting....");
    Serial.println("You have voted now");
    while(digitalRead(partyAButton)==1 && digitalRead(partyBButton)==1);
    if(digitalRead(partyAButton)==0) {
      partyA++;
      EEPROM.write(100,partyA);
      Serial.println("Voted for Party A");
    } 
    if(digitalRead(partyBButton)==0){
      partyB++;
      EEPROM.write(101,partyB);
      Serial.println("Voted for Party B");
    }
    digitalWrite(buzzer,1);
    lcd.clear();
    lcd.print("You have Voted");
    EEPROM.write(finger.fingerID,1);
    delay(2000);
    digitalWrite(buzzer,0);
    lcd.clear();
    lcd.print("Please Vote");
  }
  return finger.fingerID;
}


void enrollingFP(void) {
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (!  getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}
