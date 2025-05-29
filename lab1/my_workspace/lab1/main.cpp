#include "io_bmp_new.h"
#include <iostream>
#include <vector>

using namespace io_bmp;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <in bmp file> <out bmp file>\n";
        return -1;
    }

    Error err_code = Error::None;
    try {
        // Open input BMP
        BmpIn in;
        if ((err_code = in.open(argv[1])) != Error::None)
            throw err_code;

        int width = in.get_width();
        int height = in.get_height();
        int components = in.get_num_components();

        std::vector<byte> image_data(width * height * components);
        std::vector<byte> line_buf;

        // Read pixel data line-by-line
        for (int i = height - 1; i >= 0; --i) {
            if ((err_code = in.get_line(line_buf)) != Error::None)
                throw err_code;
            std::copy(line_buf.begin(), line_buf.end(), image_data.begin() + i * width * components);
        }
        in.close();

        // Write output BMP
        BmpOut out;
        out.set_width(width);
        out.set_height(height);
        out.set_num_components(components);
        if ((err_code = out.open(argv[2])) != Error::None)
            throw err_code;

        for (int i = height - 1; i >= 0; --i) {
            std::vector<byte> line(image_data.begin() + i * width * components,
                image_data.begin() + (i + 1) * width * components);
            if ((err_code = out.put_line(line)) != Error::None)
                throw err_code;
        }
        out.close();
    }
    catch (Error err) {
        switch (err) {
        case Error::NoFile:
            std::cerr << "Cannot open supplied input or output file.\n";
            break;
        case Error::FileHeader:
            std::cerr << "Error encountered while parsing BMP file header.\n";
            break;
        case Error::Unsupported:
            std::cerr << "Unsupported BMP format. Only 8-bit and 24-bit supported.\n";
            break;
        case Error::FileTrunc:
            std::cerr << "File truncated unexpectedly.\n";
            break;
        case Error::FileNotOpen:
            std::cerr << "Tried to access a file that is not open.\n";
            break;
        default:
            std::cerr << "Unknown error.\n";
        }
        return -1;
    }

    return 0;
}
