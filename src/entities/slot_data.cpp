#include "slot_data.h"

#include "networking/network.h"

Component parseComponent(const std::vector<uint8_t>& data, size_t& index) {
    Component component;
    size_t typeVarInt = parseVarInt(data, index);
    component.type = static_cast<ComponentType>(typeVarInt);

    // TODO: Parse component data based on type
    // Placeholder: Read a VarInt indicating data length followed by data
    size_t dataLength = parseVarInt(data, index);
    std::vector componentData(data.begin() + index, data.begin() + index + dataLength);
    component.data = componentData;
    index += dataLength;

    return component;
}
