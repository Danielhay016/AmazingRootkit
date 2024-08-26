#include "../Agent/BaseModule.h"
#include <minizip/zip.h>

class FileGrabber : public BaseModule
{
private:
    void module_impl() override;
    std::vector<std::string> list_files(const std::string working_dir, const std::string& root, const bool& recursive, const std::string& filter, const bool& regularFilesOnly);
    std::string create_working_dir(std::string base_dir);
    std::string replace_slashes_with_underscores(const std::string& path);
    void copy_file_to_folder(const std::string& source_path, const std::string& destination_folder);
    bool zip_file(zipFile zf, const std::string& file_path, const std::string& zip_entry_name);
    bool zip_folder(const std::string& folder_path, const std::string& zip_file_path);
    int run_grab_task(std::string working_dir, std::string directory, std::string filter, std::string task_id);
    int parse_tasks(std::string working_dir, nlohmann::json j);
    std::vector<unsigned char> readFile(const std::string& filepath);
    void send_err(json ret_json, std::string str);

public:
    FileGrabber() = delete;
    FileGrabber(const FileGrabber &) = delete;

    FileGrabber(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
    };

    ~FileGrabber();
};
