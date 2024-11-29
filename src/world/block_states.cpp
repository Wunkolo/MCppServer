#include "block_states.h"

#include <iostream>
#include <nlohmann/json.hpp>

#include "chunk.h"
#include "entities/entity.h"
#include "enums//enums.h"
#include "data/data.h"
#include "entities/player.h"

bool isBlockWater(const Position & pos) {
    // TODO: Implement this function
    return false;
}

int calculateRotation(double yaw) {
    // Normalize yaw to [0, 360)
    yaw = fmod(yaw, 360.0);
    if (yaw < 0) yaw += 360.0;

    // Each step is 22.5 degrees (360 / 16)
    // Add 0.5 to round to the nearest integer
    double rotation = (yaw / 22.5) + 0.5;

    // Convert to integer and wrap around using modulo 16
    int rotationValue = static_cast<int>(rotation) % 16;

    return rotationValue;
}

Direction getPlayerFacingDirection(const std::shared_ptr<Player>& player, bool considerPitch) {
    double yaw = player->rotation.yaw;
    double pitch = player->rotation.pitch;

    // Normalize yaw to [0, 360)
    while (yaw < 0) yaw += 360;
    while (yaw >= 360) yaw -= 360;

    if (considerPitch) {
        if (pitch >= 45.0) {
            return Direction::UP;
        }
        if (pitch <= -45.0) {
            return Direction::DOWN;
        }
        if (yaw >= 315.0 || yaw < 45.0) {
            return Direction::NORTH;
        }
        if (yaw >= 45.0 && yaw < 135.0) {
            return Direction::EAST;
        }
        if (yaw >= 135.0 && yaw < 225.0) {
            return Direction::SOUTH;
        }
        // yaw >= 225.0 && yaw < 315.0
        return Direction::WEST;
    }

    if (yaw >= 315.0 || yaw < 45.0) {
        return Direction::SOUTH;
    }
    if (yaw >= 45.0 && yaw < 135.0) {
        return Direction::WEST;
    }
    if (yaw >= 135.0 && yaw < 225.0) {
        return Direction::NORTH;
    }
    // yaw >= 225.0 && yaw < 315.0
    return Direction::EAST;
}

std::string directionToString(Direction dir) {
    switch(dir) {
        case Direction::UP:    return "up";
        case Direction::DOWN:  return "down";
        case Direction::NORTH: return "north";
        case Direction::SOUTH: return "south";
        case Direction::EAST:  return "east";
        case Direction::WEST:  return "west";
        default: return "north"; // Default fallback
    }
}

bool hasFaceValue(std::vector<BlockState> blockStates) {
    for (auto &state : blockStates) {
        if (auto enumState = std::get_if<EnumState>(&state)) {
            if (enumState->name == "face") {
                return true;
            }
        }
    }
    return false;
}

size_t calculateBlockStateID(const BlockData& blockData, std::vector<BlockState>& currentBlockState) {
    size_t index = 0;
    size_t multiplier = 1;

    // Iterate through the states in reverse order (least significant first)
    for(auto it = currentBlockState.rbegin(); it != currentBlockState.rend(); ++it) {
        if(auto enumState = std::get_if<EnumState>(&(*it))) {
            // Find the index of the current value
            size_t valueIndex = 0;
            for(; valueIndex < enumState->values.size(); ++valueIndex){
                if(enumState->currentValue == enumState->values[valueIndex]){
                    break;
                }
            }
            index += valueIndex * multiplier;
            multiplier *= enumState->values.size();
        }
        else if(auto intState = std::get_if<IntState>(&(*it))) {
            // Calculate the index based on the current value
            size_t valueIndex = static_cast<size_t>(intState->currentValue - intState->minValue);
            index += valueIndex * multiplier;
            multiplier *= (intState->maxValue - intState->minValue + 1);
        }
        else if(auto boolState = std::get_if<BoolState>(&(*it))) {
            size_t valueIndex = boolState->currentValue ? 0 : 1;
            index += valueIndex * multiplier;
            multiplier *= 2;
        }
    }
    // Calculate the final blockStateID
    size_t blockStateID = blockData.minStateId + index;

    // Ensure the blockStateID does not exceed maxStateId
    if(blockStateID > blockData.maxStateId) {
        throw std::out_of_range("Calculated blockStateID exceeds maxStateId.");
    }

    currentBlockState.clear();

    return blockStateID;
}

std::vector<BlockState> getBlockStates(const nlohmann::basic_json<> & states) {
    std::vector<BlockState> blockstates;
    for (const auto& state : states) {
        if (state.contains("type")) {
            std::string type = state["type"];
            if (type == "enum") {
                EnumState enumState;
                enumState.name = state["name"];
                enumState.type = StateType::Enum;
                for (const auto& value : state["values"]) {
                    enumState.values.push_back(value);
                }
                enumState.currentValue = enumState.values[0];
                blockstates.push_back(enumState);
            } else if (type == "int") {
                IntState intState;
                intState.name = state["name"];
                intState.type = StateType::Int;
                // Get first value in the array
                intState.minValue = std::stoi(static_cast<std::string>(state["values"][0]));
                int numValues = state["num_values"];
                intState.currentValue = intState.minValue;
                intState.maxValue = intState.minValue + numValues - 1;
                blockstates.push_back(intState);
            } else if (type == "bool") {
                BoolState boolState;
                boolState.name = state["name"];
                boolState.type = StateType::Bool;
                boolState.currentValue = false;
                blockstates.push_back(boolState);
            }
        }
    }

    return blockstates;
}

