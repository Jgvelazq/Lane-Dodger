#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "game_state.h"
#include "obstacles.h"

// Screen layout (16x2 character LCD):
//
//   Row 0: [S][C][O][R][E][ ......... play area ......... ]
//   Row 1: [ ][ ][ ][ ][ ][ ......... play area ......... ]
//            ^-- cols 0-4, score region (see obstacles.h for the
//                PLAY_AREA_START_COL constant that marks where the
//                play area begins)
//
// Score only ever touches columns 0-4 on row 0. The play area
// (player + obstacles) only ever touches columns 5-15 on both rows.
// Keeping those two regions in separate render functions means a
// score update never has to redraw -- or risk corrupting -- anything
// in the play area, and vice versa.
class Display {
  public:
    // Pin order matches the LiquidCrystal library convention:
    // RS, E, D4, D5, D6, D7
    Display(uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);

    void begin();

    // Full redraw -- used when entering a new game state, since menu/
    // game-over screens replace the entire screen content rather than
    // sharing the score+play-area layout. Also resets the score/play-area
    // change-detection caches so the next renderPlaying() call always
    // does a full draw rather than comparing against stale content.
    void renderMenu();
    void renderGameOver(unsigned int finalScore);

    // Per-tick redraw while playing. Only rewrites the score region
    // when the score actually changed, and only rewrites individual
    // play-area cells that actually changed since the last frame.
    void renderPlaying(const GameState& state, const ObstacleManager& obstacles);

  private:
    LiquidCrystal _lcd;
    int _lastRenderedScore; // -1 forces a redraw on first call

    // Caches of what's currently on screen in the play area. We only
    // write to the LCD when the new frame actually differs from this,
    // instead of rewriting both rows on every loop() iteration (which
    // was happening up to ~40x per single obstacle movement and caused
    // visible smearing -- the LCD doesn't write fast enough to keep up
    // with redraws that frequent, so writes were landing mid-update).
    static const uint8_t PLAY_AREA_WIDTH = 11; // PLAY_AREA_END_COL - PLAY_AREA_START_COL + 1
    char _lastTopRow[PLAY_AREA_WIDTH + 1];
    char _lastBottomRow[PLAY_AREA_WIDTH + 1];
    bool _playAreaInitialized;

    // Defense in depth: even with change-detection above, this puts a
    // hard floor on how often the play area is allowed to repaint,
    // independent of how often the caller asks. Protects against the
    // LCD being asked to start a new write before a prior one's worth
    // of real hardware latency has elapsed, which is what caused
    // smearing as the game's tick rate increased. Tracked separately
    // per row -- a shared timestamp meant the top row's write could
    // immediately close the gate for the bottom row's write in the
    // same frame, delaying it to the next call even when it had a
    // real, independent change to paint.
    unsigned long _lastTopWriteMs;
    unsigned long _lastBottomWriteMs;
    static const unsigned long MIN_RENDER_GAP_MS = 60;

    void renderScore(unsigned int score);
    void renderPlayArea(Lane playerLane, const ObstacleManager& obstacles);

    // Compares newRow against cachedRow cell by cell. If the row has
    // any real change (or this is the forced post-transition redraw),
    // writes every changed cell in one pass -- never partially, so a
    // tick that changes multiple cells in the same row (an obstacle
    // vacating one column while occupying another) can't have some
    // of its cells written and others silently skipped, which is
    // what previously left stale '#' characters stuck on screen.
    void writeChangedCells(const char* newRow, char* cachedRow, uint8_t rowIndex,
                            unsigned long& lastWriteMs);
};

#endif
