#include "display.h"
#include <string.h>

Display::Display(uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
  : _lcd(rs, en, d4, d5, d6, d7), _lastRenderedScore(-1), _playAreaInitialized(false),
    _lastTopWriteMs(0), _lastBottomWriteMs(0) {
  _lastTopRow[0] = '\0';
  _lastBottomRow[0] = '\0';
}

void Display::begin() {
  _lcd.begin(16, 2);
}

void Display::renderMenu() {
  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print("  LANE DODGER");
  _lcd.setCursor(0, 1);
  _lcd.print(" Press to start");
  _lastRenderedScore = -1; // force score region to redraw next time we play
  _playAreaInitialized = false; // force play area to redraw next time we play
}

void Display::renderGameOver(unsigned int finalScore) {
  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print("  GAME OVER");
  _lcd.setCursor(0, 1);
  _lcd.print("Score: ");
  _lcd.print(finalScore);
}

void Display::renderScore(unsigned int score) {
  // Score region is columns 0-4 on row 0 (5 columns total). "S:" + up
  // to 3 digits fits exactly in those 5 columns and covers scores up
  // to 999 -- previously this used "SC:" + 2 digits, which silently
  // truncated any score of 100+ since the buffer had no room for a
  // 3rd digit. At the current fixed tick rate, reaching 999 would
  // take roughly 8+ minutes of continuous play, so this is a safe
  // ceiling for a run-length game like this.
  if (score > 999) score = 999; // clamp defensively; format can't exceed 5 cols either way
  _lcd.setCursor(0, 0);
  char buf[6]; // "S:" + 3 digits + null
  snprintf(buf, sizeof(buf), "S:%3u", score);
  _lcd.print(buf);
}

void Display::renderPlayArea(Lane playerLane, const ObstacleManager& obstacles) {
  // Build both play-area rows as fixed-width char buffers first,
  // then compare against what's already on screen before writing
  // anything. This is the fix for the smearing issue: loop() calls
  // this every ~10ms, but obstacles only actually move once per
  // tick interval (hundreds of ms) -- without this check we were
  // rewriting the full row up to ~40x per real movement, and the
  // LCD can't physically keep up with writes that frequent, so the
  // display was catching itself mid-write and smearing characters.
  const uint8_t width = PLAY_AREA_WIDTH;
  char topRow[width + 1];
  char bottomRow[width + 1];

  for (uint8_t i = 0; i < width; i++) {
    topRow[i] = ' ';
    bottomRow[i] = ' ';
  }
  topRow[width] = '\0';
  bottomRow[width] = '\0';

  // Place obstacles.
  const Obstacle* obs = obstacles.getObstacles();
  for (uint8_t i = 0; i < obstacles.getCount(); i++) {
    if (!obs[i].active) continue;
    uint8_t idx = obs[i].col - PLAY_AREA_START_COL;
    if (idx >= width) continue; // defensive bounds check
    if (obs[i].lane == LANE_TOP) {
      topRow[idx] = '#';
    } else {
      bottomRow[idx] = '#';
    }
  }

  // Place player on top, so it's always visible even if an obstacle
  // is logically in the same cell the instant before collision is
  // detected.
  uint8_t playerIdx = PLAYER_COL - PLAY_AREA_START_COL;
  if (playerLane == LANE_TOP) {
    topRow[playerIdx] = '>';
  } else {
    bottomRow[playerIdx] = '>';
  }

  // Diff and write each row independently. Each row is gated and
  // cached separately so neither row's timing or content can block
  // or corrupt the other's update in the same frame.
  writeChangedCells(topRow, _lastTopRow, 0, _lastTopWriteMs);
  writeChangedCells(bottomRow, _lastBottomRow, 1, _lastBottomWriteMs);
  _playAreaInitialized = true;
}

void Display::writeChangedCells(const char* newRow, char* cachedRow, uint8_t rowIndex,
                                 unsigned long& lastWriteMs) {
  unsigned long now = millis();
  bool forceWrite = !_playAreaInitialized;

  // The rate gate is decided ONCE per call, not re-checked per cell.
  // Re-checking it inside the loop was the actual bug: a single tick
  // routinely changes 2+ cells in the same row (an obstacle vacates
  // its old column AND occupies its new one), and after the first
  // cell's write, the timestamp updated to "now" -- so the gap check
  // for the very next cell in the SAME call immediately read as "too
  // soon" and the loop broke early, abandoning that cell's write
  // entirely. The cache was never updated for it either, so every
  // future frame kept comparing against a stale cached value and
  // never noticed the discrepancy -- the obstacle's old position
  // stayed lit on the LCD forever, even after the obstacle itself
  // had moved on or deactivated. Gating once per call (not per cell)
  // means either the whole row's real changes go through together,
  // or none do -- no partial, inconsistent writes.
  bool gateOpen = forceWrite || (now - lastWriteMs) >= MIN_RENDER_GAP_MS;
  if (!gateOpen) return;

  for (uint8_t i = 0; i < PLAY_AREA_WIDTH; i++) {
    bool cellChanged = forceWrite || (newRow[i] != cachedRow[i]);
    if (!cellChanged) continue;

    _lcd.setCursor(PLAY_AREA_START_COL + i, rowIndex);
    _lcd.print(newRow[i]);
    cachedRow[i] = newRow[i];
  }
  lastWriteMs = now;
}

void Display::renderPlaying(const GameState& state, const ObstacleManager& obstacles) {
  // Row 1, columns 0-4 sit under the score region but are never
  // written by renderScore() (which only touches row 0) or by the
  // play area (which starts at column 5). Nothing previously claimed
  // responsibility for that strip, so leftover text written there by
  // renderMenu()/renderGameOver() (e.g. "Press" from " Press to
  // start") stayed on screen indefinitely once gameplay began. Clear
  // it explicitly, exactly once per game start -- gated on the same
  // _playAreaInitialized flag the play area uses for its own one-time
  // post-transition clear, so this doesn't add an LCD write on every
  // ordinary frame.
  if (!_playAreaInitialized) {
    _lcd.setCursor(0, 1);
    _lcd.print("     "); // 5 spaces -- matches the score region width
  }

  unsigned int score = state.getScore();
  if ((int)score != _lastRenderedScore) {
    renderScore(score);
    _lastRenderedScore = (int)score;
  }
  renderPlayArea(state.getPlayerLane(), obstacles);
}
