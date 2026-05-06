#include <OneButton.h>
#include <HCSR04.h>
#include <DHT.h>
#include <LCD_I2C.h>
#include <AccelStepper.h>
#include "Convoyeur.h"

enum EtatLCD { BOOT, DHT_STATE, CALIB_STATE, LUM_DIST_STATE, VANNE_STATE } etatLCD = BOOT;
enum EtatIrrigation { FERME, OUVERTURE, OUVERT, FERMETURE, ARRET } etatIrrigation = FERMETURE;

const int LED_PIN = 9;
const int PHOTO_PIN = A0;
const int BTN_PIN = 4;
const int ECHO_PIN = 11;
const int TRIGGER_PIN = 12;
const int DHT_PIN = 7;
const int LCD_ADDR = 0x27;
const int LCD_ROWS = 2;
const int LCD_COLS = 16;

const int STEPPER_IN1 = 31;
const int STEPPER_IN2 = 33;
const int STEPPER_IN3 = 35;
const int STEPPER_IN4 = 37;

const byte CONV_MOTEUR_PIN_1 = 44;
const byte CONV_MOTEUR_PIN_2 = 45;
const byte CONV_JOYSTICK_AXE_PIN = A1;
const byte CONV_JOYSTICK_BTN_PIN = 2;
const byte CONV_AFF_CLK = 30;
const byte CONV_AFF_DIN = 34;
const byte CONV_AFF_CS = 32;
const bool NUMERO_ETUDIANT_PAIR = true;

const int VANNE_FERMEE = 0;
const int VANNE_OUVERTE = 2038;
const int MOTOR_INTERFACE_TYPE = 4;
const int NIVEAU_MIN_CM = 20;
const int NIVEAU_MAX_CM = 25;

OneButton btn(BTN_PIN, true);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
DHT dht(DHT_PIN, DHT11);
LCD_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
AccelStepper moteur(MOTOR_INTERFACE_TYPE, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);
Convoyeur convoyeur(CONV_MOTEUR_PIN_1, CONV_MOTEUR_PIN_2, CONV_JOYSTICK_AXE_PIN, CONV_JOYSTICK_BTN_PIN, CONV_AFF_CLK, CONV_AFF_DIN, CONV_AFF_CS);

int lumiereBrute = 0;
int lumierePct = 0;
int lumiereMin = 1023;
int lumiereMax = 0;
float distanceCm = 0;
float temperature = 0;
float humidite = 0;
int positionVanne = VANNE_OUVERTE;

void initMateriel();
void clicBouton();
void doubleClicBouton();
void synchroniserPositionMoteur();

void gestionnaireEtatLCD(unsigned long ct);
void gestionnaireEtatIrr(unsigned long ct);

void bootState(unsigned long ct);
void dhtState(unsigned long ct);
void calibState(unsigned long ct);
void lumDistState(unsigned long ct);
void vanneLCDState(unsigned long ct);

void fermeState(unsigned long ct);
void ouvertureState(unsigned long ct);
void ouvertState(unsigned long ct);
void fermetureState(unsigned long ct);
void arretState(unsigned long ct);

void lumiereTache(unsigned long ct);
void distanceTache(unsigned long ct);
void tempHumTache(unsigned long ct);
void ledTache(unsigned long ct);
void envoieSerieTache(unsigned long ct);

void changerEtatLCD(EtatLCD nouvelEtat);
void changerEtatIrrigation(EtatIrrigation nouvelEtat);
void afficherTexteFixe(const char *ligne1, const char *ligne2);
int pourcentageVanne();
bool vanneBouge();

void setup() {
  Serial.begin(115200);
  initMateriel();
}

void loop() {
  unsigned long currentTime = millis();

  btn.tick();
  convoyeur.update();
  moteur.run();
  synchroniserPositionMoteur();

  lumiereTache(currentTime);
  distanceTache(currentTime);
  tempHumTache(currentTime);
  gestionnaireEtatIrr(currentTime);
  ledTache(currentTime);
  gestionnaireEtatLCD(currentTime);
  envoieSerieTache(currentTime);
}

