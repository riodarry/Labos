#include <LiquidCrystal_I2C.h>
#include <OneButton.h>
#include <DHT.h>
#include <HCSR04.h>

#define LED_PIN 9
#define PHOTO_PIN A0
#define BTN_PIN 4
#define DHTPIN 7
#define DHTTYPE DHT11
#define TRIGGER_PIN 12
#define ECHO_PIN 11
#define LCD_ADDR 0x27

enum AppState { DEMARRAGE, LUM_DIST, TEMP_HUM, CALIBRATION };

AppState currentState = DEMARRAGE;

unsigned long currentTime = 0;


int valeurLumiereBrute = 0;
int valeurLumierePourcent = 0;
int lumiereMin = 1023;
int lumiereMax = 0;

float distanceCm = 0;
float temperature = 0;
float humidite = 0;
int valMinimum=0;
int valMaximum=100;


LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneButton button(BTN_PIN, true);
DHT dht(DHTPIN, DHTTYPE);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);


void initPins();
void lcdInit();
void buttonInit();

void simpleClick();
void doubleClick();

void bootState(unsigned long ct);
void etatLumDist(unsigned long ct);
void etatTempHum(unsigned long ct);
void etatCalibration(unsigned long ct);

void lireLaPhotoresistance(unsigned long ct);
void lireLeDHT(unsigned long ct);
void lireLaDistance(unsigned long ct);
void gererLed();
void affichageSerie(unsigned long ct);

void setup() {
  Serial.begin(9600);

  initPins();
  lcdInit();
  buttonInit();
  dht.begin();
}

void loop() {
  currentTime = millis();

  button.tick();

  switch (currentState) {
    case DEMARRAGE:
      bootState(currentTime);
      break;

    case LUM_DIST:
      etatLumDist(currentTime);
      break;

    case TEMP_HUM:
      etatTempHum(currentTime);
      break;

    case CALIBRATION:
      etatCalibration(currentTime);
      break;
  }

  lireLaPhotoresistance(currentTime);
  lireLeDHT(currentTime);
  lireLaDistance(currentTime);
  gererLed();
  affichageSerie(currentTime);
}

void initPins() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
}

void lcdInit() {
  lcd.init();
  lcd.backlight();
}

void buttonInit() {
  button.attachClick(simpleClick);
  button.attachDoubleClick(doubleClick);
}

void simpleClick() {
  if (currentState == CALIBRATION) {
    currentState = LUM_DIST;
  }
  else if (currentState == LUM_DIST) {
    currentState = TEMP_HUM;
  }
  else if (currentState == TEMP_HUM) {
    currentState = LUM_DIST;
  }
}

void doubleClick() {
  lumiereMin = 1023;
  lumiereMax = 0;
  currentState = CALIBRATION;
}

void lireLaPhotoresistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 1000;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    valeurLumiereBrute = analogRead(PHOTO_PIN);

    if (lumiereMax > lumiereMin) {
      valeurLumierePourcent = map(valeurLumiereBrute, lumiereMin, lumiereMax, valMinimum, valMaximum);
    }
  }
}

void lireLeDHT(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 5000;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    float nouvelleHumidite = dht.readHumidity();
    float nouvelleTemperature = dht.readTemperature();

    if (isnan(nouvelleHumidite) || isnan(nouvelleTemperature)) {
      Serial.println("Echec de lecture du DHT!");
      return;
    }

    humidite = nouvelleHumidite;
    temperature = nouvelleTemperature;
  }
}

void lireLaDistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 250;

  if (ct - lastTime >= rate) {
    lastTime = ct;
    distanceCm = hc.dist();
  }
}

void gererLed() {
  if (valeurLumierePourcent < 30) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void affichageSerie(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 3000;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    Serial.print("Lum:");
    Serial.print(valeurLumierePourcent);
    Serial.print(",Min:");
    Serial.print(lumiereMin);
    Serial.print(",Max:");
    Serial.print(lumiereMax);
    Serial.print(",Dist:");
    Serial.print(distanceCm);
    Serial.print(",T:");
    Serial.print(temperature);
    Serial.print(",H:");
    Serial.println(humidite);
  }
}

void bootState(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long lcdRate = 250;
  const unsigned long exitTime = 3000;

  if (ct - lastTime >= lcdRate) {
    lastTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Demarrage...");
    lcd.setCursor(0, 1);
    lcd.print("Maison_01");
  }

  if (ct >= exitTime) {
    currentState = LUM_DIST;
  }
}

void etatLumDist(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long lcdRate = 250;

  if (ct - lastTime >= lcdRate) {
    lastTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lum : ");
    lcd.print(valeurLumierePourcent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Dist : ");
    lcd.print(distanceCm);
    lcd.print(" cm");
  }
}

void etatTempHum(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long lcdRate = 250;

  if (ct - lastTime >= lcdRate) {
    lastTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp : ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Hum : ");
    lcd.print(humidite);
    lcd.print(" %");
  }
}

void etatCalibration(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long lcdRate = 250;

  if (valeurLumiereBrute < lumiereMin) {
    lumiereMin = valeurLumiereBrute;
  }

  if (valeurLumiereBrute > lumiereMax) {
    lumiereMax = valeurLumiereBrute;
  }

  if (ct - lastTime >= lcdRate) {
    lastTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration");

    lcd.setCursor(0, 1);
    lcd.print("Min:");
    lcd.print(lumiereMin);
    lcd.print(" Max:");
    lcd.print(lumiereMax);
  }
}
