#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>

// Which lane the player currently occupies on the display.
enum Lane {
  LANE_TOP = 0,
  LANE_BOTTOM = 1
};

// Wraps joystick reading + debouncing so the rest of the program
// never touches analogRead() directly. game_state/obstacles only
// see a clean Lane value and a button press flag.
class InputManager {
  public:
    InputManager(uint8_t yPin, uint8_t buttonPin);

    void begin();

    // Call once per loop() iteration. Updates internal state.
    void update();

    // Current debounced lane based on joystick Y position.
    Lane getLane() const;

    // True only on the single update() call where the button
    // transitions from not-pressed to pressed (edge-triggered,
    // not level-triggered, so holding the button doesn't spam input).
    bool buttonPressed() const;

  private:
    uint8_t _yPin;
    uint8_t _buttonPin;

    Lane _currentLane;
    unsigned long _lastLaneChangeMs;

    bool _buttonPrevState;   // previous raw reading, for edge detection
    bool _buttonPressedEdge; // result computed in update(), read by buttonPressed()

    static const int CENTER = 512;
    static const int DEAD_ZONE = 150;        // ignore noise around center
    static const unsigned long LANE_DEBOUNCE_MS = 200; // min time between lane switches
};

#endif
