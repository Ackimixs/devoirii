#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include "quadtree.h"
#include "stb_image.h"
#include "stb_image_write.h"

namespace fs = std::filesystem;

struct Color {
    int r, g, b;

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
};

class Image {
public:
    std::vector<std::vector<Color>> data;
    int w_;
    int h_;

    Image() : w_(0), h_(0) {}

    Image(int width, int height)
        : data(height, std::vector<Color>(width)), w_(width), h_(height) {}

    int width() const { return data.empty() ? 0 : data[0].size(); }
    int height() const { return data.size(); }

    Color& at(int x, int y) { return data[y][x]; }
    const Color& at(int x, int y) const { return data[y][x]; }

    Image Resize(int w, int h) const {
        Image trimmed(w, h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (x < w_ && y < h_)
                    trimmed.at(x, y) = this->at(x, y);
                else
                    trimmed.at(x, y) = {0, 0, 0}; // Fill with black if out of bounds
        return trimmed;
    }
};

bool IsPowerOfTwo(int x) {
    return x > 0 && (x & (x - 1)) == 0;
}

bool IsValidImageSize(const Image& img) {
    return img.width() == img.height() && IsPowerOfTwo(img.width());
}

Image PadToSquare(const Image& input) {
    int h = input.height();
    int w = input.width();
    int size = 1;
    while (size < std::max(w, h)) size *= 2;

    Image padded(size, size);
    padded.w_ = w;
    padded.h_ = h;

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            padded.at(x, y) = input.at(x, y);

    return padded;
}

Image ReadImage(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
    if (!data) throw std::runtime_error("Failed to load image: " + filename);

    Image img(width, height);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 3;
            img.at(x, y) = {data[i], data[i+1], data[i+2]};
        }
    stbi_image_free(data);
    return img;
}

void WriteImage(const std::string& filename, const Image& img) {
    int width = img.width();
    int height = img.height();
    std::vector<unsigned char> data(width * height * 3);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 3;
            data[i] = img.at(x, y).r;
            data[i + 1] = img.at(x, y).g;
            data[i + 2] = img.at(x, y).b;
        }
    stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}

bool isUniform(const std::vector<std::vector<Color>>& img, int x, int y, int size, int tolerance = 10) {
    const Color& ref = img[y][x];
    for (int j = y; j < y + size; ++j)
        for (int i = x; i < x + size; ++i) {
            const Color& c = img[j][i];
            int dr = ref.r - c.r;
            int dg = ref.g - c.g;
            int db = ref.b - c.b;
            if (dr * dr + dg * dg + db * db > tolerance * tolerance)
                return false;
        }
    return true;
}

QuadTree<Color>* Encode(const std::vector<std::vector<Color>>& img, int x, int y, int size) {
    if (isUniform(img, x, y, size))
        return new QuadLeaf<Color>(img[y][x]);

    int half = size / 2;
    return new QuadNode<Color>(
        Encode(img, x, y, half),
        Encode(img, x + half, y, half),
        Encode(img, x + half, y + half, half),
        Encode(img, x, y + half, half)
    );
}

void Decode(std::vector<std::vector<Color>>& img, QuadTree<Color>* node, int x, int y, int size) {
    if (node->isLeaf()) {
        Color c = node->value();
        for (int j = y; j < y + size; ++j)
            for (int i = x; i < x + size; ++i)
                img[j][i] = c;
    } else {
        int half = size / 2;
        Decode(img, node->son(NW), x, y, half);
        Decode(img, node->son(NE), x + half, y, half);
        Decode(img, node->son(SE), x + half, y + half, half);
        Decode(img, node->son(SW), x, y + half, half);
    }
}

void ProcessImg(const std::string& in, const std::string& out)
{
    auto ext = fs::path(in).extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
        std::cout << "Processing: " << in << std::endl;
        Image img = ReadImage(in);
        int originalW = img.width();
        int originalH = img.height();
        bool sizeChanged = false;
        if (!IsValidImageSize(img)) {
            img = PadToSquare(img);
            sizeChanged = true;
        }

        QuadTree<Color>* qt = Encode(img.data, 0, 0, img.height());

        Image decoded(img.height(), img.height());
        Decode(decoded.data, qt, 0, 0, decoded.height());

        if (sizeChanged) {
            decoded = decoded.Resize(originalW, originalH);
        }

        WriteImage(out, decoded);

        delete qt;
    }
}

void ProcessDir(const std::string& in, const std::string& out)
{
    fs::create_directories(out);

    for (const auto& entry : fs::directory_iterator(in)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();

        std::string outFilename = out + "/" + entry.path().stem().string() + "_decoded.png";

        ProcessImg(path, outFilename);
    }
}

int main() {
    ProcessDir("Images", "out");

    //ProcessImg("../../img/Images/chat.png", "test/chat_decoded.png");

    return 0;
}