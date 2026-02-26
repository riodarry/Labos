int leds=[8,9,10,11];
unsigned long potentiometre=A2;
int btn= 2;
int buttonState = 0;
int brightness = 0;  
int fadeAmount = 5;
int ValeurBasePot=0;

void setup() {
  // put your setup code here, to run once:
setup.begin(9600);
for(int i=0; i<=3; i++){
  pinMode(leds[]; OUTPUT);

}
pinMode(btn, INPUT_PULLUP);


}
int bouton (int btn){
 // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
  } else {
    // turn LED off:
    digitalWrite(ledPin, LOW);
  }

}

int allumerLeds(){
 for (int i=0; i<=leds[i]; i++){
  if(bouton()==true && ){
      analogWrite(led, brightness);
  brightness = brightness + fadeAmount;
  
  }
 }
}

int convertionpot_Leds(){
 ValeurBasePot= analogRead(potentiometre);
 convertion= map(valeurBasePot, 0,1023,0,255);
  return convertion;
  
}


void loop() {
  // put your main code here, to run repeatedly:

}
