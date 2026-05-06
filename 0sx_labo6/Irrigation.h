#pragma once

#include <Arduino.h>
#include <AccelStepper.h>

typedef enum { FERME, OUVERTURE, OUVERT, FERMETURE, ARRET } IrrigationState;

class Irrigation {
public:
  Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4);

  void begin();

  int getPosition() { return _position; }
  int getPositionPct() { return constrain(map(_position, _closedPos, _openedPos, 0, 100), 0, 100); }
  void setClosedOpenedPos(int closed, int opened) { _closedPos = closed; _openedPos = opened; }
  int setDistance(int &dist) {
    _distance = &dist;
    return *_distance;
  }
  void setBtnClickFlag(bool &clickFlag) { _btnClickFlag = &clickFlag; }
  int isMoving() { return _state == OUVERTURE || _state == FERMETURE; }
  int getCurrentState() { return _state; }

  void update();

private:
  enum {
    MOTOR_INTERFACE_TYPE = 4,
    NIVEAU_MIN_CM = 20,
    NIVEAU_MAX_CM = 25
  };

  int _ledPin;
  int _closedPos = 0;
  int _openedPos = 2038;
  int _position = 0;
  int *_distance = nullptr;
  bool *_btnClickFlag = nullptr;
  bool _ledAllumee = false;
  unsigned long _lastLedUpdate = 0;
  IrrigationState _state = FERMETURE;

  AccelStepper _moteur;

  void changeState(IrrigationState newState);
  void handleButton();
  void updatePosition();
  void updateLed();

  void closedState();
  void openingState();
  void openedState();
  void closingState();
  void stoppedState();
};
