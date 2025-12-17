# cmake_minimum_required(VERSION 3.15)
# project(RemoteControlTool LANGUAGES C CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# # ------------------------------------------------------------------------------
# # 1. CẤU HÌNH STATIC RUNTIME (QUAN TRỌNG ĐỂ KHÔNG CẦN CÁC FILE DLL HỆ THỐNG)
# # ------------------------------------------------------------------------------
# if(MSVC)
#     # Ép sử dụng Static Runtime (/MT) cho cả Debug và Release
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#     add_compile_options($<$<CONFIG:Debug>:/MTd> $<$<CONFIG:Release>:/MT>)
    
#     # Tắt các cảnh báo không cần thiết
#     add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
# endif()

# # ------------------------------------------------------------------------------
# # 2. TỰ ĐỘNG TẢI VÀ NHÚNG SQLITE (ĐỂ KHÔNG CẦN SQLITE3.DLL)
# # ------------------------------------------------------------------------------
# message(STATUS "Dang cau hinh SQLite3 che do Static (Embedded)...")

# include(FetchContent)
# FetchContent_Declare(
#     sqlite3_src
#     # Link tải mã nguồn "Amalgamation" (chỉ gồm 1 file .c và 1 file .h)
#     URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
#     DOWNLOAD_EXTRACT_TIMESTAMP TRUE
# )

# FetchContent_MakeAvailable(sqlite3_src)

# # Tạo thư viện tĩnh từ file source vừa tải về
# # Lưu ý: Biến ${sqlite3_src_SOURCE_DIR} được tạo ra tự động bởi FetchContent
# add_library(sqlite3_static STATIC "${sqlite3_src_SOURCE_DIR}/sqlite3.c")

# # Cấu hình include folder
# target_include_directories(sqlite3_static PUBLIC "${sqlite3_src_SOURCE_DIR}")

# # Các định nghĩa cần thiết để SQLite chạy mượt trong môi trường đa luồng
# target_compile_definitions(sqlite3_static PRIVATE 
#     SQLITE_THREADSAFE=1 
#     SQLITE_OMIT_LOAD_EXTENSION=1
# )

# if(MSVC)
#     # Tắt cảnh báo khi compile code C của SQLite (để log sạch sẽ hơn)
#     target_compile_options(sqlite3_static PRIVATE /W0)
# endif()

# # ------------------------------------------------------------------------------
# # 3. CÁC THƯ VIỆN KHÁC
# # ------------------------------------------------------------------------------
# # Tìm nlohmann_json (Header only nên không lo về DLL)
# find_package(nlohmann_json CONFIG REQUIRED)

# # Cấu hình Boost Static (để không cần Boost DLL)
# set(Boost_USE_STATIC_LIBS ON) 
# find_package(Boost REQUIRED COMPONENTS system)

# # Tìm OpenSSL (Cố gắng tìm static nếu có thể, nếu không bạn phải copy libcrypto/libssl.dll)
# # Mẹo: Nếu vcpkg cài dynamic, bạn có thể cần copy 2 file dll của OpenSSL đi kèm.
# find_package(OpenSSL REQUIRED)

# if(WIN32)
#     set(OS_SRCS
#         src/modules/ProcessManager_win.cpp
#         src/modules/SystemManager_win.cpp
#         src/modules/ScreenManager_win.cpp
#         src/modules/AppManager_win.cpp
#         src/modules/KeyManager_win.cpp
#         src/modules/WebcamManager_win.cpp
#         src/modules/FileManager_win.cpp
#         src/modules/InputManager_win.cpp
#         src/modules/EdgeManager_win.cpp
#     )
#     # Các thư viện hệ thống Windows
#     set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus crypt32 wininet)

# # === [THÊM PHẦN NÀY CHO LINUX] ===
# elseif(UNIX)
#     set(OS_SRCS
#         src/modules/ProcessManager_linux.cpp
#         src/modules/SystemManager_linux.cpp
#         src/modules/ScreenManager_linux.cpp
#         src/modules/AppManager_linux.cpp
#         src/modules/KeyManager_linux.cpp
#         src/modules/WebcamManager_linux.cpp
#         src/modules/FileManager_linux.cpp
#         src/modules/InputManager_linux.cpp
#         src/modules/EdgeManager_linux.cpp
#     )
    
