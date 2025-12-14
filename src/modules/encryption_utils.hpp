#pragma once
#include <vector>
#include <string>

namespace EncryptionUtils {
    
    // DPAPI Functions
    std::vector<unsigned char> decryptDPAPIWin(const std::vector<unsigned char>& encrypted);
    
    // AES-GCM Functions using OpenSSL
    std::vector<unsigned char> decryptAESGCMOpenSSL(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv);
    
    // Base64 Functions
    std::vector<unsigned char> base64DecodeWin(const std::string& encoded);
    std::string base64EncodeWin(const unsigned char* data, size_t length);
    
    // Key Extraction
    std::string extractMasterKeyFromLocalState(const std::string& localStatePath);
    
} // namespace EncryptionUtils