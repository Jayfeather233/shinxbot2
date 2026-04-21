#pragma once

#include <fmt/core.h>
#include <string>
#include <vector>

#include <cctype>
#include <random>
using u32 = uint32_t;
using engine = std::mt19937;

static engine &drng()
{
    thread_local static engine gen(std::random_device{}());
    return gen;
}

static int dice_random(int maxi)
{
    if (maxi <= 0)
        return 0;
    return std::uniform_int_distribution<int>(1, maxi)(drng());
}

namespace dice_tokenizer {
enum class TokenType {
    NUM,
    PLUS,
    MINUS,
    MUL,
    DIV,
    POW,
    D,
    LPAREN,
    RPAREN,
    END
};

struct Token {
    TokenType type;
    int value;
};

enum class OpType { NUMBER, ADD, SUB, MUL, DIV, POW, D_UNARY, D_BINARY };

template <typename T> std::string myToString(T num)
{
    return std::to_string(num);
}

std::string myToString(const double &num)
{
    int inum = int(num + 0.4999);
    if (num < 0) {
        inum = int(num - 0.4999);
    }
    if (std::abs(num - inum) < 0.0001) {
        return std::to_string(inum);
    }
    else {
        return std::to_string(num);
    }
}

inline int getPreority(const OpType &op)
{
    switch (op) {
    case OpType::ADD:
    case OpType::SUB:
        return 1;
    case OpType::MUL:
    case OpType::DIV:
        return 2;
    case OpType::POW:
        return 3;
    case OpType::D_UNARY:
    case OpType::D_BINARY:
        return 4;
    default:
        return 0;
    }
}

// template<typename T>
// inline std::string opToString(const OpType &op, const T val) {
//     switch (op) {
//         case OpType::NUMBER: return myToString(val);
//         case OpType::ADD: return "ADD";
//         case OpType::SUB: return "SUB";
//         case OpType::MUL: return "MUL";
//         case OpType::DIV: return "DIV";
//         case OpType::POW: return "POW";
//         case OpType::D_UNARY: return "D_UNARY";
//         case OpType::D_BINARY: return "D_BINARY";
//     }
//     return "UNKNOWN";
// }
template <typename T>
inline std::string opToString(const OpType &op, const T val)
{
    switch (op) {
    case OpType::NUMBER:
        return myToString(val);
    case OpType::ADD:
        return "+";
    case OpType::SUB:
        return "-";
    case OpType::MUL:
        return "*";
    case OpType::DIV:
        return "/";
    case OpType::POW:
        return "^";
    case OpType::D_UNARY:
        return "d";
    case OpType::D_BINARY:
        return "d";
    }
    return "UNKNOWN";
}

struct ExpTree {
    OpType op;
    int value;
    double calcVal;
    std::string renderedStr;
    ExpTree *left;
    ExpTree *right;

    ExpTree(int v)
        : op(OpType::NUMBER), value(v), calcVal(0), left(nullptr),
          right(nullptr)
    {
    }
    ExpTree(OpType op, ExpTree *l = nullptr, ExpTree *r = nullptr)
        : op(op), value(0), calcVal(0), left(l), right(r)
    {
    }

    std::string output()
    {
        switch (op) {
        case OpType::NUMBER:
            return myToString(value);

        case OpType::D_UNARY:
            return fmt::format("d{}", right->output());

        default:
            std::string leftStr = left ? left->output() : "";
            std::string rightStr = right ? right->output() : "";
            if (getPreority(op) > 1) {
                if (left && left->op != OpType::NUMBER &&
                    getPreority(left->op) < getPreority(op)) {
                    leftStr = "(" + leftStr + ")";
                }
                if (right && right->op != OpType::NUMBER &&
                    getPreority(right->op) < getPreority(op)) {
                    rightStr = "(" + rightStr + ")";
                }
            }
            return fmt::format("{}{}{}", leftStr, opToString(op, value),
                               rightStr);
        }
    }

    std::string outputPostfix()
    {
        std::string result = "";
        if (left)
            result += left->outputPostfix() + " ";
        if (right)
            result += right->outputPostfix() + " ";

        if (op == OpType::NUMBER)
            result += myToString(value);
        else
            result += opToString(op, value);

        return result;
    }

    void parseVal()
    {
        left ? left->parseVal() : void();
        right ? right->parseVal() : void();
        if (op == OpType::NUMBER) {
            calcVal = value;
        }
        else {
            calcVal = 0;
        }
    }

