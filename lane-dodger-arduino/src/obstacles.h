#ifndef OBSTACLES_H
#define OBSTACLES_H

#include <Arduino.h>
#include "input.h" // for Lane enum

// Play area column range (see display.h for the full layout rationale).
// Columns 0-4 on row 0 are reserved for the score readout; obstacles
// and the player only ever occupy columns 5-15.
static const uint8_t PLAY_AREA_START_COL = 5;
static const uint8_t PLAY_AREA_END_COL = 15; // inclusive
static const uint8_t PLAYER_COL = PLAY_AREA_START_COL + 1; // fixed player position

static const uint8_t MAX_OBSTACLES = 4; // generous for a 11-wide play area

struct Obstacle {
  int8_t col;     // current column; -1 means "inactive / not in play"
  Lane lane;      // which row it's on
  bool active;
};

// Owns the obstacle array: spawning, scrolling, and collision queries.
// Deliberately has no idea how to draw anything (that's display.h's job)
// and no idea what the score is (that's game_state's job) -- it just
// tracks where obstacles are in the world.
class ObstacleManager {
  public:
    ObstacleManager();

    void reset();

    // Advances all active obstacles one column to the left, deactivates
    // anything that's scrolled off the play area, and may spawn a new
    // obstacle at the right edge.
    void update();

    // True if any active obstacle currently shares the player's column
    // and lane -- i.e. a collision happened this tick.
    bool checkCollision(Lane playerLane) const;

    const Obstacle* getObstacles() const;
    uint8_t getCount() const;

  private:
    Obstacle _obstacles[MAX_OBSTACLES];

    void trySpawn();
};

#endif
