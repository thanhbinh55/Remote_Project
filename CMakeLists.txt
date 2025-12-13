# cmake_minimum_required(VERSION 3.10)
# project(RemoteControlTool LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # --- [THÊM DÒNG NÀY] Bắt buộc để CMake tìm file .lib tĩnh ---
# set(Boost_USE_STATIC_LIBS ON)

# # Tắt cảnh báo về các hàm C cũ (localtime, strcpy...)
# add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
# # --- TÌM THƯ VIỆN CHUNG ---
# find_package(Boost REQUIRED COMPONENTS system)
# find_package(nlohmann_json CONFIG REQUIRED)

# # --- CẤU HÌNH CỤ THỂ TỪNG OS ---

# if(WIN32)
#     message(STATUS "Build target: WINDOWS")
#     add_compile_definitions(WIN32_LEAN_AND_MEAN)
    
#     # Ép Static Runtime để dễ đem file exe đi máy khác
#     if(MSVC)
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#         add_compile_options(/MT$<$<CONFIG:Debug>:d>)
#     endif()

#     set(OS_SRCS
#         src/modules/ProcessManager_win.cpp
#         src/modules/SystemManager_win.cpp
#         src/modules/ScreenManager_win.cpp
#         src/modules/AppManager_win.cpp
#         src/modules/KeyManager_win.cpp
#         src/modules/WebcamManager_win.cpp
#         src/modules/FileManager_win.cpp
#         src/modules/InputManager_win.cpp
#     )
    
#     set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus)

# elseif(UNIX AND NOT APPLE)
#     message(STATUS "Build target: LINUX")
    
#     set(OS_SRCS
#         src/modules/ProcessManager_linux.cpp
#         src/modules/SystemManager_linux.cpp
#         src/modules/ScreenManager_linux.cpp
#         src/modules/AppManager_linux.cpp
#         src/modules/KeyManager_linux.cpp
#         src/modules/WebcamManager_linux.cpp
#     )
    
#     # Linux cần pthread cho thread, X11 cho chụp màn hình, jpeg cho nén ảnh
#     set(OS_LIBS 
#         pthread 
#         X11 
#         jpeg
#     )
# endif()

# # --- TẠO EXECUTABLE ---
# add_executable(server 
#     main.cpp 
#     ${OS_SRCS}
# )

# target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# target_link_libraries(server PRIVATE 
#     Boost::system 
#     nlohmann_json::nlohmann_json 
#     ${OS_LIBS}
# )

# cmake_minimum_required(VERSION 3.10)
# project(RemoteControlTool LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Tìm nlohmann_json trước (thư viện header-only thường dễ tìm)
# find_package(nlohmann_json CONFIG REQUIRED)

# # --- CẤU HÌNH: WINDOWS ---
# if(WIN32)
#     message(STATUS "Build target: WINDOWS")
    
#     # [WINDOWS ONLY] Dùng Static Libs để dễ đóng gói file .exe mang đi máy khác
#     set(Boost_USE_STATIC_LIBS ON)
    
#     # Tắt cảnh báo CRT cũ chỉ có trên Windows
#     add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
    
#     # Ép Static Runtime (MT) cho MSVC
#     if(MSVC)
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#         add_compile_options(/MT$<$<CONFIG:Debug>:d>)
#     endif()

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
    
#     set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus crypt32)

# # --- CẤU HÌNH: LINUX ---
# elseif(UNIX AND NOT APPLE)
#     message(STATUS "Build target: LINUX")
    
#     # [LINUX ONLY] Trên Linux thường dùng thư viện động (Shared) cài qua apt
#     # Nếu ép Static ON ở đây thường sẽ lỗi vì không tìm thấy file .a
#     set(Boost_USE_STATIC_LIBS OFF) 
    
#     set(OS_SRCS
#         src/modules/ProcessManager_linux.cpp
#         src/modules/SystemManager_linux.cpp
#         src/modules/ScreenManager_linux.cpp
#         src/modules/AppManager_linux.cpp
#         src/modules/KeyManager_linux.cpp
#         src/modules/WebcamManager_linux.cpp
#         # Lưu ý: Bạn đang thiếu FileManager và InputManager phiên bản Linux
#         # Nếu chưa code xong, hãy comment tạm dòng dưới để tránh lỗi compile
#         src/modules/FileManager_linux.cpp 
#         src/modules/InputManager_linux.cpp
#     )
    
