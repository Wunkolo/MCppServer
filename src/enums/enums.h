#ifndef ENUMS_H
#define ENUMS_H

enum EntityAnimation {
  SWING_MAIN_ARM,
  LEAVE_BED,
  SWING_OFFHAND,
  CRITICAL_EFFECT,
  MAGIC_CRITICAL_EFFECT
};

enum PlayerAction {
  STARTED_DIGGING,
  CANCELLED_DIGGING,
  FINISHED_DIGGING,
  DROP_ITEM_STACK,
  DROP_ITEM,
  SHOOT_ARROW_OR_FINISH_EATING,
  SWAP_HELD_ITEMS
};

enum Face : uint8_t {
  BOTTOM,
  TOP,
  NORTH,
  SOUTH,
  WEST,
  EAST
};

enum LogType {
  LOG_INFO = 0,
  LOG_WARNING = 1,
  LOG_ERROR = 2,
  LOG_DEBUG = 3,
  LOG_RAW = 4
};

enum Gamemode {
  SURVIVAL = 0,
  CREATIVE = 1,
  ADVENTURE = 2,
  SPECTATOR = 3
};

enum ResourcePackResult {
  SUCCESSFULLY_DOWNLOADED = 0,
  DECLINED = 1,
  FAILED_DOWNLOAD = 2,
  ACCEPTED = 3,
  DOWNLOADED = 4,
  INVALID_URL = 5,
  FAILED_TO_RELOAD = 6,
  DISCARDED = 7
};

enum BossbarColor {
  PINK,
  BLUE,
  RED,
  GREEN,
  YELLOW,
  PURPLE,
  WHITE
};

enum BossbarDivision {
  NO_DIVISION,
  SIX_NOTCHES,
  TEN_NOTCHES,
  TWELVE_NOTCHES,
  TWENTY_NOTCHES
};


#endif //ENUMS_H
