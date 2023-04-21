#include "utils.h"

#include <fstream>
#include <iostream>
#include <string>

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
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
}

void addRandomNoise(const std::string &filePath)
{
    CImg<unsigned char> image(filePath.c_str());
    int w = image.width();
    int h = image.height();
    if ((int64_t)w * h > 4000000) {
        double resize_d = sqrt((double)w * h / 4000000.0);
        image.resize((size_t)(w / resize_d), (size_t)(h / resize_d));
        w = image.width();
        h = image.height();
    }
    int channels = image.spectrum();
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            for (int k = 0; k < channels; k++) {
                int value = image(i, j, 0, k) + get_random(4) - 2;
                value = std::max(0, std::min(value, 255));
                image(i, j, 0, k) = static_cast<unsigned char>(value);
            }
        }
    }
    image.save(filePath.c_str());
}