#     # Thư viện cần thiết cho Linux
#     set(OS_LIBS 
#         pthread 
#         X11 
#         jpeg
#     )
# endif()

# # --- TÌM BOOST ---
# # Đặt sau khối if-else để nhận biến Boost_USE_STATIC_LIBS đúng theo từng OS
# find_package(Boost REQUIRED COMPONENTS system)

# # --- TẠO EXECUTABLE ---
# add_executable(server 
#     main.cpp 
#     ${OS_SRCS}
# )

# target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# target_link_libraries(server PRIVATE 
#     Boost::system 
#     nlohmann_json::nlohmann_json 
#     ${OS_LIBS}
# )

# Code này chạy được 

# cmake_minimum_required(VERSION 3.15)
# project(RemoteControlTool LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Thiết lập policy CMP0167 để tránh cảnh báo Boost
# if(POLICY CMP0167)
#     cmake_policy(SET CMP0167 NEW)
# endif()

# # Tìm nlohmann_json trước
# find_package(nlohmann_json CONFIG REQUIRED)

# # --- CẤU HÌNH: WINDOWS ---
# if(WIN32)
#     message(STATUS "Build target: WINDOWS")
    
#     # [WINDOWS ONLY] Dùng Static Libs để dễ đóng gói file .exe mang đi máy khác
#     set(Boost_USE_STATIC_LIBS ON)
    
#     # Tắt cảnh báo CRT cũ chỉ có trên Windows
#     add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
    
#     # Ép Static Runtime (MT) cho MSVC
#     if(MSVC)
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#         add_compile_options(/MT$<$<CONFIG:Debug>:d>)
#     endif()

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
    
#     # Thêm các thư viện Windows cần thiết
#     set(OS_LIBS 
#         ws2_32 
#         user32 
#         advapi32 
#         shlwapi 
#         ole32 
#         mf mfplat mfreadwrite mfuuid 
#         gdiplus 
#         crypt32
#         wininet        # Thêm wininet cho Windows
#     )
    
#     # --- SQLITE3 CẤU HÌNH RIÊNG CHO WINDOWS ---
#     # Phương pháp 1: Tìm package (nếu đã cài)
#     find_package(SQLite3 QUIET)

#     # Normalize variables returned by different Find/Config packages.
#     if(SQLite3_FOUND)
#         if(NOT DEFINED SQLITE3_LIBRARIES)
#             if(DEFINED SQLite3_LIBRARIES)
#                 set(SQLITE3_LIBRARIES ${SQLite3_LIBRARIES})
#             elseif(DEFINED SQLITE3_LIBRARY)
#                 set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
#             elseif(DEFINED SQLite3_LIBRARY)
#                 set(SQLITE3_LIBRARIES ${SQLite3_LIBRARY})
#             elseif(TARGET SQLite::SQLite3)
#                 set(SQLITE3_LIBRARIES SQLite::SQLite3)
#             endif()
#         endif()

#         if(NOT DEFINED SQLITE3_INCLUDE_DIRS)
#             if(DEFINED SQLITE3_INCLUDE_DIR)
#                 set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
#             elseif(DEFINED SQLite3_INCLUDE_DIR)
#                 set(SQLITE3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
#             elseif(TARGET SQLite::SQLite3)
#                 get_target_property(_sqlite_inc SQLite::SQLite3 INTERFACE_INCLUDE_DIRECTORIES)
#                 set(SQLITE3_INCLUDE_DIRS ${_sqlite_inc})
#             endif()
#         endif()

#         # If we couldn't determine usable include/libs, treat it as not found to trigger fallback
#         if(NOT SQLITE3_LIBRARIES OR NOT SQLITE3_INCLUDE_DIRS)
#             set(SQLite3_FOUND FALSE)
#             unset(SQLITE3_LIBRARIES CACHE)
#             unset(SQLITE3_INCLUDE_DIRS CACHE)
#         endif()
#     endif()
    
