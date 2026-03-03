#include "image_utils.h"
#include "utils.h"

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

            double randomValuer = (get_random(var) - (var >> 1)) / 255.0;
            double randomValueg = (get_random(var) - (var >> 1)) / 255.0;
            double randomValueb = (get_random(var) - (var >> 1)) / 255.0;
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

void copyImageTo(Magick::Image &dst, const Magick::Image src,
                 size_t src_row_start, size_t src_row_end,
                 size_t src_col_start, size_t src_col_end,
                 size_t dst_row, size_t dst_col)
{
    size_t crop_width = src_col_end - src_col_start;
    size_t crop_height = src_row_end - src_row_start;
    Magick::Geometry crop_region(crop_width, crop_height, src_col_start, src_row_start);
    Magick::Image cropped_src = src;
    cropped_src.crop(crop_region);
    dst.composite(cropped_src, dst_col, dst_row, MagickCore::CompositeOperator::OverCompositeOp);
}

void mirrorImage(Magick::Image &img, char axis, bool direction,
                 const Magick::Image las)
{
    Magick::Image new_img = img;

    // Mirror operation
    if (axis == 0) { // Vertical axis (flip)
        // Delete original half side first
        for (size_t y = (direction == 0 ? img.rows() >> 1 : 0);
             y < (direction == 0 ? img.rows() : img.rows() >> 1); ++y) {
            for (size_t x = 0; x < img.columns(); ++x) {
                img.pixelColor(x, y, Magick::Color(0, 0, 0, 0));
            }
        }
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
    else if (axis == 1) { // Horizontal axis (flop)
        // Delete original half side first
        for (size_t y = 0; y < img.rows(); ++y) {
            for (size_t x = (direction == 0 ? img.columns() >> 1 : 0);
                 x < (direction == 0 ? img.columns() : img.columns() >> 1); ++x) {
                img.pixelColor(x, y, Magick::Color(0, 0, 0, 0));
            }
        }
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
    if (las.columns() > 1) {
        Magick::Image ret_img = las;
        copyImageTo(ret_img, img, 0, img.rows(), 0, img.columns(), 0, 0);
        img = ret_img;
    }
    return;

mirrorImageError:
    std::ostringstream oss;
    oss << "Invalid parameter in " << __FUNCTION__ << ", axis= " << axis
        << ", direction= " << direction;
    set_global_log(LOG::ERROR, oss.str());
    throw "Invalid parameter, see more in log.";
}

void mirrorImage(std::vector<Magick::Image> &img, char axis, bool direction, std::function<void(float)> callback)
{
    std::vector<Magick::Image> coalesced;
    Magick::coalesceImages(&coalesced, img.begin(), img.end());

    for (Magick::Image &im : coalesced) {
        mirrorImage(im, axis, direction);
        
        if (callback != nullptr) {
            callback(1.0 / coalesced.size());
        }
    }
    img.clear();
    Magick::optimizeImageLayers(&img, coalesced.begin(), coalesced.end());
}

using namespace Magick;

void crop_to_square(Magick::Image &img)
{
    Magick::Geometry geo = Magick::Geometry(img.columns(), img.rows());
    size_t len = std::min(geo.width(), geo.height());
    geo.xOff((geo.width() - len) >> 1);
    geo.yOff((geo.height() - len) >> 1);
    geo.height(len);
    geo.width(len);
    img.crop(geo);
    img.page(Magick::Geometry(0, 0, 0, 0));
}

void crop_to_circle(Magick::Image &img)
{
    crop_to_square(img);
    size_t len = img.columns(), half_len = img.columns() >> 1;

    for (size_t y = 0; y < len; ++y) {
        for (size_t x = 0; x < len; ++x) {
            size_t xx = ((x > half_len) ? (x - half_len) : (half_len - x));
            size_t yy = ((y > half_len) ? (y - half_len) : (half_len - y));
            if (xx + yy > 1.42 * half_len ||
                xx * xx + yy * yy > half_len * half_len) {
                img.pixelColor(x, y, Magick::Color(0, 0, 0, 0));
            }
        }
    }
}

void constsize_rotate(Magick::Image &img, double deg)
{
    size_t oriWidth = img.columns(), oriHeight = img.rows();
    // img.backgroundColor(Magick::Color(5, 0, 0, 0));
    img.rotate(deg);
    img.page(Magick::Geometry(0, 0, 0, 0));
    size_t newWidth = img.columns(), newHeight = img.rows();

    Magick::Geometry geo =
        Magick::Geometry(oriWidth, oriHeight, (newWidth - oriWidth) >> 1,
                         (newHeight - oriHeight) >> 1);

    img.crop(geo);
    img.page(Magick::Geometry(0, 0, 0, 0));
    // img.write("test_cir_"+std::to_string(deg)+".png");
}

std::vector<Magick::Image> rotateImage(const Magick::Image img, int fps,
                                       bool clockwise, std::function<void(float)> callback)
{
    Magick::Image dimg = img;
    dimg.alphaChannel(MagickCore::AlphaChannelOption::SetAlphaChannel);

    double ratio = dimg.columns() * dimg.rows() / 1000000;
    if (ratio > 1) {
        dimg.resize(
            Magick::Geometry(dimg.columns() / ratio, dimg.rows() / ratio));
        dimg.page(Magick::Geometry(0, 0, 0, 0));
    }
    crop_to_square(dimg);

    std::vector<Magick::Image> ret;
    double deg_per_frame = 360.0 / fps * (clockwise ? 1 : -1);
    dimg.animationDelay(100 / fps);
    dimg.backgroundColor(Magick::Color(3, 5, 7, 0));
    for (int i = 0; i < fps; i++) {
        Magick::Image ximg = dimg;
        ximg.animationDelay(100 / fps);
        constsize_rotate(ximg, deg_per_frame * i);
        crop_to_circle(ximg);
        ret.push_back(ximg);
        if (callback != nullptr) {
            callback(1.0 / fps);
        }
    }
    return ret;
}

struct TileTransform
{
    double angle;
    double scale;
    bool flipX;
    bool flipY;
    int x;
    int y;
};


void buildRandomCanvas(
    const std::vector<Magick::Image>& input,
    std::vector<Magick::Image>& output,
    std::function<void(float)> callback = nullptr,
    double cover_rate = 0.98,
    double scaleFactor = 4.0)
{
    if (input.empty())
        return;

    // 处理 GIF offset / partial frame
    std::vector<Magick::Image> frames = input;
    Magick::coalesceImages(&frames, frames.begin(), frames.end());
    // 限制图片大小
    double resize_d = sqrt((frames[0].columns() * frames[0].rows() * frames.size()) / 1000000.0);
    for (auto& img : frames) {
        if (resize_d > 1.0) {
            img.resize(Magick::Geometry((size_t)(img.columns() / resize_d),
                                        (size_t)(img.rows() / resize_d)));
            img.page(Magick::Geometry(0, 0, 0, 0));
        }
    }

    int baseW = frames[0].columns();
    int baseH = frames[0].rows();

    int canvasW = int(baseW * scaleFactor);
    int canvasH = int(baseH * scaleFactor);

    double scale_fact = (canvasW * canvasH) / 2.0 / (baseW * baseH);
    int tiles = std::min(int(std::log(1.0 - cover_rate) / std::log(1.0 - 1.0 / scale_fact)), 60);

    // ---------- 关键：预生成随机布局 ----------
    std::vector<TileTransform> transforms;
    transforms.reserve(tiles);

    for (int i = 0; i < tiles; i++)
    {
        TileTransform t;
        t.angle = get_random_f(0.0, 360.0);
        t.scale = get_random_f(0.8, 1.2);
        t.flipX = get_random(2);
        t.flipY = get_random(2);

        t.x = get_random(-baseW, canvasW);
        t.y = get_random(-baseH + (canvasH >> 1), canvasH);

        transforms.push_back(t);
    }

    output.clear();
    output.reserve(frames.size());

    for (size_t fi = 0; fi < frames.size(); fi++)
    {
        const Magick::Image& src = frames[fi];

        Magick::Image canvas(Magick::Geometry(canvasW, canvasH), Magick::Color(0,0,0,0));
        canvas.alpha(true);

        for (const auto& t : transforms)
        {
            Magick::Image img = src;

            if (t.flipX) img.flop();
            if (t.flipY) img.flip();

            img.resize(Magick::Geometry(
                int(src.columns() * t.scale),
                int(src.rows() * t.scale)));

            img.backgroundColor(Magick::Color(0,0,0,0));
            img.rotate(t.angle);

            img.alpha(true);
            canvas.composite(img, t.x, t.y, MagickCore::CompositeOperator::OverCompositeOp);
            
        
            if (callback != nullptr) {
                callback(1.0 / (frames.size() * tiles) * 0.5); // 预生成布局进度，50%
            }
        }

        // 继承 GIF 参数
        canvas.animationDelay(src.animationDelay());
        canvas.animationIterations(src.animationIterations());

        output.push_back(canvas);
    }
}

Magick::Image buildRandomCanvas(const Image& input, std::function<void(float)> callback = nullptr, double cover_rate = 0.98, double scaleFactor = 4.0)
{
    int w = input.columns();
    int h = input.rows();
    double scale = std::min(500.0 / w, 500.0 / h);
    Magick::Image input_scaled = input;
    if (scale < 1.0) {
        input_scaled.resize(Magick::Geometry(int(w * scale), int(h * scale)));
    }

    int canvasW = int(w * scaleFactor);
    int canvasH = int(h * scaleFactor);

    canvasW = std::max(canvasW, canvasH);
    canvasH = canvasW;

    Magick::Image canvas(Magick::Geometry(canvasW, canvasH), Magick::Color(0,0,0,0));
    canvas.alpha(true);

    double scale_fact = (canvasW * canvasH) / 2.0 / (w * h);
    int tiles = std::min(int(std::log(1.0 - cover_rate) / std::log(1.0 - 1.0 / scale_fact)), 60);

    for (int i = 0; i < tiles; i++)
    {
        Magick::Image img = input_scaled;

        // 随机翻转
        if (get_random(2)) img.flop();
        if (get_random(2)) img.flip();

        // 随机缩放
        double scale = get_random_f(0.8, 1.2);
        img.resize(Magick::Geometry(int(w * scale), int(h * scale)));

        // 旋转
        double angle = get_random_f(0.0, 360.0);
        img.backgroundColor(Magick::Color(0,0,0,0));
        img.rotate(angle);

        // alpha
        img.alpha(true);

        // 随机位置
        int x = get_random(-w, canvasW);
        int y = get_random(-h + (canvasH >> 1), canvasH);

        canvas.composite(img, x, y, MagickCore::CompositeOperator::OverCompositeOp);

        if (callback != nullptr) {
            callback(1.0 / tiles * 0.5); // 生成进度，50%
        }
    }

    return canvas;
}

Magick::Image kaleidoscopeSectorSymmetry(const Magick::Image& canvas, int sectors = 12)
{
    int w = canvas.columns();
    int h = canvas.rows();

    int size = std::min(w, h);

    double cx = size / 2.0;
    double cy = size / 2.0;

    double sectorAngle = 360.0 / sectors;

    // 构造扇形 mask
    Magick::Image mask(Magick::Geometry(size, size), Magick::Color(0, 0, 0, 0));
    mask.alpha(true);

    std::vector<Magick::Drawable> drawList;
    drawList.push_back(Magick::DrawableFillColor(Magick::Color(0, 0, 0)));

    std::vector<Magick::Coordinate> poly;
    poly.emplace_back(cx, cy);

    double r = size / 2.0;

    double a0 = 0.0;
    double a1 = sectorAngle;

    double step = 1.0;  // 每 1 度采样
    for (double a = a0; a <= a1; a += step)
    {
        double rad = a * M_PI / 180.0;
        poly.emplace_back(
            cx + r * cos(rad),
            cy + r * sin(rad)
        );
    }

    drawList.push_back(Magick::DrawablePolygon(poly));
    mask.draw(drawList);

    Magick::Image sector = canvas;
    sector.crop(Magick::Geometry(size, size, (w - size)/2, (h - size)/2));
    sector.composite(mask, 0, 0, MagickCore::DstInCompositeOp);

    // 输出画布
    Magick::Image output(Magick::Geometry(size, size), Magick::Color(0, 0, 0, 0));
    output.alpha(true);
    for (int i = 0; i < sectors; i++)
    {
        Magick::Image piece = sector;
        piece.backgroundColor(Magick::Color(0,0,0,0));
        piece.page(Magick::Geometry(0,0,0,0));

        if (i % 2 == 1) {
            piece.flip();
            piece.page(Magick::Geometry(0,0,0,0));
            piece.rotate((i + 1) * sectorAngle);
        } else {
            piece.page(Magick::Geometry(0,0,0,0));
            piece.rotate(i * sectorAngle);
        }
        piece.page(Magick::Geometry(0,0,0,0));

        // 裁剪回原图片大小，以图片正中心为标准
        int crop_x = (piece.columns() - size) / 2;
        int crop_y = (piece.rows() - size) / 2;
        piece.crop(Magick::Geometry(size, size, crop_x, crop_y));
        piece.page(Magick::Geometry(0, 0, 0, 0));

        output.composite(piece, 0, 0, MagickCore::CompositeOperator::OverCompositeOp);
    }

    return output;
}

void kaleido(Magick::Image &img, int layers, int nums_per_layer,
             std::function<void(float)> callback)
{
    img = kaleidoscopeSectorSymmetry(buildRandomCanvas(img, callback), nums_per_layer);
}

void kaleido(std::vector<Magick::Image> &img, int layers, int nums_per_layer, std::function<void(float)> callback)
{
    std::vector<Magick::Image> coalesced;
    Magick::coalesceImages(&coalesced, img.begin(), img.end());

    std::vector<Magick::Image> coalesced_canvas;
    buildRandomCanvas(coalesced, coalesced_canvas, callback);

    for (Magick::Image &im : coalesced_canvas) {
        im.backgroundColor(Magick::Color(0, 0, 0, 0));
        im.page(Magick::Geometry(0, 0, 0, 0));

        size_t delay = im.animationDelay();
        size_t dispose = im.animationIterations();
        im = kaleidoscopeSectorSymmetry(im, nums_per_layer);
        im.animationDelay(delay);
        im.animationIterations(dispose);
        if (callback != nullptr) {
            callback(1.0 / coalesced_canvas.size() * 0.5); // kaleido 进度，50%
        }
    }

    img.clear();
    Magick::optimizeImageLayers(&img, coalesced_canvas.begin(), coalesced_canvas.end());
}