void initMateriel() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);

  lcd.begin();
  lcd.backlight();
  dht.begin();

  convoyeur.setNumeroEtudiantPair(NUMERO_ETUDIANT_PAIR);
  convoyeur.begin();

  moteur.setMaxSpeed(500);
  moteur.setAcceleration(100);
  moteur.setSpeed(200);
  moteur.setCurrentPosition(VANNE_OUVERTE);
  moteur.moveTo(VANNE_FERMEE);

  btn.setDebounceMs(30);
  btn.attachClick(clicBouton);
  btn.attachDoubleClick(doubleClicBouton);
}

void clicBouton() {
  if (etatIrrigation == OUVERTURE || etatIrrigation == FERMETURE) {
    changerEtatIrrigation(ARRET);
    return;
  }

  if (etatIrrigation == ARRET) {
    changerEtatIrrigation(OUVERTURE);
    return;
  }

  if (etatLCD == LUM_DIST_STATE) {
    changerEtatLCD(DHT_STATE);
  } else if (etatLCD == DHT_STATE || etatLCD == CALIB_STATE || etatLCD == VANNE_STATE) {
    changerEtatLCD(LUM_DIST_STATE);
  }
}

void doubleClicBouton() {
  if (vanneBouge() || etatIrrigation == ARRET) {
    return;
  }

  lumiereMin = 1023;
  lumiereMax = 0;
  changerEtatLCD(CALIB_STATE);
}

void gestionnaireEtatLCD(unsigned long ct) {
  if (vanneBouge() || etatIrrigation == ARRET) {
    vanneLCDState(ct);
    return;
  }

  switch (etatLCD) {
    case BOOT:
      bootState(ct);
      break;
    case DHT_STATE:
      dhtState(ct);
      break;
    case CALIB_STATE:
      calibState(ct);
      break;
    case LUM_DIST_STATE:
      lumDistState(ct);
      break;
    case VANNE_STATE:
      vanneLCDState(ct);
      break;
  }
}

void gestionnaireEtatIrr(unsigned long ct) {
  switch (etatIrrigation) {
    case FERME:
      fermeState(ct);
      break;
    case OUVERTURE:
      ouvertureState(ct);
      break;
    case OUVERT:
      ouvertState(ct);
      break;
    case FERMETURE:
      fermetureState(ct);
      break;
    case ARRET:
      arretState(ct);
      break;
  }
}

void bootState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;
  const unsigned long dureeBoot = 3000;

  if (ct - previousTime >= rate) {
    previousTime = ct;
    afficherTexteFixe("Demarrage...", "Serre_03");
  }

  if (ct >= dureeBoot) {
    changerEtatLCD(LUM_DIST_STATE);
  }
}

void dhtState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(temperature, 1);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(humidite, 1);
  lcd.print(" %");
}

void calibState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;

  if (lumiereBrute < lumiereMin) {
    lumiereMin = lumiereBrute;
  }

  if (lumiereBrute > lumiereMax) {
    lumiereMax = lumiereBrute;
  }

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration");
  lcd.setCursor(0, 1);
  lcd.print("Min:");
  lcd.print(lumiereMin);
  lcd.print(" Max:");
  lcd.print(lumiereMax);
}

void lumDistState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lum:");
  lcd.print(lumierePct);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Dist:");
  lcd.print(distanceCm, 1);
  lcd.print(" cm");
}

void vanneLCDState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vanne:");
  lcd.print(pourcentageVanne());
  lcd.print("%");
  lcd.setCursor(0, 1);

  switch (etatIrrigation) {
    case OUVERTURE:
      lcd.print("Ouverture");
      break;
    case OUVERT:
      lcd.print("Ouverte");
      break;
    case FERMETURE:
      lcd.print("Fermeture");
      break;
    case FERME:
      lcd.print("Fermee");
      break;
    case ARRET:
      lcd.print("Arret urgence");
      break;
  }
}

void fermeState(unsigned long ct) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    Serial.println("Entree Etat : Ferme");
  }

  if (distanceCm > 0 && distanceCm < NIVEAU_MIN_CM) {
    Serial.println("Sortie Etat : Ferme");
    firstTime = true;
    changerEtatIrrigation(OUVERTURE);
  }
}