#     if(NOT SQLite3_FOUND)
#         # Phương pháp 2: Tìm thủ công
#         find_library(SQLITE3_LIBRARY
#             NAMES sqlite3 sqlite3.lib
#             PATHS
#                 "C:/Program Files/sqlite"
#                 "C:/sqlite"
#                 "C:/local/sqlite"
#                 ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
#         )
        
#         find_path(SQLITE3_INCLUDE_DIR
#             NAMES sqlite3.h
#             PATHS
#                 "C:/Program Files/sqlite/include"
#                 "C:/sqlite/include"
#                 "C:/local/sqlite/include"
#                 ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
#         )
        
#         if(SQLITE3_LIBRARY AND SQLITE3_INCLUDE_DIR)
#             set(SQLITE3_FOUND TRUE)
#             set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
#             set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
#         endif()
#     endif()
    
#     if(NOT SQLITE3_FOUND)
#         # Phương pháp 3: Tải và include trực tiếp (nếu không tìm thấy)
#         message(WARNING "SQLite3 not found, attempting to download...")
#         include(FetchContent)
#         FetchContent_Declare(
#             sqlite3
#             URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
#         )
#         FetchContent_MakeAvailable(sqlite3)
#         # Build a static sqlite3 target from the amalgamation if no target/library was found
#         if(NOT TARGET sqlite3)
#             add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
#             target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
#             target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)
#         endif()
#         set(SQLITE3_LIBRARIES sqlite3)
#         set(SQLITE3_INCLUDE_DIRS ${sqlite3_SOURCE_DIR})
#     endif()
    
#     message(STATUS "SQLite3 found: ${SQLITE3_FOUND}")
#     message(STATUS "SQLite3 includes: ${SQLITE3_INCLUDE_DIRS}")
#     message(STATUS "SQLite3 libraries: ${SQLITE3_LIBRARIES}")
    
#     # --- OPENSSL CẤU HÌNH ---
#     find_package(OpenSSL REQUIRED)
    
#     # Thêm DPAPI nếu có thư viện riêng
#     if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPAPI")
#         include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPAPI)
#         list(APPEND OS_LIBS dpapi_lib)
#     endif()

# # --- CẤU HÌNH: LINUX ---
# elseif(UNIX AND NOT APPLE)
#     message(STATUS "Build target: LINUX")
    
#     # [LINUX ONLY] Trên Linux thường dùng thư viện động
#     set(Boost_USE_STATIC_LIBS OFF)
    
#     set(OS_SRCS
#         src/modules/ProcessManager_linux.cpp
#         src/modules/SystemManager_linux.cpp
#         src/modules/ScreenManager_linux.cpp
#         src/modules/AppManager_linux.cpp
#         src/modules/KeyManager_linux.cpp
#         src/modules/WebcamManager_win.cpp
#         src/modules/FileManager_linux.cpp
#         src/modules/InputManager_linux.cpp
#         src/modules/EdgeManager_linux.cpp
#     )
    
#     # Tìm các thư viện Linux
#     find_package(SQLite3 REQUIRED)
#     find_package(OpenSSL REQUIRED)
#     find_package(JPEG REQUIRED)
#     find_package(X11 REQUIRED)
    
#     # Thư viện cần thiết cho Linux
#     set(OS_LIBS 
#         pthread 
#         ${X11_LIBRARIES}
#         ${JPEG_LIBRARIES}
#         ${SQLITE3_LIBRARIES}
#         OpenSSL::Crypto
#         OpenSSL::SSL
#         dl
#         rt
#     )
# endif()

# # --- TÌM BOOST ---
# set(BOOST_ROOT "C:/local/boost_1_83_0" CACHE PATH "Boost library root directory")
# find_package(Boost 1.70 REQUIRED COMPONENTS system)

# if(Boost_FOUND)
#     message(STATUS "Boost found: ${Boost_VERSION}")
#     message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
#     message(STATUS "Boost library dirs: ${Boost_LIBRARY_DIRS}")
# else()
#     message(FATAL_ERROR "Boost library not found!")
# endif()