    /**
     * Recursively evaluate the expression tree where possible, reducing it to a
     * simpler form. This does not perform dice rolls, only arithmetic
     * simplification.
     * @return true if the node was reduced to a number
     */
    bool reduce()
    {
        renderedStr = "";
        if (op == OpType::NUMBER) {
            renderedStr = myToString(calcVal);
            return true;
        }

        bool leftReduced = left ? left->reduce() : true;
        bool rightReduced = right ? right->reduce() : true;

        std::string leftStr = left ? left->renderedStr : "";
        std::string rightStr = right ? right->renderedStr : "";
        if (getPreority(op) > 1) {
            if (left && left->op != OpType::NUMBER &&
                getPreority(left->op) < getPreority(op)) {
                leftStr = "(" + leftStr + ")";
            }
            if (right && right->op != OpType::NUMBER &&
                getPreority(right->op) < getPreority(op)) {
                rightStr = "(" + rightStr + ")";
            }
        }
        renderedStr =
            fmt::format("{}{}{}", leftStr, opToString(op, calcVal), rightStr);

        if (leftReduced && rightReduced) {
            double leftVal = left ? left->calcVal : 0;
            double rightVal = right ? right->calcVal : 0;

            switch (op) {
            case OpType::ADD:
                calcVal = leftVal + rightVal;
                break;
            case OpType::SUB:
                calcVal = leftVal - rightVal;
                break;
            case OpType::MUL:
                calcVal = leftVal * rightVal;
                break;
            case OpType::DIV:
                calcVal = rightVal != 0 ? leftVal / rightVal : 0;
                break;
            case OpType::POW:
                calcVal = pow(leftVal, rightVal);
                break;
            default:
                return false;
            }

            op = OpType::NUMBER;
            renderedStr = myToString(calcVal);
            delete left;
            delete right;
            left = nullptr;
            right = nullptr;

            return true;
        }

        return false;
    }

    /**
     * Perform dice rolls in the expression tree. This modifies the tree
     * in-place, replacing dice operations with their rolled values. Only
     * processes nodes that on the bottom.
     * @return true if any dice were rolled
     */
    bool doDice()
    {
        renderedStr = "";
        if (op == OpType::D_UNARY) {
            if (right->op != OpType::NUMBER) {
                bool ret = right->doDice();
                if (right && right->op != OpType::NUMBER &&
                    getPreority(right->op) < getPreority(op)) {
                    renderedStr = "d(" + right->renderedStr + ")";
                }
                else {
                    renderedStr = "d" + right->renderedStr;
                }
                return ret;
            }
            int sides = right->calcVal < 0 ? right->calcVal - 0.4999
                                           : right->calcVal + 0.4999;

            int rval = dice_random(sides);
            calcVal = rval;
            renderedStr = fmt::format("(d{}={})", sides, rval);
            op = OpType::NUMBER;
            delete right;
            right = nullptr;
            return true;
        }

        if (op == OpType::D_BINARY) {
            if (!left || !right || left->op != OpType::NUMBER ||
                right->op != OpType::NUMBER) {
                if (!left || !right) {
                    return false;
                }
                bool ret1 = left->doDice();
                bool ret2 = right->doDice();
                std::string leftStr = left ? left->renderedStr : "";
                std::string rightStr = right ? right->renderedStr : "";
                if (left && left->op != OpType::NUMBER &&
                    getPreority(left->op) < getPreority(op)) {
                    leftStr = "(" + leftStr + ")";
                }
                if (right && right->op != OpType::NUMBER &&
                    getPreority(right->op) < getPreority(op)) {
                    rightStr = "(" + rightStr + ")";
                }
                renderedStr = fmt::format("{}d{}", leftStr, rightStr);
                return ret1 || ret2;
            }

            int num = left->calcVal < 0 ? left->calcVal - 0.4999
                                        : left->calcVal + 0.4999;
            int sides = right->calcVal < 0 ? right->calcVal - 0.4999
                                           : right->calcVal + 0.4999;

            int rval = 0;
            for (int i = 0; i < num; ++i) {
                int rrval = dice_random(sides);
                rval += rrval;
                renderedStr += myToString(rrval);
                if (i < num - 1)
                    renderedStr += "+";
            }
            if (num <= 1) {
                renderedStr = fmt::format("({}d{}={})", num, sides, rval);
            }
            else {
                renderedStr = fmt::format("({}d{}={}=({}))", num, sides, rval,
                                          renderedStr);
            }
            calcVal = rval;

            op = OpType::NUMBER;
            delete left;
            delete right;
            left = nullptr;
            right = nullptr;
            return true;
        }

        bool leftDone = left ? left->doDice() : false;
        bool rightDone = right ? right->doDice() : false;

        std::string leftStr = left ? left->renderedStr : "";
        std::string rightStr = right ? right->renderedStr : "";
        if (left && left->op != OpType::NUMBER &&
            getPreority(left->op) < getPreority(op)) {
            leftStr = "(" + leftStr + ")";
        }
        if (right && right->op != OpType::NUMBER &&
            getPreority(right->op) < getPreority(op)) {
            rightStr = "(" + rightStr + ")";
        }
        renderedStr =
            fmt::format("{}{}{}", leftStr, opToString(op, calcVal), rightStr);

        return leftDone || rightDone;
    }
};

std::vector<std::string> reduceAll(ExpTree *node)
{
    std::vector<std::string> steps;
    if (!node)
        return steps;
    node->parseVal();

    while (true) {
        bool reduced = node->reduce();
        if (reduced) {
            steps.push_back(node->renderedStr);
            break;
        }
        bool diced = node->doDice();
        steps.push_back(node->renderedStr);
        if (!diced) {
            fmt::print("Warning: Could not reduce or do dice for node: {}\n",
                       node->renderedStr);
            throw std::runtime_error("Failed to reduce or do dice for node");
        }
    }

    return steps;
}

class Lexer {
    std::string s;
    int pos = 0;

public:
    Lexer(const std::string &str) : s(str) {}

