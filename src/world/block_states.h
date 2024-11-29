#ifndef BLOCK_STATES_H
#define BLOCK_STATES_H
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

struct BlockData;
enum Face : uint8_t;
struct Position;
struct Player;
struct Block;

enum class StateType {
    Enum,
    Int,
    Bool
};

enum class Axis {
    X,
    Y,
    Z
};

enum class Facing {
    North,
    South,
    East,
    West,
    Up,
    Down
};

enum class RedstoneState {
    Up,
    Side,
    None
};

enum class Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST,
    UP,
    DOWN
};


struct EnumState {
    std::string name;
    StateType type;
    std::vector<std::string> values;
    std::string currentValue;
};


struct IntState {
    std::string name;
    StateType type;
    int minValue;
    int maxValue;
    int currentValue;
};


struct BoolState {
    std::string name;
    StateType type;
    bool currentValue;
};

// Variant to hold any type of state
using BlockState = std::variant<EnumState, IntState, BoolState>;

size_t calculateBlockStateID(const BlockData& blockData, std::vector<BlockState>& currentBlockState);
std::vector<BlockState> getBlockStates(const nlohmann::basic_json<> & states);
void assignCurrenBlockStates(const BlockData& blockData, const std::shared_ptr<Player>& player, const Position& pos, Face faceClicked, const Position &cursorPos, std::vector<BlockState>& currentBlockState);

#endif //BLOCK_STATES_H
