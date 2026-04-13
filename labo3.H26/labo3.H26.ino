#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define BTN_PIN 2
#define X_PIN A2
#define Y_PIN A1

const int led = 8;
const int thermistance = A0;
const String nomFamille = "NGUEUDAM";

const int maxAnalog = 1023;
const int minAnalog = 0;
const int milieuAnalog = maxAnalog / 2;

const unsigned long numeroEtudiant = 2409626;
const int deuxDerniersChiffres = 26;

const int temperatureOn = 25;
const int temperatureOff = 24;

const int positionMin = -100;
const int positionMax = 100;
const int vitesseCmSeconde = 10;

unsigned long tempsActuel = 0;

int positionX = 0;
int positionY = 0;

bool climAllumee = false;

byte customChar1[8] = {
  0b00100,
  0b01100,
  0b10100,
  0b11111,
  0b00001,
  0b00111,
  0b00001,
  0b00111
};

byte customChar2[8] = {
  0b01110,
  0b11011,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
  0b11111
};

byte customDegre[8] = {
  0b01110,
  0b10001,
  0b10001,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

void affichageDemarrage() {
  unsigned long debut = millis();

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, customChar1);
  lcd.createChar(1, customChar2);
  lcd.createChar(2, customDegre);

  while (millis() - debut < 3000) {
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(nomFamille);

    lcd.setCursor(0, 1);
    lcd.write(1);

    lcd.setCursor(13, 1);
    lcd.write(0);

    lcd.setCursor(14, 1);
    lcd.print(deuxDerniersChiffres);
  }

  lcd.clear();
}

float lireTemperature() {
  int valeurBrute = analogRead(thermistance);

  float temperature = map(valeurBrute, 0, 1023, 0, 50);
  return temperature;
}

void gererClimatisation(float temperature) {
  if (temperature > temperatureOn) {
    climAllumee = true;
  }
  else if (temperature < temperatureOff) {
    climAllumee = false;
  }

  digitalWrite(led, climAllumee);
}

void afficherPageTemperature() {
  static unsigned long dernierAffichage = 0;

  if (tempsActuel - dernierAffichage >= 100) {
    dernierAffichage = tempsActuel;

    float temperature = lireTemperature();

    gererClimatisation(temperature);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print((int)temperature);
    lcd.write(2);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("AC : ");
    lcd.print(climAllumee ? "ON " : "OFF");

    Serial.print("Brute A0: ");
    Serial.print(analogRead(thermistance));
    Serial.print("  Temp: ");
    Serial.println(temperature);
  }
}

void mettreAJourPosition() {
  static unsigned long dernierTemps = 0;

  if (tempsActuel - dernierTemps >= 1000) {
    dernierTemps = tempsActuel;

    int valeurX = analogRead(X_PIN);
    int valeurY = analogRead(Y_PIN);

    if (valeurX < 400) positionX -= vitesseCmSeconde;
    else if (valeurX > 600) positionX += vitesseCmSeconde;

    if (valeurY < 400) positionY -= vitesseCmSeconde;
    else if (valeurY > 600) positionY += vitesseCmSeconde;

    if (positionX < positionMin) positionX = positionMin;
    if (positionX > positionMax) positionX = positionMax;

    if (positionY < positionMin) positionY = positionMin;
    if (positionY > positionMax) positionY = positionMax;
  }
}

void afficherPagePosition() {
  static unsigned long dernierAffichage = 0;

  if (tempsActuel - dernierAffichage >= 100) {
    dernierAffichage = tempsActuel;

    mettreAJourPosition();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("X : ");
    lcd.print(positionX);

    lcd.setCursor(0, 1);
    lcd.print("Y : ");
    lcd.print(positionY);
  }
}

bool boutonAppuye(unsigned long now) {
  static int etatPrecedent = 0;
  static unsigned long dernierTemps = 0;
  const int debounce = 50;

  int etatPresent = !digitalRead(BTN_PIN);

  if (now - dernierTemps < debounce) return false;

  if (etatPresent != etatPrecedent) {
    dernierTemps = now;
    etatPrecedent = etatPresent;

    if (etatPresent == 1) return true;
  }

  return false;
}

void affichageSerie() {
  static unsigned long dernierEnvoi = 0;

  if (tempsActuel - dernierEnvoi >= 100) {
    dernierEnvoi = tempsActuel;

    int valX = analogRead(X_PIN);
    int valY = analogRead(Y_PIN);
    int valSys = climAllumee ? 1 : 0;

    Serial.print("etd:");
    Serial.print(numeroEtudiant);
    Serial.print(",x:");
    Serial.print(valX);
    Serial.print(",y:");
    Serial.print(valY);
    Serial.print(",sys:");
    Serial.println(valSys);
  }
}

void setup() {
  pinMode(led, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  affichageDemarrage();
}

void loop() {
  tempsActuel = millis();

  static bool pageTemperature = true;

  if (boutonAppuye(tempsActuel)) {
    pageTemperature = !pageTemperature;
    lcd.clear();
  }

  if (pageTemperature) afficherPageTemperature();
  else afficherPagePosition();

  affichageSerie();
}