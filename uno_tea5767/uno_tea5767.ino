/*
  Effaroucheur avec TEA5767 et un arduino nano
  frequences radio pre-enregistrees
  jlm 2025 03
*/

#include <EEPROM.h>
#include <Wire.h>
#include <TEA5767N.h>
#include <LiquidCrystal_I2C.h>
#include <Button.h>  // montee , descente, et appui sur les deux

LiquidCrystal_I2C lcd(0x3f, 16, 2);  // adress nb colonnes, nb lignes 0x27 0x3f

//Constants:
TEA5767N Radio = TEA5767N();  // Using standard I2C  pins A4 and A5
/*
public:
	  TEA5767N();
	  void selectFrequency(float);
	  void selectFrequencyMuting(float);
	  void mute();
	  void turnTheSoundBackOn();
	  void muteLeft();
	  void turnTheLeftSoundBackOn();
	  void muteRight();
	  void turnTheRightSoundBackOn();
	  float readFrequencyInMHz();
	  void setSearchUp();
	  void setSearchDown();
	  void setSearchLowStopLevel();
	  void setSearchMidStopLevel();
	  void setSearchHighStopLevel();
	  void setStereoReception();
	  void setMonoReception();
	  void setSoftMuteOn();
	  void setSoftMuteOff();
	  
	  void setStandByOn();
	  void setStandByOff();
	  void setHighCutControlOn();
	  void setHighCutControlOff();
	  void setStereoNoiseCancellingOn();
	  void setStereoNoiseCancellingOff();
	  
	  byte searchNext();
	  byte searchNextMuting();
	  byte startsSearchFrom(float frequency);
	  byte startsSearchFromBeginning();
	  byte startsSearchFromEnd();
	  byte startsSearchMutingFromBeginning();
	  byte startsSearchMutingFromEnd();
	  byte getSignalLevel();
	  byte isStereo();
	  byte isSearchUp();
	  byte isSearchDown();
	  boolean isMuted();
	  boolean isStandBy();
*/
/*
   read KEYWORD2
   toggled KEYWORD2
   pressed KEYWORD2
   released KEYWORD2
   has_changed KEYWORD2
*/

//==========
// Variables
//==========
#define LED_PIN_FLASH 6  //pin connected the flash
#define LDR_PIN A0       //pin ldr

bool debug = 1;

// pin button
#define P1 2
#define B2 4
#define C3 8
#define MA 7

// frequence default
float freq_default = 90.10;
// Predefined stations array
float defaultStations[16] = { 90.10, 89.50, 90.10, 90.90, 92.50, 93.0, 94.30, 98.80, 100.70, 107, 107.9, 90.10, 90.10, 90.10, 90.10, 90.10 };
// adresse eeprom
int address = 0;
float freq = 0.0;
int ldr = 0;
unsigned long currentTime = 0;
unsigned long previousTime = 0;
// interval1 radio allumee,  interval2 radio eteinte, reglages
unsigned long interval = 0, interval1 = 15000, interval2 = 180000, interval3 = 20000;
int etat = 0;
byte stationIndex = 0;

//=======
// Button
//=======
Button buttonDroite(P1);  // +
Button buttonGauche(B2);  // -
Button buttonMA(MA);      // marche forcee radio ou automatique
Button buttonTmps(C3);    // temps de l'interval

// level indicator
void updateLevelIndicator() {
  byte x, y, sl;
  char barGraph[17];

  lcd.setCursor(0, 1);
  sl = Radio.getSignalLevel();
  if (debug) {
    Serial.print("niveau de reception : ");
    Serial.println(sl);
  }
  for (x = 0; x < sl; x++) {
    barGraph[x] = 255;
  }
  for (y = x; y < 16; y++) {
    barGraph[y] = 32;
  }
  barGraph[y] = '\0';
  lcd.print(barGraph);
}

//setup
void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  if (debug) {
    Serial.println("Demarrage du module");
  }
  // initialize the lcd
  lcd.init();
  //retro eclairage
  //lcd.backlight();
  lcd.noBacklight();

  pinMode(LED_PIN_FLASH, OUTPUT);
  digitalWrite(LED_PIN_FLASH, LOW);  //  flash

  pinMode(LDR_PIN, INPUT);

  // sauvegarde de la frequence dans l'eeprom si absente
  if (EEPROM.get(address, freq) < 87.5 or EEPROM.get(address, freq) > 108) {
    EEPROM.put(address, freq_default);
  }

  lcd.setCursor(0, 0);
  lcd.print(EEPROM.get(address, freq));
  lcd.setCursor(6, 0);
  lcd.print("MHz");
  lcd.setCursor(12, 0);
  lcd.print((interval2 / 10000) / 6);
  lcd.setCursor(14, 0);
  lcd.print("Mn");
  if (debug) {
    Serial.print("freq : ");
    Serial.println(freq);
  }
  Radio.selectFrequency(EEPROM.get(address, freq));
  updateLevelIndicator();

  //pinMode(P1, INPUT_PULLUP);  // si absence de resistance
  //pinMode(B2, INPUT_PULLUP);
  //pinMode(C3, INPUT_PULLUP);
  buttonDroite.begin();
  buttonGauche.begin();
  buttonMA.begin();
  buttonTmps.begin();
}

