#ifndef COMMANDBUILDER_H
#define COMMANDBUILDER_H
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "networking/network.h"
#include "entities/player.h"

enum class NodeType {
    ROOT,
    LITERAL,
    ARGUMENT
};

struct IntRange {
    int min;
    int max;
};

struct FloatRange {
    float min;
    float max;
};

struct DoubleRange {
    double min;
    double max;
};

// Add other ranges if necessary

using ArgumentRange = std::variant<std::monostate, IntRange, FloatRange, DoubleRange>;

// Type alias for command handlers
using CommandHandler = std::function<void(const Player*, const std::vector<std::string>&, std::function<void(const std::string&, bool, const std::vector<std::string>&)>)>;

void buildAllCommands();

// Base Command Node
class CommandNode {
public:
    NodeType type;
    bool isExecutable;
    bool consoleExecutable;
    bool hasRedirect;
    bool hasSuggestions;
    int redirectIndex;
    std::string name;
    std::string identifier;
    int parserId;

    // Handler function (only for executable nodes)
    CommandHandler handler;

    std::vector<std::shared_ptr<CommandNode>> children;

    CommandNode(NodeType type, const std::string& name = "", int parserId = -1)
        : type(type), isExecutable(false), consoleExecutable(false), hasRedirect(false), hasSuggestions(false),
          redirectIndex(-1), name(name),
          parserId(parserId) {
    }

    virtual ~CommandNode() = default;

    void addChild(const std::shared_ptr<CommandNode> &child) {
        children.push_back(child);
    }

    void setHandler(const CommandHandler &h) {
        handler = h;
        isExecutable = true;
    }
};

class RootNode : public CommandNode {
public:
    RootNode() : CommandNode(NodeType::ROOT) {}
};

class LiteralNode : public CommandNode {
public:
    LiteralNode(const std::string& literal) : CommandNode(NodeType::LITERAL, literal) {}
};

class ArgumentNode : public CommandNode {
public:
    uint8_t parserProperties;
    ArgumentRange range;

    ArgumentNode(const std::string& argumentName, int parserId)
        : CommandNode(NodeType::ARGUMENT, argumentName, parserId), parserProperties(0) {
    }

    void setParserProperties(uint8_t properties) {
        parserProperties = properties;
    }

    void setHasSuggestions(bool suggestions) {
        hasSuggestions = suggestions;
    }

    void suggestionIdentifier(const std::string& id) {
        identifier = id;
    }

    void setIntegerRange(int min, int max) {
        range = IntRange{min, max};
    }

    void setFloatRange(float min, float max) {
        range = FloatRange{min, max};
    }

    void setDoubleRange(double min, double max) {
        range = DoubleRange{min, max};
    }


};

class CommandBuilder {
public:
    std::shared_ptr<RootNode> root;

    std::vector<std::shared_ptr<CommandNode>> nodeStack;

    CommandBuilder() {
        root = std::make_shared<RootNode>();
        nodeStack.push_back(root);
    }

    CommandBuilder& literal(const std::string& name, bool executable = false, bool consoleExecutable = false) {
        auto literalNode = std::make_shared<LiteralNode>(name);
        literalNode->isExecutable = executable;
        literalNode->consoleExecutable = consoleExecutable;
        nodeStack.back()->addChild(literalNode);
        nodeStack.push_back(literalNode);
        return *this;
    }

    CommandBuilder& argument(const std::string& name, int parserId, bool executable = false, bool consoleExecutable = false) {
        auto argNode = std::make_shared<ArgumentNode>(name, parserId);
        argNode->isExecutable = executable;
        argNode->consoleExecutable = consoleExecutable;
        nodeStack.back()->addChild(argNode);
        nodeStack.push_back(argNode);
        return *this;
    }

    CommandBuilder& end() {
        if (nodeStack.size() > 1) { // Prevent popping the root
            nodeStack.pop_back();
        }
        return *this;
    }

    // TODO: Implement redirect
    CommandBuilder& redirect(int nodeIndex) {
        nodeStack.back()->hasRedirect = true;
        nodeStack.back()->redirectIndex = nodeIndex;
        return *this;
    }

