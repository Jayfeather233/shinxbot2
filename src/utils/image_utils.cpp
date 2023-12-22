#include "utils.h"

#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <Magick++.h>

void download(const std::string &httpAddress, const std::string &filePath,
              const std::string &fileName, const bool proxy)
{
    try {
        std::string data = do_get(httpAddress, {}, proxy);
        std::fstream ofile;
        try {
            ofile = openfile(filePath + "/" + fileName,
                             std::ios::out | std::ios::binary);
        }
        catch (...) {
        }
        ofile << data;
        ofile.flush();
        ofile.close();
        // writefile(filePath + "/" + fileName, data);
        // std::ofstream out(, std::ios::out | std::ios::binary);
        // out.write(data.c_str(), data.size());
        // out.close();
    }
    catch (const std::exception &e) {
        set_global_log(LOG::ERROR, "At download from" + httpAddress + " to " +
                               filePath + "." + fileName +
                               ", Exception occurred: " + e.what());
    }
    // std::filesystem::path p(filePath);
    // if (!std::filesystem::exists(p)) {
    //     std::filesystem::create_directories(p);
    // }
    // p /= fileName;
    // // std::cout<<p.string()<<std::endl;
    // std::string command = "curl -o " + p.string() + " ";
    // if(!proxy) command += "--noproxy '*' ";
    // command += httpAddress + " > /dev/null 2>&1";
    // int ret = system(command.c_str());
    // if(ret) {
    //     std::ostringstream oss;
    //     oss << "download " << httpAddress << " to " << filePath << "/" << fileName << " Proxy=" << proxy << "failed.";
    //     set_global_log(LOG::ERROR, oss.str());
    // }
}

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

std::pair<std::string, std::string> image2base64(std::string filepath){
    Magick::Image img(filepath);
    Magick::Blob blob;
    img.write(&blob);
    std::string type = img.magick();
    if(type == "PNG"){
        type = "image/png";
    } else if(type == "JPEG"){
        type = "image/jpeg";
    } else if(type == "WEBP"){
        type = "image/webp";
    } else if(type == "HEIC"){
        type = "image/heic";
    } else if(type == "HEIF"){
        type = "image/heif";
    } else {
        type = "image/error";
    }
    return std::make_pair(type, blob.base64());
}