void loop() {

  ldr = analogRead(LDR_PIN);
  currentTime = millis();

  if (buttonMA.read() == LOW) {  // marche forcee radio
    Radio.turnTheSoundBackOn();
    //Radio.setStandByOff();

  } else if (ldr < 100) {  // si lumiere  faible alors
    Radio.mute();
    //Radio.setStandByOn();

  } else {
    if (etat == 0) {
      Radio.turnTheSoundBackOn();
      //Radio.setStandByOff();
      digitalWrite(LED_PIN_FLASH, HIGH);  //  flash
      interval = interval1;
    } else if (etat == 1) {
      Radio.mute();
      //Radio.setStandByOn();
      digitalWrite(LED_PIN_FLASH, LOW);  //  flash
      interval = interval2;
    } else if (etat == 2) {
      Radio.turnTheSoundBackOn();
      //Radio.setStandByOff();
      interval = interval3;
    }

    // calcul du temps de fonctionnement
    if ((currentTime - previousTime) > interval) {
      previousTime = currentTime;
      if (etat == 0) {
        etat = 1;
      } else if (etat == 1) {
        etat = 0;
      } else if (etat == 2) {
        etat = 0;
      }
      if (debug) {
        Serial.print("ldr : ");
        Serial.print(ldr);
        Serial.print("    etat : ");
        Serial.println(etat);
        Serial.print("interval : ");
        Serial.println(interval);
      }
    }
  }

  if ((buttonDroite.read() == LOW) and (buttonGauche.read() == LOW)) {  // C3 memoire des stations enregistrees
// scan frequences
/*
    Serial.println("P1 et B2");
    for (float i = 87.5; i <= 108.0; i += 0.1) {
      Serial.print("*");
      Radio.selectFrequency(i);
      delay(500);
      float sl = Radio.getSignalLevel();
      if (sl > 7) {
        Serial.println();
        lcd.setCursor(0, 0);
        lcd.print(i);
        lcd.setCursor(6, 0);
        lcd.print("MHz");
        Serial.print("   freq : ");
        Serial.print(i);
        updateLevelIndicator();
        delay(3000);
      }
    }
*/
    Radio.turnTheSoundBackOn();
    //Radio.setStandByOff();
    etat = 2;
    if (debug) {
      Serial.print("P1 et B2");
    }
    stationIndex = stationIndex + 1;
    if (stationIndex == 16) {
      stationIndex = 0;
    }
    EEPROM.put(address, defaultStations[stationIndex]);
    Radio.selectFrequency(EEPROM.get(address, freq));
    if (debug) {
      Serial.print("   freq : ");
      Serial.println(defaultStations[stationIndex]);
    }
    lcd.setCursor(0, 0);
    lcd.print(EEPROM.get(address, freq));
    lcd.setCursor(6, 0);
    lcd.print("MHz");
    updateLevelIndicator();
  } else if (buttonDroite.read() == LOW) {  // P1
    Radio.turnTheSoundBackOn();
    //Radio.setStandByOff();
    etat = 2;
    if (debug) {
      Serial.print("P1");
    }
    freq += 0.1;
    if (freq > 108.0) {
      freq = 87.5;
    }
    EEPROM.put(address, freq);
    Radio.selectFrequency(EEPROM.get(address, freq));
    if (debug) {
      Serial.print("   freq : ");
      Serial.println(freq);
    }
    lcd.setCursor(0, 0);
    lcd.print(EEPROM.get(address, freq));
    lcd.setCursor(6, 0);
    lcd.print("MHz");
    updateLevelIndicator();
  } else if (buttonGauche.read() == LOW) {  // B2
    Radio.turnTheSoundBackOn();
    //Radio.setStandByOff();
    etat = 2;
    if (debug) {
      Serial.print("B2");
    }
    freq -= 0.1;
    if (freq < 87.5) {
      freq = 108.0;
    }
    EEPROM.put(address, freq);
    Radio.selectFrequency(EEPROM.get(address, freq));
    if (debug) {
      Serial.print("   freq : ");
      Serial.println(freq);
    }
    lcd.setCursor(0, 0);
    lcd.print(EEPROM.get(address, freq));
    lcd.setCursor(6, 0);
    lcd.print("MHz");
    updateLevelIndicator();
  } else if (buttonTmps.pressed()) {  // C3
    if (debug) {
      Serial.println("interval temps : ");
    }
    // de minute en minute pour l'interval entre deux emissions radio
    interval2 += 60000;
    if (interval2 > 900000) {
      interval2 = 60000;
      lcd.setCursor(12, 0);
      lcd.print("  ");
    }
  lcd.setCursor(12, 0);
  lcd.print((interval2 / 10000) / 6);
  lcd.setCursor(14, 0);
  lcd.print("Mn");
  }
}
