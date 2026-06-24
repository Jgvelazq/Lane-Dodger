#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>
#include "input.h"
#include "obstacles.h"

enum GameStateEnum {
  STATE_MENU,
  STATE_PLAYING,
  STATE_GAME_OVER
};

// The FSM described in the design doc:
//
//   MENU --(click)--> PLAYING
//   PLAYING --(collision)--> GAME_OVER
//   GAME_OVER --(click)--> MENU
//
// This class owns the current state, the player's lane, the score,
// and the tick-speed-based difficulty curve. It does NOT know how to
// draw anything (display.h) or how to read raw input (input.h) --
// it just consumes an InputManager + ObstacleManager and decides
// what should happen next according to the rules above.
class GameState {
  public:
    GameState();

    void reset();

    // Call once per loop() iteration with fresh input + the obstacle
    // manager for this tick. Advances the FSM and game logic.
    void update(InputManager& input, ObstacleManager& obstacles);

    GameStateEnum getState() const;
    Lane getPlayerLane() const;
    unsigned int getScore() const;

    // Current delay (ms) between obstacle-update ticks. Decreases
    // slowly as score increases, creating a difficulty curve that
    // takes roughly 50 seconds of play to reach max speed.
    unsigned long getTickIntervalMs() const;

  private:
    GameStateEnum _state;
    Lane _playerLane;
    unsigned int _score;

    unsigned long _lastTickMs;
    unsigned long _lastScoreMs;

    // Starting tick interval, before any difficulty ramp applies.
    static const unsigned long BASE_TICK_MS = 400;
    // Floor for the difficulty ramp. Must stay comfortably above the
    // LCD's real write latency -- a parallel HD44780 write isn't
    // instant, and asking for the next movement before the previous
    // frame finished painting is what caused obstacles to appear
    // blurred / in multiple places at once. 200ms keeps solid margin.
    static const unsigned long MIN_TICK_MS = 200;
    static const unsigned long SCORE_INTERVAL_MS = 500; // +1 score per this many ms survived
};

#endif
