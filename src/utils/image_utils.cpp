#include "utils.h"

#include <Magick++.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>

void command_download(const std::string &httpAddress,
                      const std::string &filePath, const std::string &fileName,
                      const bool proxy)
{
    std::filesystem::path p(filePath);
    if (!std::filesystem::exists(p)) {
        std::filesystem::create_directories(p);
    }
    p /= fileName;
    // std::cout<<p.string()<<std::endl;
    std::string command = "curl -o " + p.string() + " ";
    if (!proxy)
        command += "--noproxy '*' ";
    command += httpAddress + " > /dev/null 2>&1";
    int ret = system(command.c_str());
    if (ret) {
        std::ostringstream oss;
        oss << "download " << httpAddress << " to " << filePath << "/"
            << fileName << " Proxy=" << proxy << "failed.";
        set_global_log(LOG::ERROR, oss.str());
    }
}

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
            set_global_log(LOG::ERROR, "Cannot open file " + filePath + "/" +
                                           fileName + " for download.");
            return;
        }
        ofile << data;
        ofile.flush();
        ofile.close();
    }
    catch (const std::exception &e) {
        set_global_log(LOG::ERROR, "At download from" + httpAddress + " to " +
                                       filePath + "." + fileName +
                                       ", Exception occurred: " + e.what());
    }
}

void addRandomNoiseSingle(Magick::Image &img, int strength = 4)
{
    size_t width = img.columns();
    size_t height = img.rows();
    if (width * height > 4000000) {
        double resize_d = sqrt((double)width * height / 4000000.0);
        img.resize(Magick::Geometry((size_t)(width / resize_d),
                                    (size_t)(height / resize_d)));
        width = img.columns();
        height = img.rows();
    }

    // Loop through each pixel and add a random value
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            Magick::ColorRGB pixel = img.pixelColor(x, y);

            const int var = strength << 1;

            double randomValuer = (get_random(var) - (var >> 1)) / 256.0;
            double randomValueg = (get_random(var) - (var >> 1)) / 256.0;
            double randomValueb = (get_random(var) - (var >> 1)) / 256.0;
            pixel.red(std::max(std::min(pixel.red() + randomValuer, 1.0), 0.0));
            pixel.green(
                std::max(std::min(pixel.green() + randomValueg, 1.0), 0.0));
            pixel.blue(
                std::max(std::min(pixel.blue() + randomValueb, 1.0), 0.0));

            img.pixelColor(x, y, pixel);
        }
    }
}

void addRandomNoise(const std::string &filePath)
{
    try {
        if (filePath.find(".gif") != filePath.npos) {
            std::vector<Magick::Image> img;
            Magick::readImages(&img, filePath);
            size_t numFrames = img.size();
            std::vector<Magick::Image> result;
            int frameInterval = 1;
            for (size_t i = 0; i < numFrames; ++i) {
                Magick::Image frame = img[i];
                if (i % frameInterval == 0) {
                    addRandomNoiseSingle(frame);
                }
                result.push_back(frame);
            }
            Magick::writeImages(result.begin(), result.end(), filePath);
        }
        else {
            Magick::Image img(filePath);
            addRandomNoiseSingle(img);
            img.write(filePath);
        }
    }
    catch (std::exception &e) {
        set_global_log(LOG::ERROR, e.what());
    }
}

std::pair<std::string, std::string> image2base64(std::string filepath)
{
    Magick::Image img(filepath);
    img.thumbnail(Magick::Geometry(2048, 2048));

    Magick::Blob blob;
    img.write(&blob);
    std::string type = img.magick();
    if (type == "PNG") {
        type = "image/png";
    }
    else if (type == "JPEG") {
        type = "image/jpeg";
    }
    else if (type == "WEBP") {
        type = "image/webp";
    }
    else if (type == "HEIC") {
        type = "image/heic";
    }
    else if (type == "HEIF") {
        type = "image/heif";
    }
    else {
        type = "image/error";
    }
    return std::make_pair(type, blob.base64());
}

// TODO: mirror [axis]
//      Kaleido_scope WanHuaTong
//      rotate

void copyImageTo(Magick::Image &dst, const Magick::Image src, size_t x1,
                 size_t x2, size_t y1, size_t y2, size_t x3, size_t y3)
{
    Magick::Geometry gem = Magick::Geometry(y2 - y1, x2 - x1, y1, x1);
    Magick::Image subimg = src;
    subimg.crop(gem);
    dst.composite(subimg, y3, x3,
                  MagickCore::CompositeOperator::CopyCompositeOp);
}

void mirrorImage(Magick::Image &img, char axis, bool direction)
{
    Magick::Image new_img = img;
    if (axis == 0) {
        new_img.flip();
        if (direction == 0) {
            copyImageTo(img, new_img, new_img.rows() >> 1, new_img.rows(), 0,
                        new_img.columns(), img.rows() >> 1, 0);
        }
        else if (direction == 1) {
            copyImageTo(img, new_img, 0, new_img.rows() >> 1, 0,
                        new_img.columns(), 0, 0);
        }
        else {
            goto mirrorImageError;
        }
    }
    else if (axis == 1) {
        new_img.flop();
        if (direction == 0) {
            copyImageTo(img, new_img, 0, new_img.rows(), new_img.columns() >> 1,
                        new_img.columns(), 0, img.columns() >> 1);
        }
        else if (direction == 1) {
            copyImageTo(img, new_img, 0, new_img.rows(), 0,
                        new_img.columns() >> 1, 0, 0);
        }
        else {
            goto mirrorImageError;
        }
    }
    else {
        goto mirrorImageError;
    }
    return;
mirrorImageError:
    std::ostringstream oss;
    oss << "Invalid parameter in " << __FUNCTION__ << ", axis= " << axis
        << ", direction= " << direction;
    set_global_log(LOG::ERROR, oss.str());
    throw "Invalid parameter, see more in log.";
}

void mirrorImage(std::vector<Magick::Image> &img, char axis, bool direction)
{
    for (Magick::Image &im : img) {
        mirrorImage(im, axis, direction);
    }
}

std::vector<Magick::Image> rotateImage(const Magick::Image img, int fps,
                                       bool clockwise)
{
    Magick::Image dimg = img;
    std::vector<Magick::Image> ret;
    double deg_per_frame = 360.0 / fps * (clockwise ? 1 : -1);
    for (int i = 0; i < fps; i++) {
        ret.push_back(dimg);
        dimg.rotate(deg_per_frame); // TODO: FIXME: do not change iamge size
    }
    return ret;
}