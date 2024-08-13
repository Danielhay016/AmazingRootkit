#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include "../include/json.hpp"
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include "FileGrabber.h"

// TODO: implement all the functions in the header.

void FileGrabber::module_impl()
{
    std::cout << args.dump() << std::endl;
}