#include "utils.h"

#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>

void download(const std::string &httpAddress, const std::string &filePath,
              const std::string &fileName, const bool proxy)
{
    // try {
    //     std::string data = do_get(httpAddress, {}, proxy);
    //     std::fstream ofile;
    //     try {
    //         ofile = openfile(filePath + "/" + fileName,
    //                          std::ios::out | std::ios::binary);
    //     }
    //     catch (...) {
    //     }
    //     ofile << data;
    //     ofile.flush();
    //     ofile.close();
    //     // writefile(filePath + "/" + fileName, data);
    //     // std::ofstream out(, std::ios::out | std::ios::binary);
    //     // out.write(data.c_str(), data.size());
    //     // out.close();
    // }
    // catch (const std::exception &e) {
    //     setlog(LOG::ERROR, "At download from" + httpAddress + " to " +
    //                            filePath + "." + fileName +
    //                            ", Exception occurred: " + e.what());
    // }
    std::filesystem::path p(filePath);
    if (!std::filesystem::exists(p)) {
        std::filesystem::create_directories(p);
    }
    p /= fileName;
    // std::cout<<p.string()<<std::endl;
    std::string command = "curl -o " + p.string() + " ";
    if(!proxy) command += "--noproxy '*' ";
    command += httpAddress + " > /dev/null 2>&1";
    int ret = system(command.c_str());
    if(ret) {
        std::ostringstream oss;
        oss << "download " << httpAddress << " to " << filePath << "/" << fileName << " Proxy=" << proxy << "failed.";
        set_global_log(LOG::ERROR, oss.str());
    }
}

// void addRandomNoiseSingle(CImg<unsigned char> &image) {
//     int w = image.width();
//     int h = image.height();
//     if ((int64_t)w * h > 4000000) {
//         double resize_d = sqrt((double)w * h / 4000000.0);
//         image.resize((size_t)(w / resize_d), (size_t)(h / resize_d));
//         w = image.width();
//         h = image.height();
//     }
//     int channels = image.spectrum();
//     for (int i = 0; i < w; i++) {
//         for (int j = 0; j < h; j++) {
//             for (int k = 0; k < channels; k++) {
//                 int value = image(i, j, 0, k) + get_random(128) - 2;
//                 value = std::max(0, std::min(value, 255));
//                 image(i, j, 0, k) = static_cast<unsigned char>(value);
//             }
//         }
//     }
// }
// void addRandomNoise(const std::string &filePath)
// {
//     int pid = fork();
//     if (pid < 0) {
//         throw "AddRandomNoise: fork failed.";
//     }
//     else if (pid == 0) {
//         if(filePath.find(".gif") != filePath.npos) {

//             // TODO: process gif file

//         } else {
//             CImg<unsigned char> image(filePath.c_str());
//             addRandomNoiseSingle(image);
//             image.save(filePath.c_str());
//         }
//         exit(0);
//     }
//     else {
//         waitpid(pid, 0, 0);
//     }
// }

void addRandomNoiseSingle(Magick::Image &img) {
    size_t width = img.columns();
    size_t height = img.rows();
    if (width * height > 4000000) {
        double resize_d = sqrt((double)width * height / 4000000.0);
        img.resize(Magick::Geometry((size_t)(width / resize_d), (size_t)(height / resize_d)));
        width = img.columns();
        height = img.rows();
    }

    // Loop through each pixel and add a random value
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            Magick::ColorRGB pixel = img.pixelColor(x, y);

            const int var = 8;

            double randomValuer = (get_random(var)-(var>>1))*1.0/256;
            double randomValueg = (get_random(var)-(var>>1))*1.0/256;
            double randomValueb = (get_random(var)-(var>>1))*1.0/256;
            pixel.red(std::max(std::min(pixel.red() + randomValuer, 1.0), 0.0));
            pixel.green(std::max(std::min(pixel.green() + randomValueg, 1.0), 0.0));
            pixel.blue(std::max(std::min(pixel.blue() + randomValueb, 1.0), 0.0));

            img.pixelColor(x, y, pixel);
        }
    }
}

void addRandomNoise(const std::string &filePath){
    try{
        if(filePath.find(".gif") != filePath.npos) {
            std::vector<Magick::Image> img;
            Magick::readImages(&img, filePath);
            size_t numFrames = img.size();
            std::vector<Magick::Image> result;
            int frameInterval = 1;
            for(size_t i = 0; i < numFrames; ++i){
                Magick::Image frame = img[i];
                if( i % frameInterval == 0) {
                    addRandomNoiseSingle(frame);
                }
                result.push_back(frame);
            }
            Magick::writeImages(result.begin(), result.end(), filePath);
        } else {
            Magick::Image img(filePath);
            addRandomNoiseSingle(img);
            img.write(filePath);
        }
    } catch (std::exception &e){
        set_global_log(LOG::ERROR, e.what());
    }
}