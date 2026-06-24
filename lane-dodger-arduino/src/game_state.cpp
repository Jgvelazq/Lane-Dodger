#include "game_state.h"

GameState::GameState() {
  reset();
}

void GameState::reset() {
  _state = STATE_MENU;
  _playerLane = LANE_BOTTOM;
  _score = 0;
  _lastTickMs = 0;
  _lastScoreMs = 0;
}

unsigned long GameState::getTickIntervalMs() const {
  // Ramps from BASE_TICK_MS down to MIN_TICK_MS as score increases.
  // Decrement is intentionally small (2ms per point) so the ramp
  // takes roughly 50 seconds of play to reach max speed, rather than
  // the ~10 seconds an earlier, steeper version used -- that felt
  // like an instant jump to hardest difficulty rather than a real
  // ramp. MIN_TICK_MS stays at the same floor established earlier:
  // it needs to stay comfortably above the LCD's real write latency,
  // since asking for the next obstacle movement before the previous
  // frame finished painting is what caused blurring/multiple-places-
  // at-once artifacts at high speed.
  long interval = (long)BASE_TICK_MS - (long)(_score * 2);
  if (interval < (long)MIN_TICK_MS) interval = MIN_TICK_MS;
  return (unsigned long)interval;
}

void GameState::update(InputManager& input, ObstacleManager& obstacles) {
  unsigned long now = millis();

  switch (_state) {

    case STATE_MENU:
      // Sit here doing nothing to the world until the player presses
      // the button, then start a fresh run.
      if (input.buttonPressed()) {
        obstacles.reset();
        _score = 0;
        _playerLane = LANE_BOTTOM;
        _lastTickMs = now;
        _lastScoreMs = now;
        _state = STATE_PLAYING;
      }
      break;

    case STATE_PLAYING: {
      // Player lane follows joystick every iteration -- this is cheap
      // and doesn't need to be tick-rate limited, so it stays
      // responsive even while obstacles move on the slower tick clock.
      _playerLane = input.getLane();

      // World update (obstacle scroll/spawn) runs on its own clock,
      // separate from the main loop's natural speed, so difficulty
      // scaling is just "shrink this interval" rather than touching
      // unrelated code.
      if (now - _lastTickMs >= getTickIntervalMs()) {
        obstacles.update();
        _lastTickMs = now;

        if (obstacles.checkCollision(_playerLane)) {
          _state = STATE_GAME_OVER;
          break;
        }
      }

      // Score climbs on a wall-clock timer, independent of tick speed,
      // so faster difficulty doesn't accidentally also mean faster
      // score inflation.
      if (now - _lastScoreMs >= SCORE_INTERVAL_MS) {
        _score++;
        _lastScoreMs = now;
      }
      break;
    }

    case STATE_GAME_OVER:
      if (input.buttonPressed()) {
        _state = STATE_MENU;
      }
      break;
  }
}

GameStateEnum GameState::getState() const {
  return _state;
}

Lane GameState::getPlayerLane() const {
  return _playerLane;
}

unsigned int GameState::getScore() const {
  return _score;
}
