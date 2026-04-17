#include "dice.h"
#include "utils.h"
#include "dice_tokenizer.hpp"

static std::string remove_all_spaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (!std::isspace(ch)) {
            out.push_back(static_cast<char>(ch));
        }
    }
    return out;
}

void dice::process(std::string message, const msg_meta &conf) {
    message = trim(message);
    message = remove_all_spaces(message);

    try {
        if (!message.empty() && message[0] == '.') {
            message.erase(0, 1);
        }
        dice_tokenizer::Parser parser(message);
        auto tree = parser.parse();
        std::string ori = tree->output();
        std::vector<std::string> steps = dice_tokenizer::reduceAll(tree);

        std::ostringstream oss;
        for (size_t i = 0; i < steps.size(); ++i) {
            oss << "\n=" << steps[i];
        }

        if (oss.str().length() < 300) {
            ori += oss.str();
        } else {
            tree->reduce();
            ori += fmt::format(" = {}", tree->renderedStr);
        }

        std::string reply = "[CQ:reply,id=" + std::to_string(conf.message_id) + "]" + ori;
        conf.p->cq_send(reply, conf);
        conf.p->setlog(LOG::INFO,
            "dice_module: user " + std::to_string(conf.user_id) + " rolled " + message);

        delete tree;
    } catch (const std::exception& e) {
        // conf.p->setlog(LOG::ERROR,
        //     "dice_module: error processing message from user " + std::to_string(conf.user_id) + ": " + e.what());
    }
}

bool dice::check(std::string message, const msg_meta &conf) {
    return true;
}

std::string dice::help() {
    return "骰子投掷：输入如 d20、3d6、3d6+5、2d6+2d4+3、.2d6+2d4+3 的表达式进行投掷";
}

DECLARE_FACTORY_FUNCTIONS(dice)