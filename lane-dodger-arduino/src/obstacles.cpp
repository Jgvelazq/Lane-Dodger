#include "obstacles.h"

ObstacleManager::ObstacleManager() {
  reset();
}

void ObstacleManager::reset() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; i++) {
    _obstacles[i].active = false;
    _obstacles[i].col = -1;
  }
}

void ObstacleManager::update() {
  // Scroll everything left by one column.
  for (uint8_t i = 0; i < MAX_OBSTACLES; i++) {
    if (!_obstacles[i].active) continue;

    _obstacles[i].col--;

    if (_obstacles[i].col < PLAY_AREA_START_COL) {
      // Scrolled past the left edge -- deactivate so the slot can
      // be reused by a future spawn. No shifting/compacting the
      // array; we just mark slots free, which keeps this O(MAX_OBSTACLES)
      // instead of needing memmove-style logic.
      _obstacles[i].active = false;
      _obstacles[i].col = -1;
    }
  }

  trySpawn();
}

void ObstacleManager::trySpawn() {
  // Don't spawn if the rightmost column is already occupied --
  // avoids overlapping obstacles in the same lane stacking up unfairly.
  for (uint8_t i = 0; i < MAX_OBSTACLES; i++) {
    if (_obstacles[i].active && _obstacles[i].col == PLAY_AREA_END_COL) {
      return;
    }
  }

  // Find a free slot.
  int8_t freeSlot = -1;
  for (uint8_t i = 0; i < MAX_OBSTACLES; i++) {
    if (!_obstacles[i].active) {
      freeSlot = i;
      break;
    }
  }
  if (freeSlot == -1) return; // all slots busy, skip this tick

  // Spawn chance per tick -- tune this for difficulty. Kept here
  // rather than in game_state so all "where do obstacles come from"
  // logic lives in one place.
  if (random(0, 100) < 35) {
    _obstacles[freeSlot].active = true;
    _obstacles[freeSlot].col = PLAY_AREA_END_COL;
    _obstacles[freeSlot].lane = (random(0, 2) == 0) ? LANE_TOP : LANE_BOTTOM;
  }
}

bool ObstacleManager::checkCollision(Lane playerLane) const {
  for (uint8_t i = 0; i < MAX_OBSTACLES; i++) {
    if (_obstacles[i].active &&
        _obstacles[i].col == PLAYER_COL &&
        _obstacles[i].lane == playerLane) {
      return true;
    }
  }
  return false;
}

const Obstacle* ObstacleManager::getObstacles() const {
  return _obstacles;
}

uint8_t ObstacleManager::getCount() const {
  return MAX_OBSTACLES;
}
