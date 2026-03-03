#include "nativeQuery.h"

// .nq endpoint {"json": "data"} -> send to endpoint with data, return response

void nativeQuery::process(std::string message, const msg_meta &conf){
    message = cq_decode(message);
    std::istringstream iss(trim(message.substr(3))); // remove ".nq"
    std::string endpoint, data, line;;
    iss >> endpoint;
    while (std::getline(iss, line)) {
        data += line + "\n";
    }
    try {
        std::string response = conf.p->cq_send(endpoint, string_to_json(data));
        conf.p->cq_send(cq_encode(string_to_json(response).toStyledString()), conf);
    } catch (std::exception &e) {
        conf.p->setlog(LOG::ERROR, std::string("nativeQuery: exception: ") + e.what());
        conf.p->cq_send("nativeQuery 出错: " + std::string(e.what()), conf);
    } catch (const char *s) {
        conf.p->setlog(LOG::ERROR, std::string("nativeQuery: exception: ") + s);
        conf.p->cq_send("nativeQuery 出错: " + std::string(s), conf);
    } catch (...) {
        conf.p->setlog(LOG::ERROR, "nativeQuery: unknown exception\n");
        conf.p->cq_send("nativeQuery 出错: 未知错误", conf);
    }
}
bool nativeQuery::check(std::string message, const msg_meta &conf) {
    return message.find(".nq") == 0 && conf.p->is_op(conf.user_id);
}
std::string nativeQuery::help() {
    return "";
}

DECLARE_FACTORY_FUNCTIONS(nativeQuery)