# # --- TẠO EXECUTABLE ---
# add_executable(server 
#     main.cpp 
#     ${OS_SRCS}
# )

# # Thêm đường dẫn include tổng
# target_include_directories(server PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
#     ${CMAKE_CURRENT_SOURCE_DIR}/third_party
#     ${Boost_INCLUDE_DIRS}
#     ${SQLITE3_INCLUDE_DIRS}
#     ${OPENSSL_INCLUDE_DIR}
# )

# # Link tất cả thư viện - SỬA LỖI LINK SQLITE3
# target_link_libraries(server PRIVATE 
#     Boost::system
#     nlohmann_json::nlohmann_json 
#     ${SQLITE3_LIBRARIES}
#     ${OS_LIBS}
#     OpenSSL::Crypto
#     OpenSSL::SSL
# )

# # Thêm tính năng C++17
# target_compile_features(server PRIVATE cxx_std_17)

# # Hiển thị thông tin cấu hình
# message(STATUS "=== CONFIGURATION SUMMARY ===")
# message(STATUS "SQLite3 include dirs: ${SQLITE3_INCLUDE_DIRS}")
# message(STATUS "SQLite3 libraries: ${SQLITE3_LIBRARIES}")
# message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
# message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
# cmake_minimum_required(VERSION 3.15)
# project(RemoteControlTool LANGUAGES C CXX)  # THÊM C vào đây

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_C_STANDARD 11)  # THÊM DÒNG NÀY
# set(CMAKE_C_STANDARD_REQUIRED ON)

# # Thiết lập policy CMP0167 để tránh cảnh báo Boost
# if(POLICY CMP0167)
#     cmake_policy(SET CMP0167 NEW)
# endif()

# # Tìm nlohmann_json trước
# find_package(nlohmann_json CONFIG REQUIRED)

# # --- CẤU HÌNH: WINDOWS ---
# if(WIN32)
#     message(STATUS "Build target: WINDOWS")
    
#     # [WINDOWS ONLY] Dùng Static Libs để dễ đóng gói file .exe mang đi máy khác
#     set(Boost_USE_STATIC_LIBS ON)
    
#     # Tắt cảnh báo CRT cũ chỉ có trên Windows
#     add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
    
#     # Ép Static Runtime (MT) cho MSVC
#     if(MSVC)
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#         add_compile_options(/MT$<$<CONFIG:Debug>:d>)
#     endif()

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
    
#     # Thêm các thư viện Windows cần thiết
#     set(OS_LIBS 
#         ws2_32 
#         user32 
#         advapi32 
#         shlwapi 
#         ole32 
#         mf mfplat mfreadwrite mfuuid 
#         gdiplus 
#         crypt32
#         wininet
#     )
    
#     # --- SQLITE3: TẢI VÀ COMPILE TRỰC TIẾP VÀO EXE (STATIC LINKING) ---
#     message(STATUS "Downloading SQLite3 amalgamation...")
    
#     include(FetchContent)
#     FetchContent_Declare(
#         sqlite3_amalgamation
#         URL https://www.sqlite.org/2025/sqlite-amalgamation-3460000.zip
#         DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#         SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/sqlite3-src"
#     )
    
#     FetchContent_GetProperties(sqlite3_amalgamation)
#     if(NOT sqlite3_amalgamation_POPULATED)
#         FetchContent_Populate(sqlite3_amalgamation)
#     endif()
    
#     # Tạo thư viện tĩnh từ source code SQLite - SỬA LẠI CÁCH KHAI BÁO
#     file(GLOB SQLITE_SOURCES 
#         "${sqlite3_amalgamation_SOURCE_DIR}/*.c"
#         "${sqlite3_amalgamation_SOURCE_DIR}/*.h"
#     )
    
#     # CÁCH 1: Dùng add_library với source file cụ thể
#     add_library(sqlite3_static STATIC
#         "${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c"
#     )
    
#     # CÁCH 2: Hoặc tạo INTERFACE library nếu chỉ có header (nhưng SQLite cần compile .c file)
#     # add_library(sqlite3_static INTERFACE)
#     # target_sources(sqlite3_static INTERFACE "${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c")
    
