#include "random_color.h"
#include "utils.h"

#include <iostream>
#include <sys/wait.h>

std::string int_to_hex = "0123456789ABCDEF";

std::string get_code(int color)
{
    std::string res = "#";
    for (int i = 1048576; i >= 1; i /= 16) {
        res += int_to_hex[color / i % 16];
    }
    return res;
}

void r_color::process(std::string message, const msg_meta &conf)
{
    Json::Value J;
    J["message_id"] = conf.message_id;
    conf.p->cq_send("mark_msg_as_read", J);
    std::wstring w_mess = trim(string_to_wstring(message).substr(4));
    int color = 0;
    if (w_mess[0] == L'#') {
        for (int i = 1; i <= 6; i++) {
            if (w_mess[i] > L'9' || L'0' > w_mess[i]) {
                if (w_mess[i] > L'Z' || L'A' > w_mess[i]) {
                    color = (color << 4) + w_mess[i] + 10 - L'a';
                }
                else {
                    color = (color << 4) + w_mess[i] + 10 - L'A';
                }
            }
            else {
                color = (color << 4) + w_mess[i] - L'0';
            }
        }
    }
    else {
        color =
            get_random(256) * 65536 + get_random(256) * 256 + get_random(256);
    }
    color = color % 16777216;

    std::string name = get_code(color);

    int pid = fork();
    if (pid < 0) {
        throw "fork failed.";
    }
    else if (pid == 0) {
        CImg<unsigned char> img(256, 256, 1, 3);

        unsigned char color_code[] = {
            static_cast<unsigned char>(color / 65536),
            static_cast<unsigned char>(color / 256 % 256),
            static_cast<unsigned char>(color % 256)};

        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                img(i, j, 0, 0) = color_code[0];
                img(i, j, 0, 1) = color_code[1];
                img(i, j, 0, 2) = color_code[2];
            }
        }

        int cnt = 0;
        for (int i = 0; i < 25; i++) {
            cnt += (color & (1 << i)) ? 1 : 0;
        }

        const unsigned char white[] = {255, 255, 255};
        const unsigned char black[] = {0, 0, 0};

        int img_width = img.width();
        int img_height = img.height();
        int center_x = img_width / 2;
        int center_y = img_height / 2;

        CImgList<> *font_size = new CImgList<>(CImgList<>::font(32, false));

        CImg<unsigned char> text;
        text.draw_text(0, 0, name.c_str(), cnt >= 12 ? black : white, 0, 1,
                       font_size);

        // Set the font size, color and style
        img.draw_text(center_x - text.width() / 2, center_y - text.height() / 2,
                      name.c_str(), cnt >= 12 ? black : white, 0, 1, font_size);

        // Save the image to a file
        img.save_png(
            ((std::string) "./resource/temp/" + name + ".png").c_str());
        delete font_size;
        exit(0);
    }
    else {
        waitpid(pid, 0, 0);
    }

    char *c_name = curl_easy_escape(nullptr, name.c_str(), name.length());

    conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                        "/resource/temp/" + c_name + ".png,id=40000]",
                    conf);
    curl_free(c_name);
    conf.p->setlog(LOG::INFO, "r_color at group " +
                                  std::to_string(conf.group_id) + " by " +
                                  std::to_string(conf.user_id));
}
bool r_color::check(std::string message, const msg_meta &conf)
{
    return string_to_wstring(message).find(L"来点色图") == 0;
}
std::string r_color::help() { return "来点色图：来点色图+#color_hex_code"; }