void ouvertureState(unsigned long ct) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    Serial.println("Entree Etat : Ouverture");
    moteur.moveTo(VANNE_OUVERTE);
  }

  if (distanceCm >= NIVEAU_MAX_CM) {
    Serial.println("Sortie Etat : Ouverture");
    firstTime = true;
    changerEtatIrrigation(OUVERT);
    return;
  }

  if (moteur.distanceToGo() == 0) {
    Serial.println("Sortie Etat : Ouverture");
    firstTime = true;
    changerEtatIrrigation(OUVERT);
  }
}

void ouvertState(unsigned long ct) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    Serial.println("Entree Etat : Ouvert");
  }

  if (distanceCm >= NIVEAU_MAX_CM) {
    Serial.println("Sortie Etat : Ouvert");
    firstTime = true;
    changerEtatIrrigation(FERMETURE);
  }
}

void fermetureState(unsigned long ct) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    Serial.println("Entree Etat : Fermeture");
    moteur.moveTo(VANNE_FERMEE);
  }

  if (moteur.distanceToGo() == 0) {
    Serial.println("Sortie Etat : Fermeture");
    firstTime = true;
    changerEtatIrrigation(FERME);
  }
}

void arretState(unsigned long ct) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    Serial.println("Entree Etat : Arret");
    moteur.moveTo(moteur.currentPosition());
  }

  if (etatIrrigation != ARRET) {
    firstTime = true;
  }
}

void synchroniserPositionMoteur() {
  positionVanne = constrain((int)moteur.currentPosition(), VANNE_FERMEE, VANNE_OUVERTE);
}

void lumiereTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 1000;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  lumiereBrute = analogRead(PHOTO_PIN);

  if (lumiereMax > lumiereMin) {
    lumierePct = map(lumiereBrute, lumiereMin, lumiereMax, 0, 100);
    lumierePct = constrain(lumierePct, 0, 100);
  }
}

void distanceTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  distanceCm = hc.dist();
}

void tempHumTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 5000;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  float nouvelleHumidite = dht.readHumidity();
  float nouvelleTemperature = dht.readTemperature();

  if (isnan(nouvelleHumidite) || isnan(nouvelleTemperature)) {
    return;
  }

  humidite = nouvelleHumidite;
  temperature = nouvelleTemperature;
}

void ledTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  static bool ledAllumee = false;
  const unsigned long rate = 100;

  if (!vanneBouge()) {
    digitalWrite(LED_PIN, LOW);
    ledAllumee = false;
    return;
  }

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  ledAllumee = !ledAllumee;
  digitalWrite(LED_PIN, ledAllumee);
}

void envoieSerieTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 3000;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  Serial.print("Lum:");
  Serial.print(lumierePct);
  Serial.print(",Min:");
  Serial.print(lumiereMin);
  Serial.print(",Max:");
  Serial.print(lumiereMax);
  Serial.print(",Dist:");
  Serial.print(distanceCm, 1);
  Serial.print(",T:");
  Serial.print(temperature, 1);
  Serial.print(",H:");
  Serial.print(humidite, 1);
  Serial.print(",Van:");
  Serial.print(pourcentageVanne());
  Serial.print(",Conv:");
  Serial.println(convoyeur.getVitesse());
}

void changerEtatLCD(EtatLCD nouvelEtat) {
  etatLCD = nouvelEtat;
}

void changerEtatIrrigation(EtatIrrigation nouvelEtat) {
  etatIrrigation = nouvelEtat;
  changerEtatLCD(VANNE_STATE);
}

void afficherTexteFixe(const char *ligne1, const char *ligne2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(ligne1);
  lcd.setCursor(0, 1);
  lcd.print(ligne2);
}

int pourcentageVanne() {
  long pct = map(positionVanne, VANNE_FERMEE, VANNE_OUVERTE, 0, 100);
  return constrain((int)pct, 0, 100);
}

bool vanneBouge() {
  return etatIrrigation == OUVERTURE || etatIrrigation == FERMETURE;
}
