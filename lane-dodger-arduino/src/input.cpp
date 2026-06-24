#include "input.h"

InputManager::InputManager(uint8_t yPin, uint8_t buttonPin)
  : _yPin(yPin), _buttonPin(buttonPin),
    _currentLane(LANE_BOTTOM), _lastLaneChangeMs(0),
    _buttonPrevState(false), _buttonPressedEdge(false) {
}

void InputManager::begin() {
  pinMode(_buttonPin, INPUT_PULLUP); // button reads LOW when pressed
}

void InputManager::update() {
  // --- Lane detection from joystick Y axis ---
  int yVal = analogRead(_yPin);
  unsigned long now = millis();

  bool wantTop = yVal > (CENTER + DEAD_ZONE);
  bool wantBottom = yVal < (CENTER - DEAD_ZONE);

  // Only allow a lane change if enough time has passed since the
  // last one. This prevents a single physical nudge from registering
  // as several rapid switches, and gives the player a consistent
  // "one move per push" feel rather than a jittery one.
  if (now - _lastLaneChangeMs >= LANE_DEBOUNCE_MS) {
    if (wantTop && _currentLane != LANE_TOP) {
      _currentLane = LANE_TOP;
      _lastLaneChangeMs = now;
    } else if (wantBottom && _currentLane != LANE_BOTTOM) {
      _currentLane = LANE_BOTTOM;
      _lastLaneChangeMs = now;
    }
  }

  // --- Button edge detection ---
  bool rawPressed = (digitalRead(_buttonPin) == LOW); // active low w/ pullup
  _buttonPressedEdge = rawPressed && !_buttonPrevState;
  _buttonPrevState = rawPressed;
}

Lane InputManager::getLane() const {
  return _currentLane;
}

bool InputManager::buttonPressed() const {
  return _buttonPressedEdge;
}
