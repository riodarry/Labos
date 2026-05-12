#include <OneButton.h>
#include <HCSR04.h>
#include <DHT.h>
#include <LCD_I2C.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include "Convoyeur.h"
#include "Irrigation.h"

enum LCDState { ECLAIRAGE, ALARME, AC, DATA_RECEIVED } lcdState = ECLAIRAGE;

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
const byte CONV_AFF_CS =32;
const bool NUMERO_ETUDIANT_PAIR = true;

const int VANNE_FERMEE = 0;
const int VANNE_OUVERTE = 2038;

const char ETUDIANT_NUMERO[] = "2409626";
const int MQTT_NUMERO = 6;
const char WIFI_SSID[] ="TechniquesInformatique-Etudiant";//  "BELL486";//
const char WIFI_PASS[] =  "shawi123";    //                "E6FDE4563D3D"; 
const char MQTT_SERVER[] = "216.128.180.194";
const int MQTT_PORT = 1883;
const char MQTT_USER[] = "etdshawi";
const char MQTT_PASS[] = "shawi123";
const unsigned long AT_BAUD_RATE = 115200;

char mqttPublishTopic[12];
char mqttSubscribeTopic[16];
char mqttSubscribeTopicZero[16];
char mqttSubscribeTopicSlash[16];
char mqttSubscribeTopicSlashZero[16];
char mqttConvVitTopic[20];
char mqttConvVitTopicZero[20];
char mqttConvVitTopicSlash[20];
char mqttConvVitTopicSlashZero[20];
char deviceName[16];
char lcdBuff[2][17] = {
  "                ",
  "                "
};

OneButton btn(BTN_PIN, true);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
DHT dht(DHT_PIN, DHT11);
LCD_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Convoyeur convoyeur(CONV_MOTEUR_PIN_1, CONV_MOTEUR_PIN_2, CONV_JOYSTICK_AXE_PIN, CONV_JOYSTICK_BTN_PIN, CONV_AFF_CLK, CONV_AFF_DIN, CONV_AFF_CS);
Irrigation irrigation(LED_PIN, STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);

int lumiereBrute = 0;
int lumierePct = 0;
int lumiereMin = 1023;
int lumiereMax = 0;
int distanceCm = 0;
float temperature = 0;
float humidite = 0;
int maxmap = 100;
int minmap = 0;
bool irrigationBtnClicked = false;
unsigned long lcdStateChangedAt = 0;
unsigned long currentTime = 0;


void initMateriel();
void initTopics();
void wifiInit();
void mqttInit();
void mqttTask();
void mqttEvent(char *topic, byte *payload, unsigned int length);
bool reconnectMqtt();
void iotPublishTask(unsigned long ct);
void printWifiStatus();
void printMacAddress(byte mac[]);
void clicBouton();
void doubleClicBouton();
void gestionnaireEtatLCD(unsigned long ct);
void eclairageState(unsigned long ct);
void alarmeState(unsigned long ct);
void acState(unsigned long ct);
void dataReceivedState(unsigned long ct);
void vanneLCDState(unsigned long ct);
void lumiereTache(unsigned long ct);
void distanceTache(unsigned long ct);
void tempHumTache(unsigned long ct);
void envoieSerieTache(unsigned long ct);
void changerEtatLCD(LCDState nouvelEtat);
void afficherTexteFixe(const char *ligne1, const char *ligne2);
void afficherLCD();
bool vanneBouge();

void setup() {
  Serial.begin(115200);
  initMateriel();
}

void loop() {
  currentTime = millis();

  btn.tick();
  convoyeur.update();
  irrigation.update();
  mqttTask();

  lumiereTache(currentTime);
  distanceTache(currentTime);
  tempHumTache(currentTime);
  gestionnaireEtatLCD(currentTime);
  envoieSerieTache(currentTime);
  iotPublishTask(currentTime);
}

void initMateriel() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);

  lcd.begin();
  lcd.backlight();
  dht.begin();

  convoyeur.setNumeroEtudiantPair(NUMERO_ETUDIANT_PAIR);
  convoyeur.begin();

  irrigation.setClosedOpenedPos(VANNE_FERMEE, VANNE_OUVERTE);
  irrigation.setDistance(distanceCm);
  irrigation.setBtnClickFlag(irrigationBtnClicked);
  irrigation.begin();

  btn.setDebounceMs(30);
  btn.attachClick(clicBouton);
  btn.attachDoubleClick(doubleClicBouton);

  initTopics();
  wifiInit();
  mqttInit();
}

