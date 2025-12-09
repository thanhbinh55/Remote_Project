cmake_minimum_required(VERSION 3.10)
project(RemoteControlTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- [THÊM DÒNG NÀY] Bắt buộc để CMake tìm file .lib tĩnh ---
set(Boost_USE_STATIC_LIBS ON)

# Tắt cảnh báo về các hàm C cũ (localtime, strcpy...)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
# --- TÌM THƯ VIỆN CHUNG ---
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json CONFIG REQUIRED)

# --- CẤU HÌNH CỤ THỂ TỪNG OS ---

if(WIN32)
    message(STATUS "Build target: WINDOWS")
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    
    # Ép Static Runtime để dễ đem file exe đi máy khác
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        add_compile_options(/MT$<$<CONFIG:Debug>:d>)
    endif()

    set(OS_SRCS
        src/modules/ProcessManager_win.cpp
        src/modules/SystemManager_win.cpp
        src/modules/ScreenManager_win.cpp
        src/modules/AppManager_win.cpp
        src/modules/KeyManager_win.cpp
        src/modules/WebcamManager_win.cpp
        src/modules/FileManager_win.cpp
        src/modules/InputManager_win.cpp
    )
    
    set(OS_LIBS ws2_32 user32 advapi32 shlwapi ole32 mf mfplat mfreadwrite mfuuid gdiplus)

elseif(UNIX AND NOT APPLE)
    message(STATUS "Build target: LINUX")
    
    set(OS_SRCS
        src/modules/ProcessManager_linux.cpp
        src/modules/SystemManager_linux.cpp
        src/modules/ScreenManager_linux.cpp
        src/modules/AppManager_linux.cpp
        src/modules/KeyManager_linux.cpp
        src/modules/WebcamManager_linux.cpp
    )
    
    # Linux cần pthread cho thread, X11 cho chụp màn hình, jpeg cho nén ảnh
    set(OS_LIBS 
        pthread 
        X11 
        jpeg
    )
endif()

# --- TẠO EXECUTABLE ---
add_executable(server 
    main.cpp 
    ${OS_SRCS}
)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(server PRIVATE 
    Boost::system 
    nlohmann_json::nlohmann_json 
    ${OS_LIBS}
)