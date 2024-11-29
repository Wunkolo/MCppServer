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

// Type alias for command handlers
using CommandHandler = std::function<void(const Player*, const std::vector<std::string>&, std::function<void(const std::string&, bool)>)>;

void buildAllCommands();

// Base Command Node
class CommandNode {
public:
    NodeType type;
    bool isExecutable;
    bool consoleExecutable;
    bool hasRedirect;
    int redirectIndex;
    std::string name;
    int parserId;

    // Handler function (only for executable nodes)
    CommandHandler handler;

    std::vector<std::shared_ptr<CommandNode>> children;

    CommandNode(NodeType type, const std::string& name = "", int parserId = -1)
        : type(type), isExecutable(false), consoleExecutable(false), hasRedirect(false), redirectIndex(-1), name(name),
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
    int minValue;
    int maxValue;

    ArgumentNode(const std::string& argumentName, int parserId)
        : CommandNode(NodeType::ARGUMENT, argumentName, parserId), parserProperties(0), minValue(0), maxValue(0) {
    }

    void setParserProperties(uint8_t properties) {
        parserProperties = properties;
    }

    void setIntegerRange(int min, int max) {
        minValue = min;
        maxValue = max;
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

            // Parser ID
            if (node->type == NodeType::ARGUMENT) {
                writeVarInt(serializedNodes, node->parserId);
                if (node->parserId == 6) { // minecraft:entity
                    if (auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node)) {
                        writeByte(serializedNodes, argNode->parserProperties);
                    }
                }
                // TODO: Serialize other parser properties
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
    CommandParser(const std::shared_ptr<RootNode> &rootNode, const std::function<void(const std::string&, bool)> &outputFunc) : root(rootNode), sendOutput(outputFunc) {}

    bool parseAndExecute(ClientConnection& client, const Player& player, const std::string& inputCommand) const {
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
                    std::string parsedArg;
                    if (validateArgument(child, token, parsedArg)) {
                        args.push_back(parsedArg);
                        currentNode = child;
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched) {
                // No matching child found
                sendOutput("Unknown command or invalid arguments.", true);
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
            sendOutput("Incomplete command.", true);
            return false;
        }
    }

    bool parseAndExecuteConsole(const std::string& inputCommand, const std::function<void(const std::string&, bool)> &output) const {
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
                    std::string parsedArg;
                    if (validateArgument(child, token, parsedArg)) {
                        args.push_back(parsedArg);
                        currentNode = child;
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched) {
                // No matching child found
                output("Unknown command or invalid arguments.", true);
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
        output("Incomplete command.", true);
        return false;
    }

private:
    std::shared_ptr<RootNode> root;
    std::function<void(const std::string&, bool)> sendOutput;

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
    static bool validateArgument(const std::shared_ptr<CommandNode>& node, const std::string& token, std::string& parsedArg) {
        if (node->parserId == 6) { // minecraft:entity
            auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
            if (!argNode) return false;

            parsedArg = token;
            return true;
        }
        if (node->parserId == 3) { // brigadier:integer
            try {
                int value = std::stoi(token);
                auto argNode = std::dynamic_pointer_cast<ArgumentNode>(node);
                if (!argNode) return false;

                // Check min and max
                if (value < argNode->minValue || value > argNode->maxValue) {
                    return false;
                }

                parsedArg = std::to_string(value);
                return true;
            }
            catch (std::invalid_argument&) {
                return false;
            }
            catch (std::out_of_range&) {
                return false;
            }
        }
        // TODO: Handle other parserIds

        // If parserId not handled, accept the argument as-is
        parsedArg = token;
        return true;
    }
};


#endif //COMMANDBUILDER_H