void initTopics() {
  sprintf(mqttPublishTopic, "etd/%d", MQTT_NUMERO);
  sprintf(mqttSubscribeTopic, "etu_%d/#", MQTT_NUMERO);
  sprintf(mqttSubscribeTopicZero, "etu_%02d/#", MQTT_NUMERO);
  sprintf(mqttSubscribeTopicSlash, "etu/%d/#", MQTT_NUMERO);
  sprintf(mqttSubscribeTopicSlashZero, "etu/%02d/#", MQTT_NUMERO);
  sprintf(mqttConvVitTopic, "etu_%d/convVit", MQTT_NUMERO);
  sprintf(mqttConvVitTopicZero, "etu_%02d/convVit", MQTT_NUMERO);
  sprintf(mqttConvVitTopicSlash, "etu/%d/convVit", MQTT_NUMERO);
  sprintf(mqttConvVitTopicSlashZero, "etu/%02d/convVit", MQTT_NUMERO);
  sprintf(deviceName, "Serre_%d", MQTT_NUMERO);
}

void wifiInit() {
  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("La communication avec le module WiFi a echoue!");
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(50);
    }
  }

  Serial.println("En attente de connexion au WiFi");
  for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connexion au reseau ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  Serial.println("Connecte au reseau WiFi.");
  Serial.print("Adresse : ");
  Serial.println(WiFi.localIP());
  printWifiStatus();
}

void mqttInit() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttEvent);
  mqttClient.setBufferSize(320);

  if (reconnectMqtt()) {
    Serial.println("Connecte sur le serveur MQTT");
  } else {
    Serial.print("client.state : ");
    Serial.println(mqttClient.state());
  }
}

void mqttTask() {
  static unsigned long lastReconnectTry = 0;
  const unsigned long reconnectRate = 5000;

  if (!mqttClient.connected() && currentTime - lastReconnectTry >= reconnectRate) {
    lastReconnectTry = currentTime;
    reconnectMqtt();
  }

  mqttClient.loop();
}

void mqttEvent(char *topic, byte *payload, unsigned int length) {
  char message[16];
  unsigned int msgLength = length;

  if (msgLength > sizeof(message) - 1) {
    msgLength = sizeof(message) - 1;
  }

  for (unsigned int i = 0; i < msgLength; i++) {
    message[i] = (char)payload[i];
  }
  message[msgLength] = '\0';

  Serial.print("Message recu [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (strstr(topic, "convVit") != nullptr) {
    int vitesse = atoi(message);
    bool accepted = convoyeur.setVitesseConsigne(vitesse);

    Serial.print("Consigne convoyeur ");
    if (accepted) {
      Serial.println("acceptee");
    } else {
      Serial.println("ignoree - convoyeur inactif");
    }
    changerEtatLCD(DATA_RECEIVED);
  }
}

bool reconnectMqtt() {
  if (mqttClient.connected()) {
    return true;
  }

  bool result = mqttClient.connect(deviceName, MQTT_USER, MQTT_PASS);

  if (result) {
    mqttClient.subscribe(mqttSubscribeTopic, 0);
    mqttClient.subscribe(mqttSubscribeTopicZero, 0);
    mqttClient.subscribe(mqttSubscribeTopicSlash, 0);
    mqttClient.subscribe(mqttSubscribeTopicSlashZero, 0);
    mqttClient.subscribe("etu/#", 0);
    Serial.print("Abonne a ");
    Serial.println(mqttSubscribeTopic);
    Serial.print("Abonne a ");
    Serial.println(mqttSubscribeTopicZero);
    Serial.print("Abonne a ");
    Serial.println(mqttSubscribeTopicSlash);
    Serial.print("Abonne a ");
    Serial.println(mqttSubscribeTopicSlashZero);
    Serial.println("Abonne a etu/#");
  } else {
    Serial.println("Incapable de se connecter sur le serveur MQTT");
    Serial.print("client.state : ");
    Serial.println(mqttClient.state());
  }

  return result;
}

void iotPublishTask(unsigned long ct) {
  static unsigned long previousTime = 0;
  static bool firstPublish = true;
  const unsigned long rate = 3000;
  char message[300];
  char szTemp[8];
  char szHum[8];

  if (!firstPublish && ct - previousTime < rate) {
    return;
  }

  firstPublish = false;
  previousTime = ct;

  dtostrf(temperature, 4, 1, szTemp);
  dtostrf(humidite, 4, 1, szHum);

  snprintf(
    message,
    sizeof(message),
    "{\"name\":\"%s\",\"temp\":\"%s\",\"hum\":\"%s\",\"millis\":%lu,\"lum\":%d,\"irrState\":%d,\"irrPos\":%d,\"convVit\":%d}",
    ETUDIANT_NUMERO,
    szTemp,
    szHum,
    ct,
    lumierePct,
    irrigation.getCurrentState(),
    irrigation.getPositionPct(),
    convoyeur.getVitesse()
  );

  Serial.print("Envoie MQTT : ");
  Serial.println(message);

  if (!mqttClient.publish(mqttPublishTopic, message)) {
    Serial.println("Incapable d'envoyer le message MQTT!");
    reconnectMqtt();
  }
}

void printWifiStatus() {
  char ssid[33];
  WiFi.SSID(ssid);
  Serial.print("SSID: ");
  Serial.println(ssid);

  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  printMacAddress(mac);

  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());

  long rssi = WiFi.RSSI();
  Serial.print("force du signal (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void clicBouton() {
  if (irrigation.isMoving() || irrigation.getCurrentState() == ARRET) {
    irrigationBtnClicked = true;
    return;
  }

  if (lcdState == ECLAIRAGE) {
    changerEtatLCD(ALARME);
  } else if (lcdState == ALARME) {
    changerEtatLCD(AC);
  } else {
    changerEtatLCD(ECLAIRAGE);
  }
}

