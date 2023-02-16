#include "random_color.h"
#include "utils.h"

#include <opencv2/opencv.hpp>

std::string int_to_hex = "0123456789ABCDEF";

r_color::r_color(){
    std::random_device os_seed;
    const u32 seed = os_seed();
    generator = engine(seed);
}

std::string get_code(int color){
    std::string res = "#";
    for(int i=1048576;i>=1;i/=16){
        res += int_to_hex[color/i%16];
    }
    return res;
}

void r_color::process(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    std::wstring w_mess = trim(string_to_wstring(message).substr(4));
    int color = 0;
    if(w_mess[0]==L'#'){
        for(int i=1;i<=6;i++){
            if(w_mess[i]>L'9'){
                if(w_mess[i]>L'Z'){
                    color = (color<<4) + w_mess[i] - L'a';
                } else {
                    color = (color<<4) + w_mess[i] - L'A';
                }
            } else {
                color = (color<<4) + w_mess[i] - L'0';
            }
        }
    } else {
        color = uni_dis_0_255(generator) * 65536
                + uni_dis_0_255(generator) * 256
                + uni_dis_0_255(generator);
    }
    color = color % 1677216;
    cv::Mat img = cv::Mat(256,256, CV_8UC3);
    img = cv::Scalar(color/65536,color/256%256,color%256);
    int cnt = 0;
    for(int i=0;i<25;i++){
        cnt+= (color&(1<<i)) ? 1 : 0;
    }

    int fontFace = cv::FONT_HERSHEY_TRIPLEX;
    double fontScale = 1;
    int thickness = 1;
    cv::String text = get_code(color);
    cv::cvtColor(img, img, cv::COLOR_RGB2BGR);

    // Get text size and baseline
    cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, nullptr);
    int baseline = (img.rows - textSize.height) / 2;

    // Center text horizontally and vertically
    cv::Point textOrg((img.cols - textSize.width) / 2, baseline + textSize.height);

    // Add text to image
    cv::putText(img, text, textOrg, fontFace, fontScale, cnt <= 12 ? cv::Scalar::all(255) : cv::Scalar::all(0), thickness, cv::LINE_8);

    cv::imwrite("./resource/temp/" + text + ".png", img);

    cq_send("[CQ:image,file=file://" + get_local_path() + "/resource/temp/%23" + text.substr(1) + ".png,id=40000]", message_type, user_id, group_id);
}
bool r_color::check(std::string message, std::string message_type, int64_t user_id, int64_t group_id){
    return string_to_wstring(message).find(L"来点色图") == 0;
}
std::string r_color::help(){
    return "来点色图：来点色图+#color_hex_code";
}