    CommandBuilder& setEntityProperties(bool singleOnly, bool playersOnly) {
        if (nodeStack.empty()) {
            logMessage("No node to set properties on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        uint8_t properties = 0;
        if (singleOnly) properties |= 0x01;
        if (playersOnly) properties |= 0x02;
        argNode->setParserProperties(properties);

        return *this;
    }

    CommandBuilder& setIntegerRange(int min, int max) {
        if (nodeStack.empty()) {
            logMessage("No node to set range on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        argNode->setIntegerRange(min, max);

        return *this;
    }

    CommandBuilder& setFloatRange(float min, float max) {
        if (nodeStack.empty()) {
            logMessage("No node to set range on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        argNode->setFloatRange(min, max);
        return *this;
    }

    CommandBuilder& setDoubleRange(double min, double max) {
        if (nodeStack.empty()) {
            logMessage("No node to set range on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        argNode->setDoubleRange(min, max);
        return *this;
    }

    CommandBuilder& setHasSuggestions(bool suggestions) {
        if (nodeStack.empty()) {
            logMessage("No node to set suggestions on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        argNode->setHasSuggestions(suggestions);
        return *this;
    }

    CommandBuilder& suggestionIdentifier(const std::string& id) {
        if (nodeStack.empty()) {
            logMessage("No node to set suggestion identifier on", LOG_ERROR);
            return *this;
        }

        auto lastNode = nodeStack.back();
        if (lastNode->type != NodeType::ARGUMENT) {
            logMessage("Last node is not an argument", LOG_ERROR);
            return *this;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(lastNode);
        if (!argNode) {
            logMessage("Failed to cast to ArgumentNode", LOG_ERROR);
            return *this;
        }

        argNode->suggestionIdentifier(id);
        return *this;
    }

    // Associate a handler with the current node (must be executable)
    CommandBuilder& handler(const CommandHandler &h) {
        if (nodeStack.empty()) {
            logMessage("No node to set handler on", LOG_ERROR);
            return *this;
        }

        auto currentNode = nodeStack.back();
        if (!currentNode->isExecutable) {
            logMessage("Node is not executable", LOG_ERROR);
            return *this;
        }

        currentNode->setHandler(h);

        return *this;
    }

    std::shared_ptr<RootNode> build() {
        return root;
    }
};

class CommandSerializer {
public:
    static std::pair<std::vector<uint8_t>, int> serializeCommands(const std::shared_ptr<RootNode>& root, size_t& numberOfNodes) {
        std::vector<std::shared_ptr<CommandNode>> nodeList;
        std::unordered_map<CommandNode*, int> nodeIndices;

        // Flatten the graph (BFS)
        std::vector<std::shared_ptr<CommandNode>> queue = { root };
        while (!queue.empty()) {
            auto current = queue.front();
            queue.erase(queue.begin());

            if (nodeIndices.contains(current.get())) {
                continue; // Already processed
            }

            nodeIndices[current.get()] = nodeList.size();
            nodeList.push_back(current);

            for (auto& child : current->children) {
                queue.push_back(child);
            }

            if (current->hasRedirect) {
                // TODO: Handle redirect nodes
            }
        }

        std::vector<uint8_t> serializedNodes;

        // Serialize each node
        for (const auto& node : nodeList) {
            // Flags
            uint8_t flags = 0;
            switch (node->type) {
                case NodeType::ROOT:
                    flags |= 0x00;
                    break;
                case NodeType::LITERAL:
                    flags |= 0x01;
                    break;
                case NodeType::ARGUMENT:
                    flags |= 0x02;
                    break;
            }
            if (node->isExecutable) flags |= 0x04;
            if (node->hasRedirect) flags |= 0x08;
            if (node->hasSuggestions) flags |= 0x10;

            serializedNodes.push_back(flags);

            // Children count
            writeVarInt(serializedNodes, node->children.size());

            // Children indices
            for (const auto& child : node->children) {
                int index = nodeIndices[child.get()];
                writeVarInt(serializedNodes, index);
            }

            // Redirect node
            if (node->hasRedirect) {
                writeVarInt(serializedNodes, node->redirectIndex);
            }

            // Name
            if (node->type == NodeType::LITERAL || node->type == NodeType::ARGUMENT) {
                writeString(serializedNodes, node->name);
            }

            // Parser ID and Properties
            if (node->type == NodeType::ARGUMENT) {
                writeVarInt(serializedNodes, node->parserId);

                switch (node->parserId) {
                     case 1: { // brigadier:float
                    auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                    if (argNode && std::holds_alternative<FloatRange>(argNode->range)) {
                        FloatRange fr = std::get<FloatRange>(argNode->range);
                        uint8_t flags = 0;
                        if (fr.min != 0.0f) flags |= 0x01;
                        if (fr.max != 0.0f) flags |= 0x02;
                        writeByte(serializedNodes, flags);
                        if (flags & 0x01) {
                            float min = fr.min;
                            writeFloat(serializedNodes, min);
                        }
                        if (flags & 0x02) {
                            float max = fr.max;
                            writeFloat(serializedNodes, max);
                        }
                    }
                    break;
                }
                case 2: { // brigadier:double
                    auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                    if (argNode && std::holds_alternative<DoubleRange>(argNode->range)) {
                        DoubleRange dr = std::get<DoubleRange>(argNode->range);
                        uint8_t flags = 0;
                        if (dr.min != 0.0) flags |= 0x01;
                        if (dr.max != 0.0) flags |= 0x02;
                        writeByte(serializedNodes, flags);
                        if (flags & 0x01) {
                            double min = dr.min;
                            writeDouble(serializedNodes, min);
                        }
                        if (flags & 0x02) {
                            double max = dr.max;
                            writeDouble(serializedNodes, max);
                        }
                    }
                    break;
                }
                case 3: { // brigadier:integer
                    auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                    if (argNode && std::holds_alternative<IntRange>(argNode->range)) {
                        IntRange ir = std::get<IntRange>(argNode->range);
                        uint8_t flags = 0;
                        if (ir.min != 0) flags |= 0x01;
                        if (ir.max != 0) flags |= 0x02;
                        writeByte(serializedNodes, flags);
                        if (flags & 0x01) {
                            writeInt(serializedNodes, ir.min);
                        }
                        if (flags & 0x02) {
                            writeInt(serializedNodes, ir.max);
                        }
                    }
                    break;
                }
                case 6: { // minecraft:entity
                    // Write parser properties (flags)
                    auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                    if (argNode) {
                        writeByte(serializedNodes, argNode->parserProperties);
                    }
                    break;
                }
                case 42: { // minecraft:time
                    // Write parser properties (Min)
                    auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                    if (argNode) {
                        writeInt(serializedNodes, argNode->range.index() == 0 ? 0 :
                            (std::holds_alternative<IntRange>(argNode->range) ? std::get<IntRange>(argNode->range).min :
                            (std::holds_alternative<DoubleRange>(argNode->range) ? static_cast<int>(std::get<DoubleRange>(argNode->range).min) : 0)));
                    }
                    break;
                }
                // TODO: Serialize other parser properties
                default:
                    // If there are no additional properties, do nothing
                    break;
                }
            }

            // Suggestions
            if (node->hasSuggestions) {
                writeString(serializedNodes, node->identifier);
            }
        }

        // Root index
        int rootIndex = nodeIndices[root.get()];

        numberOfNodes = nodeList.size();

        return { serializedNodes, rootIndex };
    }
};


class CommandParser {
public:
    CommandParser(const std::shared_ptr<RootNode> &rootNode, const std::function<void(const std::string&, bool, const std::vector<std::string>&)> &outputFunc) : root(rootNode), sendOutput(outputFunc) {}

    bool parseAndExecute(const Player& player, const std::string& inputCommand) const {
        // Tokenize the input command
        std::vector<std::string> tokens = tokenize(inputCommand);
        if (tokens.empty()) {
            // Empty command
            return false;
        }

        // Start traversal from root
        std::shared_ptr<CommandNode> currentNode = root;
        std::vector<std::string> args;
        size_t tokenIndex = 0;

        while (tokenIndex < tokens.size()) {
            const std::string& token = tokens[tokenIndex];

            // Try to find a matching child (literal or argument)
            bool matched = false;
            for (const auto& child : currentNode->children) {
                if (child->type == NodeType::LITERAL && child->name == token) {
                    // Literal match
                    currentNode = child;
                    matched = true;
                    break;
                }
                if (child->type == NodeType::ARGUMENT) {
                    // Argument node, accept any token
                    // Validate based on parserId and properties
                    if (validateArgument(child, tokens, tokenIndex, args, &player)) {
                        currentNode = child;
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched) {
                // No matching child found
                sendOutput("Unknown command or invalid arguments.", true, {});
                return false;
            }

            tokenIndex++;
        }

        // After processing all tokens, check if current node is executable
        if (currentNode->isExecutable && currentNode->handler) {
            // Execute the handler with collected arguments
            currentNode->handler(&player, args, sendOutput);
            return true;
        }
        else {
            // Command incomplete or not executable
            sendOutput("Incomplete command.", true, {});
            return false;
        }
    }

    bool parseAndExecuteConsole(const std::string& inputCommand, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &output) const {
        // Tokenize the input command
        std::vector<std::string> tokens = tokenize(inputCommand);
        if (tokens.empty()) {
            // Empty command
            return false;
        }

        // Start traversal from root
        std::shared_ptr<CommandNode> currentNode = root;
        std::vector<std::string> args;
        size_t tokenIndex = 0;

        while (tokenIndex < tokens.size()) {
            const std::string& token = tokens[tokenIndex];

            // Try to find a matching child (literal or argument)
            bool matched = false;
            for (const auto& child : currentNode->children) {
                if (child->type == NodeType::LITERAL && child->name == token) {
                    // Literal match
                    currentNode = child;
                    matched = true;
                    break;
                }
                if (child->type == NodeType::ARGUMENT) {
                    // Argument node, accept any token
                    if (validateArgument(child, tokens, tokenIndex, args, nullptr)) {
                        currentNode = child;
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched) {
                // No matching child found
                output("Unknown command or invalid arguments.", true, {});
                return false;
            }

            tokenIndex++;
        }

        // After processing all tokens, check if current node is executable
        if (currentNode->consoleExecutable && currentNode->handler) {
            // Execute the handler with collected arguments
            currentNode->handler(nullptr, args, sendOutput);
            return true;
        }
        // Command incomplete or not executable
        output("Incomplete command.", true, {});
        return false;
    }

private:
    std::shared_ptr<RootNode> root;
    std::function<void(const std::string&, bool, const std::vector<std::string>&)> sendOutput;

    // Helper function to tokenize the input command
    static std::vector<std::string> tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        std::istringstream stream(input);
        std::string token;
        bool inQuotes = false;
        std::string currentToken;

        while (stream) {
            char c = stream.get();
            if (!stream) break;

            if (c == '\"') {
                inQuotes = !inQuotes;
                continue;
            }

            if (std::isspace(c) && !inQuotes) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            }
            else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }

        return tokens;
    }

    // Helper function to validate and parse an argument based on parserId and properties
    static bool validateArgument(const std::shared_ptr<CommandNode>& node, const std::vector<std::string>& tokens, size_t& currentTokenIndex, std::vector<std::string>& parsedArgs, const Player* player) {
        if (node->type != NodeType::ARGUMENT) {
            // Not an argument node
            return false;
        }

        auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
        if (!argNode) {
            // Failed to cast to ArgumentNode
            return false;
        }

        int parserId = argNode->parserId;
        switch (parserId) {
            case 1: { // brigadier:float
                try {
                    const std::string& token = tokens[currentTokenIndex];
                    float value = std::stof(token);

                    // Check min and max
                    if (std::holds_alternative<FloatRange>(argNode->range)) {
                        FloatRange fr = std::get<FloatRange>(argNode->range);
                        if (fr.min != 0.0f && value < fr.min) return false;
                        if (fr.max != 0.0f && value > fr.max) return false;
                    }

                    parsedArgs.emplace_back(std::to_string(value));
                    return true;
                }
                catch (std::invalid_argument&) {
                    return false;
                }
                catch (std::out_of_range&) {
                    return false;
                }
            }

            case 2: { // brigadier:double
                double value = 0.0;
                try {
                    const std::string& token = tokens[currentTokenIndex];
                    value = std::stod(token);

                    // Check range if set
                    if (std::holds_alternative<DoubleRange>(argNode->range)) {
                        DoubleRange dr = std::get<DoubleRange>(argNode->range);
                        if (dr.min != 0.0 && value < dr.min) return false;
                        if (dr.max != 0.0 && value > dr.max) return false;
                    }

                    parsedArgs.emplace_back(std::to_string(value));
                    return true;
                }
                catch (std::invalid_argument&) {
                    return false;
                }
                catch (std::out_of_range&) {
                    return false;
                }
            }
            case 3: { // brigadier:integer
                try {
                    const std::string& token = tokens[currentTokenIndex];
                    int value = std::stoi(token);

                    // Check min and max
                    if (std::holds_alternative<IntRange>(argNode->range)) {
                        IntRange ir = std::get<IntRange>(argNode->range);
                        if (ir.min != 0.0f && (value < ir.min)) return false;
                        if (ir.max != 0.0f && (value > ir.max)) return false;
                    }

                    parsedArgs.emplace_back(std::to_string(value));
                    return true;
                }
                catch (std::invalid_argument&) {
                    return false;
                }
                catch (std::out_of_range&) {
                    return false;
                }
            }
            case 6: { // minecraft:entity
                if (currentTokenIndex >= tokens.size()) return false;
                const std::string& token = tokens[currentTokenIndex];
                if (token.empty()) return false;

                parsedArgs.emplace_back(token);
                return true;
            }

            case 11: { // minecraft:vec2
                if (currentTokenIndex + 1 >= tokens.size()) return false;
                std::string tokenX = tokens[currentTokenIndex];
                std::string tokenZ = tokens[currentTokenIndex + 1];
                float x = 0.0f, z = 0.0f;

                try {
                    // tokenX contains ~
                    if (tokenX.find("~") != std::string::npos) {
                        // Remove the ~ character
                        tokenX = tokenX.substr(1);
                        // If the token is empty, the position is the player's current position
                        if (tokenX.empty()) {
                            if (player) {
                                x = player->position.x;
                            } else {
                                x = 0.0f;
                            }
                        } else {
                            x = std::stof(tokenX);
                            if (player) {
                                x += player->position.x;
                            }
                        }
                    }
                    else {
                        x = std::stof(tokenX);
                    }
                    if (tokenZ.find("~") != std::string::npos) {
                        // Remove the ~ character
                        tokenZ = tokenZ.substr(1);
                        // If the token is empty, the position is the player's current position
                        if (tokenZ.empty()) {
                            if (player) {
                                z = player->position.z;
                            } else {
                                z = 0.0f;
                            }
                        } else {
                            z = std::stof(tokenZ);
                            if (player) {
                                z += player->position.z;
                            }
                        }
                    }
                    else {
                        z = std::stof(tokenZ);
                    }
                }
                catch (const std::exception&) {
                    return false;
                }

                // Check range if set
                if (std::holds_alternative<FloatRange>(argNode->range)) {
                    FloatRange fr = std::get<FloatRange>(argNode->range);
                    if (fr.min != 0.0f && (x < fr.min || z < fr.min)) return false;
                    if (fr.max != 0.0f && (x > fr.max || z > fr.max)) return false;
                }

                // Format parsedArg as "x,z"
                std::ostringstream oss;
                oss << x << "," << z;
                parsedArgs.emplace_back(oss.str());

                currentTokenIndex++;
                return true;
            }
            case 42: { // minecraft:time
                const std::string& token = tokens[currentTokenIndex];
                int value = std::stoi(token);

                // Check min
                if (std::holds_alternative<IntRange>(argNode->range)) {
                    IntRange ir = std::get<IntRange>(argNode->range);
                    if (value < ir.min) return false;
                }

                parsedArgs.emplace_back(token);
            }
            // TODO: Handle other parserIds
            default: {
                if (currentTokenIndex >= tokens.size()) return false;
                parsedArgs.emplace_back(tokens[currentTokenIndex]);
                return true;
            }
        }
    }
};


#endif //COMMANDBUILDER_H
