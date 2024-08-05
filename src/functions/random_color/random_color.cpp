#include "random_color.h"
#include "utils.h"

#include <iostream>
#include <sys/wait.h>

#include <Magick++.h>

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

    try {
        const int img_size = 256;
        // Create a new image with specified size and color
        double dr, dg, db;
        Magick::ColorRGB colorrgb(dr = (color / 65536) / 255.0,
                                  dg = (color / 256 % 256) / 255.0,
                                  db = (color % 256) / 255.0);
        Magick::Image image(Magick::Geometry(img_size, img_size), colorrgb);

        // Set the font and font size
        image.fontFamily("sans-serif");
        image.fontPointsize(32);

        // Set the text color
        image.fillColor(dr + dg + db >= 1.5 ? "black" : "white");

        // Annotate (write) the text on the image
        image.annotate(name, Magick::Geometry(img_size, img_size, 0, 0),
                       Magick::CenterGravity);

        // Save the image
        image.write((std::string) "./resource/r_color/" + name + ".png");
    }
    catch (std::exception &error) {
        set_global_log(LOG::ERROR, error.what());
    }

    char *c_name = curl_easy_escape(nullptr, name.c_str(), name.length());

    conf.p->cq_send("[CQ:image,file=file://" + get_local_path() +
                        "/resource/r_color/" + c_name + ".png,id=40000]",
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

DECLARE_FACTORY_FUNCTIONS(r_color)