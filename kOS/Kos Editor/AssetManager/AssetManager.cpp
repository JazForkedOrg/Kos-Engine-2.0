#include "AssetManager.h"
#include "DeSerialization/json_handler.h"
#include "Compilers/Compiler.h"
#include "Configs/ConfigPath.h"

#include "ECS/ECS.h"


std::shared_ptr<AssetManager> AssetManager::m_instancePtr = nullptr;

AssetManager::AssetManager()
{
    CompilerData Data = Serialization::ReadJsonFile<CompilerData>(configpath::configFilePath);

    Data.ApplyFunction([&](auto& member) {
        for (const auto& inputExtension : member.inputExtensions)
            m_compilerMap[inputExtension].emplace_back(CompilerD{ member.type, member.path, member.outputExtension });
        });

}

AssetManager::~AssetManager() {

}


void AssetManager::Init(const std::string& assetDirectory, const std::string& resourceDirectory)
{
	m_assetDirectory = assetDirectory;
    m_resourceDirectory = resourceDirectory;

    std::function<void(const std::string&)> readDirectory;

    readDirectory = [&](const std::string& Dir) {
        for (const auto& entry : std::filesystem::directory_iterator(Dir)) {
            std::string filepath = entry.path().string();

            if (entry.is_directory()) {
                readDirectory(filepath); 
            }
            else {
                RegisterAsset(entry.path());

                //test if resource is already in the resource folder
                std::filesystem::path metaPath = filepath + ".meta";


                if (std::filesystem::exists(metaPath)) {
                    AssetData data = Serialization::ReadJsonFile<AssetData>(metaPath.string());
                    const auto& map = m_compilerMap.at(entry.path().filename().extension().string());
                    for (const auto& compilerData : map)
                    {
                        std::string type = compilerData.type;
                        std::string outputResourcePath = std::filesystem::absolute(m_resourceDirectory).string() + "/" + type + "/" + data.GUID + compilerData.outputExtension;
                        if (!std::filesystem::exists(outputResourcePath)) {
                            Compilefile(filepath);
                        }

                    }
                }
                else {
                    return;
                }


            }
        }
        };


	readDirectory(m_assetDirectory);


    //Setup Watchers

    m_assetWatcher = std::make_unique<Watcher>(m_assetDirectory, std::chrono::milliseconds(1000));

    m_assetWatcher->SetCallback([&](ACTION action, const std::filesystem::path& filePath) {

        switch (action)
        {
		case ADDED:
            LOGGING_INFO("Watcher: Added - " + filePath.string());
            RegisterAsset(filePath);
            Compilefile(filePath);

			break;
		case MODIFIED:
            LOGGING_INFO("Watcher: Modified - " + filePath.string());
            Compilefile(filePath);

			break;
		case REMOVED:
            LOGGING_INFO("Watcher: Removed - " + filePath.string());
			break;
		default:
			break;
        }

       
        });

    //Ignore all .meta files
    m_assetWatcher->SetIgnoreExtension(".meta");

    m_assetWatcher->Start();



}



void AssetManager::RegisterAsset(const std::filesystem::path& filePath)
{
	std::string inputExtension = filePath.extension().string();
	//check if compiler have been registered 
	if (m_compilerMap.find(inputExtension) == m_compilerMap.end()) {
		//LOGGING_WARN("RegisterAsset: " + inputExtension + " not found");
		return;
	}
    
    const std::string& type = m_compilerMap.at(inputExtension).front().type;

    std::string GUID = m_dataBase.ImportAsset(filePath, type);

    m_GUIDtoFilePath[GUID] = filePath;
}

void AssetManager::Compilefile(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        LOGGING_WARN("Compile File: " + filePath.string() + " filepath does not exist");
        return;
    }

    std::string inputExtension = filePath.extension().string();
	//check if compiler have been registered
	if (m_compilerMap.find(inputExtension) == m_compilerMap.end()) {
		//LOGGING_WARN("Compile File: " + inputExtension + "not found");
		return;
		
        //just copy the file over to the resource

	}
    
	//check if file meta exist
	std::filesystem::path metaPath = filePath.string() + ".meta";

	if (!std::filesystem::exists(metaPath)) {
        //Import asset and create meta file
        RegisterAsset(filePath);

        if (!std::filesystem::exists(metaPath)) {
			LOGGING_WARN("Compile File: Meta file does not exist after registering asset");
			return;
        }
	}
    //get file type from meta
    AssetData data = Serialization::ReadJsonFile<AssetData>(metaPath.string());

    const auto& map = m_compilerMap.at(inputExtension);
    for(const auto& compilerData : map)
    {
        std::string type = compilerData.type;

        std::string guid = data.GUID;

        std::string inputPath = std::filesystem::absolute(filePath).string();
        std::string outputResourcePath = std::filesystem::absolute(m_resourceDirectory).string() + "/" + type + "/" + data.GUID + compilerData.outputExtension;

        //assets that have a compiler
        if (compilerData.compilerFilePath != "null")
        {
            //COMPILING CODE HERE
            std::string compilerPath = std::filesystem::absolute(compilerData.compilerFilePath).string();
            std::string absmetaPath = std::filesystem::absolute(metaPath).string();

            //std::string command = "\"" + compilerPath + "\" \"" + inputPath + "\" \"" + absmetaPath + "\" \"" + outputResourcePath + "\"";

            //std::cout << "Running: " << command << std::endl;

            std::string tempBatch = "temp_run_meshcompiler.bat";
            // Write the batch file
            std::ofstream batchFile(tempBatch);
            if (!batchFile) {
                std::cerr << "Failed to create batch file!" << std::endl;
                return;
            }

            batchFile << "@echo off\n";
            batchFile << "set EXE=\"" << compilerPath << "\"\n";
            batchFile << "%EXE% \"" << inputPath << "\" \"" << absmetaPath << "\" \"" << outputResourcePath << "\" \"" << "\"\n";
            batchFile.close();

            // Run the batch file
            auto runSystemAsync = [](const std::string& batfile)
                {
                    auto future = std::async(std::launch::async, [batfile]() {
                        int result = std::system(batfile.c_str());
                        if (result != 0) {
                            std::cerr << "Command failed with code: " << result << std::endl;
                        }

                        // Delete the temporary batch file
                        std::filesystem::remove(batfile);
                     });

                    return future; // you can store this if you want to wait later
                };

            runSystemAsync(tempBatch);


        }
        else {
            //assets without

            //check if directory exist, if not create one

            std::filesystem::path dir = std::filesystem::path(outputResourcePath).parent_path();

            if (!std::filesystem::exists(dir)) {
                if (!std::filesystem::create_directories(dir)) {
                    LOGGING_ERROR("Fail to create directory");
                }
            }


            std::filesystem::copy_file(inputPath, outputResourcePath, std::filesystem::copy_options::overwrite_existing);

        }

        return;
	}


    






}