#     # Thư viện Linux cần thiết
#     # pthread: Cho đa luồng (std::thread)
#     # dl: Cho dynamic loading
#     # X11, Xtst: Thường cần cho ScreenManager (chụp màn hình) và InputManager (chuột/phím)
    
#     find_package(JPEG REQUIRED) 
#     set(OS_LIBS pthread dl X11 Xtst JPEG::JPEG)
# endif()

# # ------------------------------------------------------------------------------
# # 5. TẠO FILE EXE VÀ LIÊN KẾT (LINKING)
# # ------------------------------------------------------------------------------
# add_executable(server main.cpp ${OS_SRCS})

# target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# target_link_libraries(server PRIVATE 
#     sqlite3_static
#     Boost::system 
#     nlohmann_json::nlohmann_json 
#     OpenSSL::Crypto
#     OpenSSL::SSL
#     ${OS_LIBS} # Biến này giờ sẽ chứa thư viện Windows hoặc Linux tùy môi trường
# )

cmake_minimum_required(VERSION 3.15)
project(RemoteControlTool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# 1. CẤU HÌNH STATIC RUNTIME (GIỮ NGUYÊN)
# ------------------------------------------------------------------------------
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options($<$<CONFIG:Debug>:/MTd> $<$<CONFIG:Release>:/MT>)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()

# ------------------------------------------------------------------------------
# 2. SQLITE STATIC (GIỮ NGUYÊN)
# ------------------------------------------------------------------------------
message(STATUS "Dang cau hinh SQLite3 che do Static (Embedded)...")
include(FetchContent)
FetchContent_Declare(
    sqlite3_src
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3_src)
add_library(sqlite3_static STATIC "${sqlite3_src_SOURCE_DIR}/sqlite3.c")
target_include_directories(sqlite3_static PUBLIC "${sqlite3_src_SOURCE_DIR}")
target_compile_definitions(sqlite3_static PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1)
if(MSVC)
    target_compile_options(sqlite3_static PRIVATE /W0)
endif()

# ------------------------------------------------------------------------------
# 3. CÁC THƯ VIỆN KHÁC (GIỮ NGUYÊN)
# ------------------------------------------------------------------------------
find_package(nlohmann_json CONFIG REQUIRED)
set(Boost_USE_STATIC_LIBS ON) 
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# ------------------------------------------------------------------------------
# 4. KHAI BÁO SOURCE FILES (CẬP NHẬT MỚI)
# ------------------------------------------------------------------------------

# Các file lõi mới tách ra
set(CORE_SRCS
    src/utils/SystemUtils.cpp
    src/core/RegistryClient.cpp
    src/core/WebSocketServer.cpp
    # CommandDispatcher là Header-only hoặc đã được include, nếu có .cpp thì thêm vào đây
)

if(WIN32)
    set(OS_SRCS
        src/modules/ProcessManager_win.cpp
        src/modules/SystemManager_win.cpp
        src/modules/ScreenManager_win.cpp
        src/modules/AppManager_win.cpp
        src/modules/KeyManager_win.cpp
        src/modules/WebcamManager_win.cpp
        src/modules/FileManager_win.cpp
        src/modules/InputManager_win.cpp
        src/modules/EdgeManager_win.cpp
    )
    set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus crypt32 wininet)
elseif(UNIX)
    set(OS_SRCS
        src/modules/ProcessManager_linux.cpp
        src/modules/SystemManager_linux.cpp
        src/modules/ScreenManager_linux.cpp
        src/modules/AppManager_linux.cpp
        src/modules/KeyManager_linux.cpp
        src/modules/WebcamManager_linux.cpp
        src/modules/FileManager_linux.cpp
        src/modules/InputManager_linux.cpp
        src/modules/EdgeManager_linux.cpp
    )
    find_package(JPEG REQUIRED) 
    set(OS_LIBS pthread dl X11 Xtst JPEG::JPEG)
endif()

# ------------------------------------------------------------------------------
# 5. LINKING
# ------------------------------------------------------------------------------
# Lưu ý: main.cpp nằm trong src/
add_executable(server src/main.cpp ${CORE_SRCS} ${OS_SRCS})

# Include thư mục src để code có thể gọi "utils/SystemUtils.hpp"
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(server PRIVATE 
    sqlite3_static
    Boost::system 
    nlohmann_json::nlohmann_json 
    OpenSSL::Crypto
    OpenSSL::SSL
    ${OS_LIBS}
)