void assignCurrenBlockStates(const BlockData& blockData, const std::shared_ptr<Player>& player, const Position& pos, Face faceClicked, const Position &cursorPos, std::vector<BlockState>& currentBlockState) {
    if(blockData.states.empty()) {
        return;
    }
    if(!currentBlockState.empty()) {
        currentBlockState.clear();
    }
    currentBlockState = blockData.states;
    bool requiresVerticalFacing = false;
    for(auto& state : currentBlockState) {
        if(auto enumState = std::get_if<EnumState>(&state)) {
            if(enumState->name == "facing" && enumState->values.size() == 6) {
                requiresVerticalFacing = true;
            }
            if(enumState->name == "axis") {
                // Determine axis based on face clicked
                switch(faceClicked) {
                    case TOP:
                    case BOTTOM:
                        enumState->currentValue = "y";
                        break;
                    case NORTH:
                    case SOUTH:
                        enumState->currentValue = "z";
                        break;
                    case EAST:
                    case WEST:
                        enumState->currentValue = "x";
                        break;
                }
            }
            else if(enumState->name == "facing" && !hasFaceValue(currentBlockState)) {
                // Assign facing based on player's look direction
                Direction playerDirection = getPlayerFacingDirection(player, requiresVerticalFacing);
                std::string playerFacingStr = directionToString(playerDirection);
                enumState->currentValue = playerFacingStr;
            } else if(enumState->name == "facing" && hasFaceValue(currentBlockState)) {
                switch(faceClicked) {
                    case NORTH:
                        enumState->currentValue = "north";
                        break;
                    case SOUTH:
                        enumState->currentValue = "south";
                        break;
                    case EAST:
                        enumState->currentValue = "east";
                        break;
                    case WEST:
                        enumState->currentValue = "west";
                        break;
                    default:
                        enumState->currentValue = "north";
                        break;
                }
            }
            else if(enumState->name == "type") {
                // Assign type based on cursor Y position
                if(faceClicked == TOP) {
                    enumState->currentValue = "bottom";
                }
                else if(faceClicked == BOTTOM) {
                    enumState->currentValue = "top";
                }
                else if(cursorPos.y < 0.5f) {
                    enumState->currentValue = "bottom";
                }
                else if(cursorPos.y >= 0.5f && cursorPos.y < 1.0f) {
                    enumState->currentValue = "top";
                }
                // TODO: Implement logic for "double" based on existing block state
                // This would require additional context, such as querying the existing block's state
            }
            else if(enumState->name == "half" && enumState->values.at(0) == "top" && enumState->values.at(1) == "bottom") {
                // Assign type based on cursor Y position
                if(faceClicked == TOP) {
                    enumState->currentValue = "bottom";
                }
                else if(faceClicked == BOTTOM) {
                    enumState->currentValue = "top";
                }
                else if(cursorPos.y < 0.5f) {
                    enumState->currentValue = "bottom";
                }
                else if(cursorPos.y >= 0.5f && cursorPos.y < 1.0f) {
                    enumState->currentValue = "top";
                }
            }
            else if(enumState->name == "half" && enumState->values.at(0) == "upper" && enumState->values.at(1) == "lower") {
                enumState->currentValue = "lower";

                // TODO: Implement logic for "upper" based on block above
            }
            else if(enumState->name == "face") {
                switch (faceClicked) {
                    case TOP:
                        enumState->currentValue = "floor";
                        break;
                    case BOTTOM:
                        enumState->currentValue = "ceiling";
                        break;
                    case NORTH:
                    case SOUTH:
                    case WEST:
                    case EAST:
                        enumState->currentValue = "wall";
                        break;
                }
            }
            // TODO: Handle other EnumState properties
        }
        else if(auto intState = std::get_if<IntState>(&state)) {
            if(intState->name == "rotation") { // For signs
                // Calculate rotation based on player's yaw
                double yaw = player->rotation.yaw;
                int rotationValue = calculateRotation(yaw);
                intState->currentValue = rotationValue;
            }
            else if(intState->name == "power") {
                // For now: Set to maximum power
                intState->currentValue = 15;
            }
            // TODO: Handle other IntState properties
        }
        else if(auto boolState = std::get_if<BoolState>(&state)) {
            if(boolState->name == "waterlogged") {
                bool isWater = isBlockWater(pos);
                boolState->currentValue = isWater;
            }
            /// TODO: Handle other BoolState properties
        }
    }
}
