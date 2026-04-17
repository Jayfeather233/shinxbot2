#include "dice.h"
#include "utils.h"
#include <regex>
#include <random>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

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

    static const std::regex full_expr_regex(
        R"(^\.?(?:(?:\d{0,3}[dD]\d{1,3})|\d{1,4})(?:[+-](?:(?:\d{0,3}[dD]\d{1,3})|\d{1,4}))*$)"
    );

    static const std::regex term_regex(
        R"(([+-]?)(\d{0,3}[dD]\d{1,3}|\d{1,4}))"
    );

    static const std::regex dice_term_regex(
        R"((\d{0,3})[dD](\d{1,3}))"
    );

    static thread_local std::mt19937 rng(std::random_device{}());

    if (!std::regex_match(message, full_expr_regex)) {
        return;
    }

    std::string expr = message;
    if (!expr.empty() && expr[0] == '.') {
        expr.erase(0, 1);
    }

    long long total = 0;
    std::vector<std::string> rendered_terms;

    bool has_multiple_terms = false;
    {
        int term_count = 0;
        for (std::sregex_iterator i(expr.begin(), expr.end(), term_regex), end; i != end; ++i) {
            ++term_count;
            if (term_count > 1) {
                has_multiple_terms = true;
                break;
            }
        }
    }

    for (std::sregex_iterator i(expr.begin(), expr.end(), term_regex), end; i != end; ++i) {
        std::smatch term_match = *i;
        std::string sign_str = term_match[1].str();
        std::string body = term_match[2].str();

        int sign = (sign_str == "-") ? -1 : 1;

        std::smatch dice_match;
        if (std::regex_match(body, dice_match, dice_term_regex)) {
            std::string num_str = dice_match[1].str();
            std::string side_str = dice_match[2].str();

            int num = num_str.empty() ? 1 : std::stoi(num_str);
            if (num <= 0) num = 1;

            int sides = std::stoi(side_str);
            if (sides <= 0) return;

            std::uniform_int_distribution<int> dist(1, sides);

            std::vector<int> rolls;
            long long dice_sum = 0;

            for (int n = 0; n < num; ++n) {
                int r = dist(rng);
                rolls.push_back(r);
                dice_sum += r;
            }

            total += sign * dice_sum;

            std::ostringstream part;

            if (!rendered_terms.empty()) {
                part << (sign > 0 ? "+ " : "- ");
            } else if (sign < 0) {
                part << "-";
            }

            bool need_paren = (num > 1);
            bool is_first_negative_single = rendered_terms.empty() && sign < 0;

            if (need_paren) {
                if (is_first_negative_single) part << "(";
                else part << "(";

                for (size_t j = 0; j < rolls.size(); ++j) {
                    if (j > 0) part << "+";
                    part << rolls[j];

                    if (part.tellp() > 1000) {
                        part << "...";
                        break;
                    }
                }
                part << ")";
            } else {
                part << rolls[0];
            }

            rendered_terms.push_back(part.str());
        } else {
            int value = std::stoi(body);
            total += sign * value;

            std::ostringstream part;
            if (!rendered_terms.empty()) {
                part << (sign > 0 ? "+ " : "- ") << value;
            } else {
                if (sign < 0) part << "-";
                part << value;
            }

            rendered_terms.push_back(part.str());
        }
    }

    std::ostringstream oss;
    oss << message << ": " << total;

    if (!rendered_terms.empty()) {
        bool single_term = (rendered_terms.size() == 1);

        if (!single_term) {
            oss << " = ";
            for (size_t i = 0; i < rendered_terms.size(); ++i) {
                if (i > 0) oss << " ";
                oss << rendered_terms[i];
            }
        } else {
            std::smatch only_dice_match;
            if (std::regex_match(expr, only_dice_match, dice_term_regex)) {
                std::string num_str = only_dice_match[1].str();
                int num = num_str.empty() ? 1 : std::stoi(num_str);
                if (num > 1) {
                    oss << " = " << rendered_terms[0];
                }
            }
        }
    }

    std::string reply = "[CQ:reply,id=" + std::to_string(conf.message_id) + "]" + oss.str();
    conf.p->cq_send(reply, conf);
    conf.p->setlog(LOG::INFO,
        "dice_module: user " + std::to_string(conf.user_id) + " rolled " + message);
}

bool dice::check(std::string message, const msg_meta &conf) {
    (void)conf;
    message = trim(message);
    message = remove_all_spaces(message);

    static const std::regex full_expr_regex(
        R"(^\.?(?:(?:\d{0,3}[dD]\d{1,3})|\d{1,4})(?:[+-](?:(?:\d{0,3}[dD]\d{1,3})|\d{1,4}))*$)"
    );

    return std::regex_match(message, full_expr_regex);
}

std::string dice::help() {
    return "骰子投掷：输入如 d20、3d6、3d6+5、2d6+2d4+3、.2d6+2d4+3 的表达式进行投掷";
}

DECLARE_FACTORY_FUNCTIONS(dice)