#     # Cấu hình biên dịch cho SQLite
#     target_compile_definitions(sqlite3_static PRIVATE
#         SQLITE_THREADSAFE=1
#         SQLITE_OMIT_LOAD_EXTENSION=1
#         SQLITE_DEFAULT_MEMSTATUS=0
#     )
    
#     # Đặt ngôn ngữ rõ ràng cho target này
#     set_target_properties(sqlite3_static PROPERTIES
#         LINKER_LANGUAGE C
#     )
    
#     # Thêm include directory
#     target_include_directories(sqlite3_static PUBLIC
#         ${sqlite3_amalgamation_SOURCE_DIR}
#     )
    
#     # Trên Windows, disable warnings cho SQLite code
#     if(MSVC)
#         target_compile_options(sqlite3_static PRIVATE /W0)
#     else()
#         target_compile_options(sqlite3_static PRIVATE -w)
#     endif()
    
#     set(SQLITE3_FOUND TRUE)
#     set(SQLITE3_LIBRARIES sqlite3_static)
#     set(SQLITE3_INCLUDE_DIRS ${sqlite3_amalgamation_SOURCE_DIR})
    
#     # --- OPENSSL CẤU HÌNH ---
#     find_package(OpenSSL REQUIRED)

# # --- CẤU HÌNH: LINUX ---
# elseif(UNIX AND NOT APPLE)
#     message(STATUS "Build target: LINUX")
    
#     # [LINUX ONLY] Trên Linux thường dùng thư viện động
#     set(Boost_USE_STATIC_LIBS OFF)
    
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
    
#     # Tương tự cho Linux
#     include(FetchContent)
#     FetchContent_Declare(
#         sqlite3_amalgamation
#         URL https://www.sqlite.org/2025/sqlite-amalgamation-3460000.zip
#         DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#     )
    
#     FetchContent_GetProperties(sqlite3_amalgamation)
#     if(NOT sqlite3_amalgamation_POPULATED)
#         FetchContent_Populate(sqlite3_amalgamation)
#     endif()
    
#     add_library(sqlite3_static STATIC
#         "${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c"
#     )
    
#     target_compile_definitions(sqlite3_static PRIVATE
#         SQLITE_THREADSAFE=1
#         SQLITE_OMIT_LOAD_EXTENSION=1
#         SQLITE_DEFAULT_MEMSTATUS=0
#     )
    
#     set_target_properties(sqlite3_static PROPERTIES
#         LINKER_LANGUAGE C
#     )
    
#     target_include_directories(sqlite3_static PUBLIC
#         ${sqlite3_amalgamation_SOURCE_DIR}
#     )
    
#     target_compile_options(sqlite3_static PRIVATE -w)
    
#     set(SQLITE3_FOUND TRUE)
#     set(SQLITE3_LIBRARIES sqlite3_static)
#     set(SQLITE3_INCLUDE_DIRS ${sqlite3_amalgamation_SOURCE_DIR})
    
#     # Tìm các thư viện Linux khác
#     find_package(OpenSSL REQUIRED)
#     find_package(JPEG REQUIRED)
#     find_package(X11 REQUIRED)
    
#     # Thư viện cần thiết cho Linux
#     set(OS_LIBS 
#         pthread 
#         ${X11_LIBRARIES}
#         ${JPEG_LIBRARIES}
#         OpenSSL::Crypto
#         OpenSSL::SSL
#         dl
#         rt
#     )
# endif()

# # --- TÌM BOOST ---
# find_package(Boost 1.70 REQUIRED COMPONENTS system)

# if(Boost_FOUND)
#     message(STATUS "Boost found: ${Boost_VERSION}")
# else()
#     message(FATAL_ERROR "Boost library not found!")
# endif()

# # --- TẠO EXECUTABLE ---
# add_executable(server 
#     main.cpp 
#     ${OS_SRCS}
# )

# # Thêm đường dẫn include tổng
# target_include_directories(server PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/src
#     ${CMAKE_CURRENT_SOURCE_DIR}/third_party
#     ${Boost_INCLUDE_DIRS}
#     ${SQLITE3_INCLUDE_DIRS}
# )

