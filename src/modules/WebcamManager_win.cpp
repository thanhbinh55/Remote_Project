#include "WebcamManager.hpp"
#include <windows.h>
#include <objidl.h> 
#include <gdiplus.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using namespace Gdiplus;

// --- HELPER: GDI+ Encoder (Static) ---
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT  num = 0;
    UINT  size = 0;
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

// --- HELPER: GDI+ Init ---
struct GdiPlusInitWebcam {
    ULONG_PTR gdiplusToken;
    GdiPlusInitWebcam() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~GdiPlusInitWebcam() {
        GdiplusShutdown(gdiplusToken);
    }
};
static GdiPlusInitWebcam init; 

// --- WEBCAM LOGIC ---

void WebcamManager::stop_stream() {
    running_ = false;
    if (stream_thread_.joinable()) {
        stream_thread_.join();
    }
}

void WebcamManager::start_stream(StreamCallback callback) {
    stop_stream(); 
    running_ = true;

    stream_thread_ = std::thread([this, callback]() {
        HRESULT hr = S_OK;
        
        hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) return;

        IMFAttributes* pAttributes = NULL;
        IMFActivate** ppDevices = NULL;
        UINT32 count = 0;
        IMFMediaSource* pSource = NULL;
        IMFSourceReader* pReader = NULL;

        // 1. Tìm thiết bị Webcam
        MFCreateAttributes(&pAttributes, 1);
        pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
        if (SUCCEEDED(hr) && count > 0) {
            hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
        }
        
        pAttributes->Release();
        for(UINT32 i = 0; i < count; i++) ppDevices[i]->Release();
        CoTaskMemFree(ppDevices);

        if (!pSource) {
            std::cerr << "[WEBCAM] No webcam found.\n";
            return;
        }

        // 2. [QUAN TRỌNG] Cấu hình Source Reader để bật Video Processing
        // Việc này bắt buộc để chuyển đổi YUV/MJPG sang RGB32
        IMFAttributes* pReaderAttributes = NULL;
        MFCreateAttributes(&pReaderAttributes, 1);
        pReaderAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

        // 3. Tạo Source Reader với cấu hình trên
        hr = MFCreateSourceReaderFromMediaSource(pSource, pReaderAttributes, &pReader);
        
        // Giải phóng attribute config ngay sau khi tạo xong Reader
        if (pReaderAttributes) pReaderAttributes->Release();

        if (FAILED(hr)) {
            std::cerr << "[WEBCAM] Failed to create Source Reader.\n";
            pSource->Release();
            return;
        }

        // 4. Ép kiểu sang RGB32 (Bây giờ sẽ thành công vì đã bật Video Processing)
        IMFMediaType* pType = NULL;
        MFCreateMediaType(&pType);
        pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32); 

        hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
        pType->Release();

        if (FAILED(hr)) {
            // Nếu vẫn lỗi, in ra mã lỗi chi tiết (Hex)
            std::cerr << "[WEBCAM] Failed to configure RGB32 format. Error: " << std::hex << hr << "\n";
            pSource->Release();
            pReader->Release();
            return;
        }

        CLSID jpgClsid;
        GetEncoderClsid(L"image/jpeg", &jpgClsid);
        EncoderParameters encoderParameters;
        encoderParameters.Count = 1;
        encoderParameters.Parameter[0].Guid = EncoderQuality;
        encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
        encoderParameters.Parameter[0].NumberOfValues = 1;
        ULONG quality = 50; 
        encoderParameters.Parameter[0].Value = &quality;

        std::cout << "[WEBCAM] Streaming started (Color Mode)...\n";

        while (running_) {
            IMFSample* pSample = NULL;
            DWORD streamIndex, flags;
            LONGLONG llTimeStamp;

            hr = pReader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0,
                &streamIndex,
                &flags,
                &llTimeStamp,
                &pSample
            );

            if (FAILED(hr)) break;

            if (pSample) {
                IMFMediaBuffer* pBuffer = NULL;
                pSample->ConvertToContiguousBuffer(&pBuffer);
                
                BYTE* pBitmapData = NULL;
                DWORD maxLength = 0, currentLength = 0;
                
                if (SUCCEEDED(pBuffer->Lock(&pBitmapData, &maxLength, &currentLength))) {
                    IMFMediaType* pCurrentType = NULL;
                    pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
                    UINT32 width = 0, height = 0;
                    MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &width, &height);
                    
                    // RGB32 Stride = Width * 4
                    Bitmap bmp(width, height, width * 4, PixelFormat32bppRGB, pBitmapData);

                    IStream* pStream = NULL;
                    if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
                        if (bmp.Save(pStream, &jpgClsid, &encoderParameters) == Ok) {
                            STATSTG stg;
                            pStream->Stat(&stg, STATFLAG_NONAME);
                            ULONG sz = stg.cbSize.LowPart;
                            std::vector<uint8_t> jpgData(sz);
                            LARGE_INTEGER seekPos = {0};
                            pStream->Seek(seekPos, STREAM_SEEK_SET, NULL);
                            ULONG bytesRead;
                            pStream->Read(jpgData.data(), sz, &bytesRead);

                            if (callback) callback(jpgData);
                        }
                        pStream->Release();
                    }
                    
                    pCurrentType->Release();
                    pBuffer->Unlock();
                }
                pBuffer->Release();
                pSample->Release();
            }

            // Giới hạn FPS khoảng 20-30 để không quá tải mạng
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }

        if(pSource) pSource->Release();
        if(pReader) pReader->Release();
        MFShutdown();
        std::cout << "[WEBCAM] Streaming stopped.\n";
    });
}