    Token next()
    {
        while (pos < s.size() && isspace(s[pos]))
            pos++;

        if (pos >= s.size())
            return {TokenType::END, 0};

        char c = s[pos];

        if (isdigit(c)) {
            int val = 0;
            while (pos < s.size() && isdigit(s[pos])) {
                val = val * 10 + (s[pos++] - '0');
            }
            return {TokenType::NUM, val};
        }

        pos++;
        switch (c) {
        case '+':
            return {TokenType::PLUS, 0};
        case '-':
            return {TokenType::MINUS, 0};
        case '*':
            return {TokenType::MUL, 0};
        case '/':
            return {TokenType::DIV, 0};
        case '^':
            return {TokenType::POW, 0};
        case 'd':
            return {TokenType::D, 0};
        case 'D':
            return {TokenType::D, 0};
        case '(':
            return {TokenType::LPAREN, 0};
        case ')':
            return {TokenType::RPAREN, 0};
        }

        throw std::runtime_error("Invalid character");
    }
};
class Parser {
    // expr        = add_sub
    // add_sub     = mul_div ((+|-) mul_div)*
    // mul_div     = power ((*|/) power)*
    // power       = d_level (^ d_level)*
    // d_level     = (d d_level) | primary (d d_level)*
    // primary     = number | (expr)
    Lexer lexer;
    Token cur;

    void next() { cur = lexer.next(); }

public:
    Parser(const std::string &s) : lexer(s) { next(); }

    ExpTree *parse() { return parseAddSub(); }

private:
    ExpTree *parseAddSub()
    {
        auto node = parseMulDiv();
        while (cur.type == TokenType::PLUS || cur.type == TokenType::MINUS) {
            TokenType op = cur.type;
            next();
            auto right = parseMulDiv();
            node = new ExpTree(
                op == TokenType::PLUS ? OpType::ADD : OpType::SUB, node, right);
        }
        return node;
    }

    ExpTree *parseMulDiv()
    {
        auto node = parsePower();
        while (cur.type == TokenType::MUL || cur.type == TokenType::DIV) {
            TokenType op = cur.type;
            next();
            auto right = parsePower();
            node = new ExpTree(op == TokenType::MUL ? OpType::MUL : OpType::DIV,
                               node, right);
        }
        return node;
    }

    ExpTree *parsePower()
    {
        auto node = parseDLevel();
        while (cur.type == TokenType::POW) {
            next();
            auto right = parseDLevel();
            node = new ExpTree(OpType::POW, node, right);
        }
        return node;
    }

    ExpTree *parseDLevel()
    {
        ExpTree *node = nullptr;

        if (cur.type == TokenType::D) {
            next();
            auto right = parseDLevel();
            node = new ExpTree(OpType::D_UNARY, nullptr, right);
        }
        else {
            node = parsePrimary();
        }

        while (cur.type == TokenType::D) {
            next();
            auto right = parseDLevel();
            node = new ExpTree(OpType::D_BINARY, node, right);
        }

        return node;
    }

    ExpTree *parsePrimary()
    {
        if (cur.type == TokenType::NUM) {
            int val = cur.value;
            next();
            return new ExpTree(val);
        }

        if (cur.type == TokenType::LPAREN) {
            next();
            auto node = parseAddSub();
            if (cur.type != TokenType::RPAREN)
                throw std::runtime_error("Missing )");
            next();
            return node;
        }

        throw std::runtime_error("Invalid expression");
    }
};
}; // namespace dice_tokenizer