# if(OpenSSL_FOUND)
#     target_include_directories(server PRIVATE ${OPENSSL_INCLUDE_DIR})
# endif()

# # Link tất cả thư viện
# target_link_libraries(server PRIVATE 
#     Boost::system
#     nlohmann_json::nlohmann_json 
#     ${SQLITE3_LIBRARIES}
#     ${OS_LIBS}
# )

# if(OpenSSL_FOUND)
#     target_link_libraries(server PRIVATE 
#         OpenSSL::Crypto
#         OpenSSL::SSL
#     )
# endif()

# # Thêm tính năng C++17
# target_compile_features(server PRIVATE cxx_std_17)

# # Hiển thị thông tin cấu hình
# message(STATUS "=== CONFIGURATION SUMMARY ===")
# message(STATUS "Build type: Static linking")
# message(STATUS "SQLite3: Embedded (no DLL required)")
# message(STATUS "SQLite3 include dirs: ${SQLITE3_INCLUDE_DIRS}")
# message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# cmake_minimum_required(VERSION 3.15)
# project(RemoteControlTool LANGUAGES C CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# # --- QUAN TRỌNG: CẤU HÌNH STATIC LINKING ---
# if(WIN32)
#     # 1. Sử dụng Static Runtime Library (/MT)
#     if(MSVC)
#         # Ép dùng static runtime (/MT hoặc /MTd)
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        
#         # Thêm compile option
#         add_compile_options(/MT$<$<CONFIG:Debug>:d>)
        
#         # Hoặc dùng cách này:
#         # set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
#         # set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
#     endif()
    
#     # 2. Ép tìm static libraries
#     set(Boost_USE_STATIC_LIBS ON)
#     set(OPENSSL_USE_STATIC_LIBS ON)
#     set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    
#     # 3. Tắt cảnh báo và cấu hình Windows
#     add_compile_definitions(
#         _CRT_SECURE_NO_WARNINGS
#         WIN32_LEAN_AND_MEAN
#         BOOST_ALL_NO_LIB  # Bắt Boost dùng auto-link
#         BOOST_ALL_STATIC_LIB  # Bắt Boost link static
#     )
# endif()

# # Tìm các package với static libs
# find_package(Boost REQUIRED COMPONENTS system)
# find_package(OpenSSL REQUIRED)
# find_package(nlohmann_json CONFIG REQUIRED)

# # --- SQLITE3: TẢI VÀ NHÚNG VÀO EXE ---
# include(FetchContent)
# # Sửa URL chính xác (3460000 có thể không tồn tại)
# FetchContent_Declare(
#     sqlite3_amalgamation
#     URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
#     DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#     # KHÔNG dùng URL_HASH
# )

# FetchContent_MakeAvailable(sqlite3_amalgamation)

# # Tạo static library từ sqlite3.c
# add_library(sqlite3_embedded STATIC
#     ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c
# )

# target_include_directories(sqlite3_embedded PUBLIC
#     ${sqlite3_amalgamation_SOURCE_DIR}
# )

# target_compile_definitions(sqlite3_embedded PRIVATE
#     SQLITE_THREADSAFE=1
#     SQLITE_OMIT_LOAD_EXTENSION=1
#     SQLITE_DEFAULT_MEMSTATUS=0
# )

# set_target_properties(sqlite3_embedded PROPERTIES
#     LINKER_LANGUAGE C
#     POSITION_INDEPENDENT_CODE OFF
# )

# # --- TẠO EXECUTABLE CHÍNH ---
# add_executable(server
#     main.cpp
#     src/modules/*.cpp  # Thay bằng danh sách file cụ thể
# )

# # Link tất cả thư viện STATIC
# target_link_libraries(server PRIVATE
#     Boost::system
#     nlohmann_json::nlohmann_json
#     sqlite3_embedded
#     OpenSSL::Crypto
#     OpenSSL::SSL
# )

# # Thêm các thư viện hệ thống Windows
# if(WIN32)
#     target_link_libraries(server PRIVATE
#         ws2_32
#         user32
#         advapi32
#         shell32
#         ole32
#         gdi32
#         crypt32
#         mf
#         mfplat
#         mfreadwrite
#         mfuuid
#         shlwapi
#         wininet
#     )
    
