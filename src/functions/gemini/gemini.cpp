#include "gemini.h"
#include "image_utils.h"

#include <fstream>

/*
// count_token
curl
https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:countTokens?key=$API_KEY
\
    -H 'Content-Type: application/json' \
    -X POST \
    -d '{
      "contents": [{
        "parts":[{
          "text": "Write a story about a magic backpack."}]}]}' > response.json
//response
{
  "totalTokens": 8
}


// request example
PNG - image/png
JPEG - image/jpeg
WEBP - image/webp
HEIC - image/heic
HEIF - image/heif
最多 16 张图片
整个提示（包括图片和文本）不得超过 4MB
对图像中的像素数没有具体限制；但是，较大的图像会缩小以适应最大分辨率 (3072 x
3072)，同时保持其原始宽高比。
curl
https://generativelanguage.googleapis.com/v1beta/models/gemini-pro[-vision]:generateContent?key=$API_KEY
\
    -H 'Content-Type: application/json' \
    -X POST \
    -d '{
      "contents": [
        {"role":"user",
         "parts":[{"text": "Write the first ..."},

            {"inline_data": {
                "mime_type":"image/jpeg",
                "data": "'$(base64 -w0 image.jpg)'"
            }}
         ]
        },
        {"role": "model",
         "parts":[{
           "text": "In the bustling city of Meadow brook,..."}]
        },
        {"role": "user",
         "parts":[{
           "text": "Can you set it ..."}]
        },
      ],
        "generationConfig": {
            "stopSequences": [
                "Title"
            ],
            "temperature": 1.0,
            "maxOutputTokens": 800,
            "topP": 0.8,
            "topK": 10
        }
    }'


// response example
{
  "candidates": [
    {
      "content": {
        "parts": [
          {
            "text": "Once upon a time, ..."
          }
        ],
        "role": "model"
      },
      "finishReason": "STOP",
      "index": 0
      ]
    }
  ],
}

// my history:
[
    {
        "role":"user",
        "parts":[
            {"text": "Write the first ..."},
            {"inline_data": {
                "mime_type":"image/jpeg",
                "data": "'$(base64 -w0 image.jpg)'"
            }}
        ]
    },
    {
        "role": "model",
        "parts":[{
            "text": "In the bustling city of Meadow brook,..."}]
    },
    {
        "role": "user",
        "parts":[{
            "text": "Can you set it ..."}]
    },
]
*/
size_t MAX_PRO_LENGTH = 30720;
size_t MAX_PRO_REPLY = 2048;
size_t MAX_PRO_VISION_LENGTH = 12288;
size_t MAX_PRO_VISION_REPLY = 4096;

const std::string help_msg =
    "Gemini by Google.\nUseage:\n.gem For text only\n.gemvi For image with "
    "text\n.gem.reset for reset histroy.";

#define next_key(nowkey)                                                       \
    nowkey++;                                                                  \
    if (nowkey == keys.end())                                                  \
        nowkey = keys.begin();

gemini::gemini()
{
    if (!std::filesystem::exists("./config/gemini.json")) {
        std::cout << "Please config your gemini key in gemini.json (and "
                     "restart) OR see gemini_example.json"
                  << std::endl;
        std::ofstream of("./config/gemini.json", std::ios::out);
        of << "{"
              "\"keys\": [\"\"],"
              "\"mode\": [\"default\"],"
              "\"modes\":{\"default\": []},"
              "\"black_list\": [\"这是要屏蔽的词\"]"
              "}";
        of.close();
    }
    else {
        Json::Value conf =
            string_to_json(readfile("./config/gemini.json", "{}"));
        Json::ArrayIndex sz = conf["keys"].size();
        for (Json::ArrayIndex i = 0; i < sz; ++i) {
            keys.push_back(conf["keys"][i].asString());
        }
        nowkey = keys.begin();
        sz = conf["mode"].size();
        for (Json::ArrayIndex i = 0; i < sz; ++i) {
            std::string tmp = conf["mode"][i].asString();
            modes.push_back(tmp);
            mode_prompt[tmp] = conf["modes"][tmp];
            if (i == 0) {
                default_prompt = tmp;
            }
        }
    }
}

size_t gemini::get_tokens(const Json::Value &history)
{
    Json::Value qes;
    qes["content"] = history;
    qes = string_to_json(
        do_post((std::string) "https://generativelanguage.googleapis.com/"
                              "v1beta/models/gemini-pro:countTokens?key=" +
                    *nowkey,
                qes, {}, true));
    return qes["totalTokens"].as<size_t>();
}

