#include "utils.h"


bool is_op(const bot *p, const int64_t a) { return p->is_op(a); }
std::string cq_send(const bot *p, const std::string &message,
                    const msg_meta &conf)
{
    return p->cq_send(message, conf);
}
std::string cq_send(const bot *p, const std::string &end_point,
                    const Json::Value &J)
{
    return p->cq_send(end_point, J);
}
std::string cq_get(const bot *p, const std::string &end_point)
{
    return p->cq_get(end_point);
}
void setlog(bot *p, LOG type, std::string message) { p->setlog(type, message); }

int64_t get_botqq(const bot *p) { return p->get_botqq(); }

std::string get_local_path() { return std::filesystem::current_path(); }
void input_process(bot *p, std::string *input) { p->input_process(input); }