#     # Kiểm tra static linking
#     set_target_properties(server PROPERTIES
#         LINK_SEARCH_START_STATIC ON
#         LINK_SEARCH_END_STATIC ON
#     )
# endif()


cmake_minimum_required(VERSION 3.15)
project(RemoteControlTool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# 1. CẤU HÌNH STATIC RUNTIME (QUAN TRỌNG ĐỂ KHÔNG CẦN CÁC FILE DLL HỆ THỐNG)
# ------------------------------------------------------------------------------
if(MSVC)
    # Ép sử dụng Static Runtime (/MT) cho cả Debug và Release
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options($<$<CONFIG:Debug>:/MTd> $<$<CONFIG:Release>:/MT>)
    
    # Tắt các cảnh báo không cần thiết
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()

# ------------------------------------------------------------------------------
# 2. TỰ ĐỘNG TẢI VÀ NHÚNG SQLITE (ĐỂ KHÔNG CẦN SQLITE3.DLL)
# ------------------------------------------------------------------------------
message(STATUS "Dang cau hinh SQLite3 che do Static (Embedded)...")

include(FetchContent)
FetchContent_Declare(
    sqlite3_src
    # Link tải mã nguồn "Amalgamation" (chỉ gồm 1 file .c và 1 file .h)
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(sqlite3_src)

# Tạo thư viện tĩnh từ file source vừa tải về
# Lưu ý: Biến ${sqlite3_src_SOURCE_DIR} được tạo ra tự động bởi FetchContent
add_library(sqlite3_static STATIC "${sqlite3_src_SOURCE_DIR}/sqlite3.c")

# Cấu hình include folder
target_include_directories(sqlite3_static PUBLIC "${sqlite3_src_SOURCE_DIR}")

# Các định nghĩa cần thiết để SQLite chạy mượt trong môi trường đa luồng
target_compile_definitions(sqlite3_static PRIVATE 
    SQLITE_THREADSAFE=1 
    SQLITE_OMIT_LOAD_EXTENSION=1
)

if(MSVC)
    # Tắt cảnh báo khi compile code C của SQLite (để log sạch sẽ hơn)
    target_compile_options(sqlite3_static PRIVATE /W0)
endif()

# ------------------------------------------------------------------------------
# 3. CÁC THƯ VIỆN KHÁC
# ------------------------------------------------------------------------------
# Tìm nlohmann_json (Header only nên không lo về DLL)
find_package(nlohmann_json CONFIG REQUIRED)

# Cấu hình Boost Static (để không cần Boost DLL)
set(Boost_USE_STATIC_LIBS ON) 
find_package(Boost REQUIRED COMPONENTS system)

# Tìm OpenSSL (Cố gắng tìm static nếu có thể, nếu không bạn phải copy libcrypto/libssl.dll)
# Mẹo: Nếu vcpkg cài dynamic, bạn có thể cần copy 2 file dll của OpenSSL đi kèm.
find_package(OpenSSL REQUIRED)

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
    # Các thư viện hệ thống Windows
    set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus crypt32 wininet)

# === [THÊM PHẦN NÀY CHO LINUX] ===
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
    
    # Thư viện Linux cần thiết
    # pthread: Cho đa luồng (std::thread)
    # dl: Cho dynamic loading
    # X11, Xtst: Thường cần cho ScreenManager (chụp màn hình) và InputManager (chuột/phím)
    
    find_package(JPEG REQUIRED) 
    set(OS_LIBS pthread dl X11 Xtst JPEG::JPEG)
endif()

# ------------------------------------------------------------------------------
# 5. TẠO FILE EXE VÀ LIÊN KẾT (LINKING)
# ------------------------------------------------------------------------------
add_executable(server main.cpp ${OS_SRCS})

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(server PRIVATE 
    sqlite3_static
    Boost::system 
    nlohmann_json::nlohmann_json 
    OpenSSL::Crypto
    OpenSSL::SSL
    ${OS_LIBS} # Biến này giờ sẽ chứa thư viện Windows hoặc Linux tùy môi trường
)