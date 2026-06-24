// Lane Dodger
// UNO R3 + LCD1602 (parallel) + analog joystick
//
// Player dodges scrolling obstacles by switching between the top and
// bottom row of a 16x2 character LCD. See display.h for the screen
// layout (score region vs. play area) and game_state.h for the FSM.

#include "input.h"
#include "obstacles.h"
#include "game_state.h"
#include "display.h"

// ---- Pin assignments ----
// LCD: RS, E, D4, D5, D6, D7
const uint8_t LCD_RS = 7;
const uint8_t LCD_EN = 6;
const uint8_t LCD_D4 = 5;
const uint8_t LCD_D5 = 4;
const uint8_t LCD_D6 = 3;
const uint8_t LCD_D7 = 2;

// Joystick
const uint8_t JOY_Y_PIN = A0;
const uint8_t JOY_BUTTON_PIN = 8; // moved off D2 to avoid clashing with LCD_D7

// ---- Module instances ----
InputManager input(JOY_Y_PIN, JOY_BUTTON_PIN);
ObstacleManager obstacles;
GameState game;
Display display(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Track the previous state so we know when we've just transitioned
// into MENU or GAME_OVER and need a one-time full-screen redraw,
// rather than redrawing those static screens every loop iteration.
GameStateEnum lastState;

void setup() {
  randomSeed(analogRead(A1)); // unconnected-ish analog pin for entropy
  input.begin();
  display.begin();

  lastState = STATE_MENU;
  display.renderMenu();
}

void loop() {
  input.update();
  game.update(input, obstacles);

  GameStateEnum current = game.getState();

  if (current != lastState) {
    // One-time transition redraws.
    if (current == STATE_MENU) {
      display.renderMenu();
    } else if (current == STATE_GAME_OVER) {
      display.renderGameOver(game.getScore());
    }
    lastState = current;
  }

  if (current == STATE_PLAYING) {
    display.renderPlaying(game, obstacles);
  }

  // Small delay keeps analogRead/loop from spinning faster than the
  // joystick and human reaction time actually need, without adding
  // perceptible input lag.
  delay(10);
}
