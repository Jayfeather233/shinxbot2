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
    p /= fileName;
    // std::cout<<p.string()<<std::endl;
    std::string command = "curl -o " + p.string() + " ";
    if(!proxy) command += "--noproxy '*' ";
    command += httpAddress + " > /dev/null 2>&1";
    system(command.c_str());
}

void addRandomNoise(const std::string &filePath)
{
    int pid = fork();
    if (pid < 0) {
        throw "AddRandomNoise: fork failed.";
    }
    else if (pid == 0) {
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
        exit(0);
    }
    else {
        waitpid(pid, 0, 0);
    }
}