void gemini::shrink_prompt_size(uint64_t u, bool is_vision)
{
    size_t limits = is_vision ? (MAX_PRO_VISION_LENGTH - MAX_PRO_VISION_REPLY)
                              : (MAX_PRO_LENGTH - MAX_PRO_REPLY);
    Json::Value ign;
    while (get_tokens(history[is_vision][u]) > limits) {
        history[is_vision][u].removeIndex(0, &ign);
    }
}

std::string gemini::generate_text(std::string message, uint64_t id)
{
    Json::Value J;
    J["role"] = "user";
    J["parts"][0]["text"] = message;
    history[0][id].append(J);
    J.clear();
    J["contents"] = history[0][id];
    shrink_prompt_size(id, 0);
    Json::Value res = string_to_json(do_post(
        (std::string) "https://generativelanguage.googleapis.com/v1beta/models/"
                      "gemini-1.5-pro-latest:generateContent?key=" +
            *nowkey,
        J, {}, true));
    next_key(nowkey);
    std::string str_ans;
    if (res.isMember("error")) {
        str_ans = res.toStyledString();
    }
    else {
        str_ans =
            res["candidates"][0]["content"]["parts"][0]["text"].asString();
        J.clear();
        J["role"] = "model";
        J["parts"][0]["text"] = str_ans;
        history[0][id].append(J);
    }
    return str_ans;
}
std::string gemini::generate_image(std::string message, uint64_t id)
{
    int cnt = 0;
    size_t index = -1, index2 = -1;
    std::string fn = std::to_string(get_random());
    while (true) {
        index = message.find(",url=", index + 1);
        if (index == message.npos)
            break;
        index += 5;
        index2 = index;
        while (message[index2] != ']') {
            ++index2;
        }
        download(cq_decode(message.substr(index, index2 - index)),
                 "./resource/download/", fn);
        cnt++;
        break;
    }
    index = message.find("[CQ:image");
    Json::Value J;
    if (cnt == 0) {
        // J["role"] = "user";
        // J["parts"][0]["text"] = message;
        // history[1][id].append(J);
        return "Picture please.";
    }
    else {
        std::pair<std::string, std::string> img =
            image2base64((std::string) "./resource/download/" + fn);
        J["role"] = "user";
        J["parts"][0]["text"] =
            message.substr(0, index) + message.substr(index2 + 1);
        J["parts"][1]["inline_data"]["mime_type"] = img.first;
        J["parts"][1]["inline_data"]["data"] = img.second;
        // history[1][id].append(J);
    }
    // J.clear();
    // J["contents"] = history[1][id];
    Json::Value Q;
    Q["contents"][0] = J;
    // shrink_prompt_size(id, 1);
    Json::Value res = string_to_json(do_post(
        (std::string) "https://generativelanguage.googleapis.com/v1beta/models/"
                      "gemini-pro-vision:generateContent?key=" +
            *nowkey,
        Q, {}, true));
    next_key(nowkey);
    std::string str_ans;
    if (res.isMember("error")) {
        str_ans = res.toStyledString();
    }
    else {
        str_ans =
            res["candidates"][0]["content"]["parts"][0]["text"].asString();
        // str_ans = res.toStyledString();
        // J.clear();
        // J["role"] = "model";
        // J["parts"][0]["text"] = str_ans;
        // history[1][id].append(J);
    }
    return str_ans;
}

void gemini::process(std::string message, const msg_meta &conf)
{
    uint64_t id = conf.message_type == "group" ? (conf.group_id << 1)
                                               : ((conf.user_id << 1) | 1);
    message = message.substr(4);
    std::string result;
    if (message.find("vi") == 0) {
        message = message.substr(2);
        if (message.find(".reset") == 0) {
            history[1][id].clear();
            cq_send(conf.p, "clear done.", conf);
            return;
        }
        else if (message.find(".help") == 0) {
            cq_send(conf.p, help_msg, conf);
            return;
        }
        result = generate_image(message, id);
    }
    else {
        if (message.find(".reset") == 0) {
            history[0][id].clear();
            cq_send(conf.p, "clear done.", conf);
            return;
        }
        else if (message.find(".help") == 0) {
            cq_send(conf.p, help_msg, conf);
            return;
        }
        result = generate_text(message, id);
    }
    cq_send(conf.p, result, conf);
}
bool gemini::check(std::string message, const msg_meta &conf)
{
    return message.find(".gem") == 0;
}
std::string gemini::help()
{
    return "gemini: MultiModal AI,\n\tuseage: .gem for text only,\n\t.gemvi "
           "for image with text";
}
DECLARE_FACTORY_FUNCTIONS(gemini)