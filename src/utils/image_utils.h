#pragma once
#include <utility>
#include <string>
#include <Magick++.h>

/**
 * add random noise to a image
 */
void addRandomNoise(const std::string &filePath);

std::pair<std::string, std::string> image2base64(std::string filepath);

/// @brief Copy a image into another image.
///        Copy from src[x1\~x2][y1\~y2] to
///        dst[x3][y3](left-upper)
/// @param dst destination
/// @param src source
void copyImageTo(Magick::Image &dst, const Magick::Image src, size_t x1,
                 size_t x2, size_t y1, size_t y2, size_t x3, size_t y3);

/// @brief Mirror a given static image, with mirror axis and direction
/// @param img a Magick Image
/// @param axis only allow 0/1. 0 for x-axis, 1 for y-axis
/// @param direction 0/1. 0 for mirror left to right, 1 for reverse
void mirrorImage(Magick::Image &img, char axis = 1, bool direction = 0,
                 const Magick::Image las = Magick::Image(
                     Magick::Geometry(1, 1), Magick::Color("white")));

/// @brief Mirror a given gif image, with mirror axis and direction
/// @param img a array of Magick Image, gif
/// @param axis only allow 0/1. 0 for x-axis, 1 for y-axis
/// @param direction 0/1. 0 for mirror left to right, 1 for reverse
void mirrorImage(std::vector<Magick::Image> &img, char axis = 1,
                 bool direction = 0);

/// @brief generate a rotating gif from img.
/// @param img source image
/// @param fps frame per second
/// @param clockwise if rotate in clockwise
/// @return a sequence of gif image
std::vector<Magick::Image> rotateImage(const Magick::Image img, int fps,
                                       bool clockwise = 1);

void kaleido(Magick::Image &img, int layers = 3, int nums_per_layer = 8,
             const Magick::Image las = Magick::Image(Magick::Geometry(1, 1),
                                                     Magick::Color("white")));

void kaleido(std::vector<Magick::Image> &img, int layers = 3,
             int nums_per_layer = 8);
