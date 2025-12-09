#include "ScreenManager.hpp"

// [FIX] Thêm thư viện này để định nghĩa IStream cho GDI+
// Bắt buộc phải có nếu dự án dùng WIN32_LEAN_AND_MEAN

#include <gdiplus.h>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <direct.h> // Để tạo thư mục (_mkdir)
// Link thư viện GDI+ (Chỉ hoạt động với MSVC, nếu dùng MinGW cần thêm trong CMakeLists.txt)
#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

// Helper để lấy Encoder ID cho JPEG
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT  num = 0; UINT  size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    auto pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}
// Hàm khởi tạo/hủy GDI+ (Singleton đơn giản)
struct GdiPlusInit {
    ULONG_PTR gdiplusToken;
    GdiPlusInit() { GdiplusStartupInput g; GdiplusStartup(&gdiplusToken, &g, NULL); }
    ~GdiPlusInit() { GdiplusShutdown(gdiplusToken); }
};
static GdiPlusInit init;

static void SaveDataToDisk(const std::vector<uint8_t>& data, const std::string& prefix) {
    // 1. Tạo thư mục logs nếu chưa có
    _mkdir("captured_data");
    
    // 2. Tạo tên file theo thời gian: captured_data/screen_20231208_103001.jpg
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << "captured_data/" << prefix << "_" 
        << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".jpg";
    
    std::string filename = oss.str();

    // 3. Ghi file
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        std::cout << "[STORAGE] Saved: " << filename << std::endl;
    }
}

// === CAPTURE SCREEN TO JPEG BUFFER ===
// === CAPTURE SCREEN ===
bool ScreenManager::capture_screen_data(std::vector<uint8_t>& out_buffer, std::string& error_msg) {
    error_msg.clear();
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY)) {
        error_msg = "BitBlt failed";
        return false;
    }

    Bitmap* bitmap = Bitmap::FromHBITMAP(hBitmap, NULL);
    IStream* stream = NULL;
    if (CreateStreamOnHGlobal(NULL, TRUE, &stream) != S_OK) {
        delete bitmap; return false;
    }

    CLSID jpgClsid;
    GetEncoderClsid(L"image/jpeg", &jpgClsid);
    EncoderParameters encoderParameters;
    encoderParameters.Count = 1;
    encoderParameters.Parameter[0].Guid = EncoderQuality;
    encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
    encoderParameters.Parameter[0].NumberOfValues = 1;
    ULONG quality = 60; 
    encoderParameters.Parameter[0].Value = &quality;

    Status stat = bitmap->Save(stream, &jpgClsid, &encoderParameters);
    
    if (stat == Ok) {
        STATSTG stg;
        stream->Stat(&stg, STATFLAG_NONAME);
        ULONG streamSize = stg.cbSize.LowPart;
        out_buffer.resize(streamSize);
        LARGE_INTEGER seekPos; seekPos.QuadPart = 0;
        stream->Seek(seekPos, STREAM_SEEK_SET, NULL);
        ULONG bytesRead;
        stream->Read(out_buffer.data(), streamSize, &bytesRead);

        // [MỚI] GỌI HÀM LƯU FILE KHI CHỤP THÀNH CÔNG
        SaveDataToDisk(out_buffer, "screen");
    } else {
        error_msg = "GDI+ Save Failed";
    }

    stream->Release();
    delete bitmap;
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return (stat == Ok);
}

json ScreenManager::handle_command(const json& request) {
    return { {"status", "ok"} };
}