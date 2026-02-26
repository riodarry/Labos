const int LedPin = LED_BUILTIN;   
const int NbClignotements = 1;
const int TempsCligno = 250;
const int TempsTotalVariation = 2048;
const int TempsParPas = 2048 / 256;

int LuminositeMax = 255;
int DebutFor = 0;
int tempsEteintAllume = 1000;

void setup() {
  Serial.begin(9600);
  pinMode(LedPin, OUTPUT);
}

void Clignotement() {
  Serial.println("Clignotement - 2409626");

  for (int i = DebutFor; i < NbClignotements; i++) {
    digitalWrite(LedPin, HIGH);
    delay(TempsCligno);
    digitalWrite(LedPin, LOW);
    delay(TempsCligno);
  }
}

void Variation() {
  Serial.println("Variation - 2409626");  
  for (int i = 0; i <= LuminositeMax; i++) {
    analogWrite(LedPin, i);
    delay(TempsParPas);
  }
}

void etatAllumeEtEteint() {
  
  Serial.println("Allume - 2409626");

  digitalWrite(LedPin, LOW);     
  delay(TempsCligno);

  digitalWrite(LedPin, HIGH);    
  delay(tempsEteintAllume);

  digitalWrite(LedPin, LOW);      
  delay(tempsEteintAllume);
}

void loop() {
  Clignotement();
  Variation();
  etatAllumeEtEteint();
}
