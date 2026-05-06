#include "Irrigation.h"

Irrigation::Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4)
  : _ledPin(ledPin),
    _moteur(MOTOR_INTERFACE_TYPE, pin1, pin3, pin2, pin4) {
}

void Irrigation::begin() {
  pinMode(_ledPin, OUTPUT);

  _moteur.setMaxSpeed(500);
  _moteur.setAcceleration(100);
  _moteur.setSpeed(200);
  _moteur.setCurrentPosition(_openedPos);
  _moteur.moveTo(_closedPos);

  updatePosition();
}

void Irrigation::update() {
  handleButton();
  _moteur.run();
  updatePosition();

  switch (_state) {
    case FERME:
      closedState();
      break;
    case OUVERTURE:
      openingState();
      break;
    case OUVERT:
      openedState();
      break;
    case FERMETURE:
      closingState();
      break;
    case ARRET:
      stoppedState();
      break;
  }

  updateLed();
}

void Irrigation::changeState(IrrigationState newState) {
  if (_state == newState) {
    return;
  }

  _state = newState;

  switch (_state) {
    case OUVERTURE:
      Serial.println("Entree Etat : Ouverture");
      _moteur.moveTo(_openedPos);
      break;
    case FERMETURE:
      Serial.println("Entree Etat : Fermeture");
      _moteur.moveTo(_closedPos);
      break;
    case ARRET:
      Serial.println("Entree Etat : Arret");
      _moteur.moveTo(_moteur.currentPosition());
      break;
    case FERME:
      Serial.println("Entree Etat : Ferme");
      break;
    case OUVERT:
      Serial.println("Entree Etat : Ouvert");
      break;
  }
}

void Irrigation::handleButton() {
  if (_btnClickFlag == nullptr || !*_btnClickFlag) {
    return;
  }

  *_btnClickFlag = false;

  if (isMoving()) {
    changeState(ARRET);
    return;
  }

  if (_state == ARRET) {
    changeState(OUVERTURE);
  }
}

void Irrigation::updatePosition() {
  _position = constrain((int)_moteur.currentPosition(), _closedPos, _openedPos);
}

void Irrigation::updateLed() {
  const unsigned long rate = 100;
  unsigned long currentTime = millis();

  if (!isMoving()) {
    digitalWrite(_ledPin, LOW);
    _ledAllumee = false;
    return;
  }

  if (currentTime - _lastLedUpdate < rate) {
    return;
  }

  _lastLedUpdate = currentTime;
  _ledAllumee = !_ledAllumee;
  digitalWrite(_ledPin, _ledAllumee);
}

void Irrigation::closedState() {
  if (_distance != nullptr && *_distance > 0 && *_distance < NIVEAU_MIN_CM) {
    changeState(OUVERTURE);
  }
}

void Irrigation::openingState() {
  if (_distance != nullptr && *_distance >= NIVEAU_MAX_CM) {
    changeState(OUVERT);
    return;
  }

  if (_moteur.distanceToGo() == 0) {
    changeState(OUVERT);
  }
}

void Irrigation::openedState() {
  if (_distance != nullptr && *_distance >= NIVEAU_MAX_CM) {
    changeState(FERMETURE);
  }
}

void Irrigation::closingState() {
  if (_moteur.distanceToGo() == 0) {
    changeState(FERME);
  }
}

void Irrigation::stoppedState() {
}