void doubleClicBouton() {
  if (vanneBouge() || irrigation.getCurrentState() == ARRET) {
    return;
  }

  lumiereMin = 1023;
  lumiereMax = 0;
  afficherTexteFixe("Calibration", "Min/Max reset");
}

void gestionnaireEtatLCD(unsigned long ct) {
  if (vanneBouge() || irrigation.getCurrentState() == ARRET) {
    vanneLCDState(ct);
    return;
  }

  switch (lcdState) {
    case ECLAIRAGE:
      eclairageState(ct);
      break;
    case ALARME:
      alarmeState(ct);
      break;
    case AC:
      acState(ct);
      break;
    case DATA_RECEIVED:
      dataReceivedState(ct);
      break;
  }
}

void eclairageState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  char ligne1[17];
  char ligne2[17];
  snprintf(ligne1, sizeof(ligne1), "Luminosite:%d%%", lumierePct);

  if (lumierePct < 50) {
    snprintf(ligne2, sizeof(ligne2), "Etat:Allume");
  } else {
    snprintf(ligne2, sizeof(ligne2), "Etat:Eteint");
  }

  afficherTexteFixe(ligne1, ligne2);
}

void alarmeState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  char ligne1[17];
  char ligne2[17];
  snprintf(ligne1, sizeof(ligne1), "Distance:%d cm", distanceCm);
  snprintf(ligne2, sizeof(ligne2), "Vanne:%d%%", irrigation.getPositionPct());
  afficherTexteFixe(ligne1, ligne2);
}

void acState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  char szTemp[8];
  char szHum[8];
  char ligne1[17];
  char ligne2[17];
  dtostrf(temperature, 4, 1, szTemp);
  dtostrf(humidite, 4, 1, szHum);
  snprintf(ligne1, sizeof(ligne1), "Temp:%s C", szTemp);
  snprintf(ligne2, sizeof(ligne2), "Hum:%s %%", szHum);
  afficherTexteFixe(ligne1, ligne2);
}

void dataReceivedState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 500;
  const unsigned long nextStateRate = 3000;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  afficherTexteFixe("Donnees recues", "Convoyeur MAJ");

  if (ct - lcdStateChangedAt >= nextStateRate) {
    changerEtatLCD(ECLAIRAGE);
  }
}

void vanneLCDState(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  char ligne1[17];
  char ligne2[17];
  snprintf(ligne1, sizeof(ligne1), "Vanne:%d%%", irrigation.getPositionPct());

  switch (irrigation.getCurrentState()) {
    case OUVERTURE:
      snprintf(ligne2, sizeof(ligne2), "Ouverture");
      break;
    case OUVERT:
      snprintf(ligne2, sizeof(ligne2), "Ouverte");
      break;
    case FERMETURE:
      snprintf(ligne2, sizeof(ligne2), "Fermeture");
      break;
    case FERME:
      snprintf(ligne2, sizeof(ligne2), "Fermee");
      break;
    case ARRET:
      snprintf(ligne2, sizeof(ligne2), "Arret urgence");
      break;
  }

  afficherTexteFixe(ligne1, ligne2);
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
    lumierePct = map(lumiereBrute, lumiereMin, lumiereMax, minmap, maxmap);
  } else {
    lumierePct = map(lumiereBrute, 0, 1023, minmap, maxmap);
  }

  if (lumierePct < 0) {
    lumierePct = 0;
  }

  if (lumierePct > 100) {
    lumierePct = 100;
  }
}

void distanceTache(unsigned long ct) {
  static unsigned long previousTime = 0;
  const unsigned long rate = 250;

  if (ct - previousTime < rate) {
    return;
  }

  previousTime = ct;
  distanceCm = (int)hc.dist();
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
  Serial.print(distanceCm);
  Serial.print(",T:");
  Serial.print(temperature, 1);
  Serial.print(",H:");
  Serial.print(humidite, 1);
  Serial.print(",Van:");
  Serial.print(irrigation.getPositionPct());
  Serial.print(",Conv:");
  Serial.println(convoyeur.getVitesse());
}

void changerEtatLCD(LCDState nouvelEtat) {
  lcdState = nouvelEtat;
  lcdStateChangedAt = millis();
}

void afficherTexteFixe(const char *ligne1, const char *ligne2) {
  snprintf(lcdBuff[0], sizeof(lcdBuff[0]), "%-16.16s", ligne1);
  snprintf(lcdBuff[1], sizeof(lcdBuff[1]), "%-16.16s", ligne2);
  afficherLCD();
}

void afficherLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdBuff[0]);
  lcd.setCursor(0, 1);
  lcd.print(lcdBuff[1]);
}

bool vanneBouge() {
  return irrigation.isMoving();
}
