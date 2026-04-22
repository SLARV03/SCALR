#include "ParserSimulator.hpp"
#include <iostream>
#include <sstream>

static std::string joinTokens(const std::vector<Token>& tokens, size_t startIndex) {
    std::stringstream ss;
    for (size_t i = startIndex; i < tokens.size(); ++i) {
        if (i > startIndex) ss << " ";
        ss << tokens[i].value;
    }
    return ss.str();
}

static std::string joinStack(const std::vector<int>& stateStack, const std::vector<Symbol>& symbolStack) {
    std::stringstream ss;
    ss << stateStack[0];
    for (size_t i = 0; i < symbolStack.size(); ++i) {
        ss << " " << symbolStack[i] << " " << stateStack[i + 1];
    }
    return ss.str();
}

ParseResult ParserSimulator::simulate(const std::vector<Token>& tokens, const ParsingTable& table, const Grammar& grammar) {
    ParseResult result;
    result.accepted = false;

    // Check for lexer errors first
    for (const auto& t : tokens) {
        if (t.type == "error") {
             result.errorMsg = "Lexical error: Unrecognized token '" + t.value + "' at line " + std::to_string(t.line) + ", col " + std::to_string(t.col);
             return result;
        }
    }

    std::vector<int> stateStack = {0};
    std::vector<Symbol> symbolStack;
    size_t inputPos = 0;

    while (true) {
        int state = stateStack.back();
        std::string currentSym = tokens[inputPos].type;

        ParseStep step;
        step.stackStr = joinStack(stateStack, symbolStack);
        step.inputStr = joinTokens(tokens, inputPos);

        // Find action for state and currentSym
        auto stateIt = table.actionTable.find(state);
        if (stateIt == table.actionTable.end()) {
            step.actionStr = "Error: Invalid state";
            result.steps.push_back(step);
            result.errorMsg = "Syntax error at token '" + currentSym + "'";
            break;
        }

        auto actionIt = stateIt->second.find(currentSym);
        if (actionIt == stateIt->second.end() || actionIt->second.empty()) {
            step.actionStr = "Error: No action defined";
            result.steps.push_back(step);
            result.errorMsg = "Syntax error at token '" + currentSym + "'";
            break;
        }

        const auto& actions = actionIt->second;
        if (actions.size() > 1) {
            step.actionStr = "Conflict encountered (" + std::to_string(actions.size()) + " actions)";
            result.steps.push_back(step);
            result.errorMsg = "Parser halted due to grammar conflict at state " + std::to_string(state) + " on symbol '" + currentSym + "'.";
            break;
        }

        Action act = actions[0];
        
        if (act.type == ActionType::SHIFT) {
            step.actionStr = "Shift to state " + std::to_string(act.target);
            stateStack.push_back(act.target);
            symbolStack.push_back(currentSym);
            inputPos++;
        } else if (act.type == ActionType::REDUCE) {
            const Rule& rule = grammar.rules[act.target];
            step.actionStr = "Reduce by " + rule.toString();
            
            // Pop RHS length
            int popCount = rule.rhs.size();

            if (stateStack.size() <= (size_t)popCount) {
                step.actionStr += " (Error: Stack underflow)";
                result.steps.push_back(step);
                result.errorMsg = "Stack underflow during reduction.";
                break;
            }

            for (int i = 0; i < popCount; ++i) {
                stateStack.pop_back();
                symbolStack.pop_back();
            }

            int topState = stateStack.back();
            symbolStack.push_back(rule.lhs);
            
            auto gotoStateIt = table.gotoTable.find(topState);
            if (gotoStateIt == table.gotoTable.end() || gotoStateIt->second.find(rule.lhs) == gotoStateIt->second.end()) {
                step.actionStr += " (Error: Missing GOTO for " + rule.lhs + ")";
                result.steps.push_back(step);
                result.errorMsg = "Missing GOTO entry for non-terminal '" + rule.lhs + "' at state " + std::to_string(topState);
                break;
            }
            
            int nextState = gotoStateIt->second.at(rule.lhs);
            stateStack.push_back(nextState);
        } else if (act.type == ActionType::ACCEPT) {
            step.actionStr = "Accept";
            result.accepted = true;
            result.steps.push_back(step);
            break;
        } else {
            step.actionStr = "Error action";
            result.steps.push_back(step);
            result.errorMsg = "Explicit error action encountered.";
            break;
        }

        result.steps.push_back(step);
    }
    
    return result;
}
