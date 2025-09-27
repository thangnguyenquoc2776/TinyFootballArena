cmake_minimum_required(VERSION 3.15)
project(TinyFootballArena CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Thư mục gốc chứa SDL2 và extension
set(SDL2_ROOT "C:/mingw_dev_libs")

# Include paths (trỏ hẳn vào /include/SDL2 để match #include <SDL.h>, <SDL_ttf.h>, …)
include_directories(
    ${SDL2_ROOT}/SDL2/x86_64-w64-mingw32/include/SDL2
    ${SDL2_ROOT}/SDL2_Image/x86_64-w64-mingw32/include/SDL2
    ${SDL2_ROOT}/SDL2_Font/x86_64-w64-mingw32/include/SDL2
    ${SDL2_ROOT}/SDL2_Mixer/x86_64-w64-mingw32/include/SDL2
    ${CMAKE_SOURCE_DIR}/src
)

# Lib paths
link_directories(
    ${SDL2_ROOT}/SDL2/x86_64-w64-mingw32/lib
    ${SDL2_ROOT}/SDL2_Image/x86_64-w64-mingw32/lib
    ${SDL2_ROOT}/SDL2_Font/x86_64-w64-mingw32/lib
    ${SDL2_ROOT}/SDL2_Mixer/x86_64-w64-mingw32/lib
)

# Quét toàn bộ file cpp
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp src/*/*.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Liên kết thư viện
target_link_libraries(${PROJECT_NAME}
    mingw32 SDL2main SDL2
    SDL2_image SDL2_ttf SDL2_mixer
)

# Xuất exe ngay thư mục gốc (TinyFootballArena/)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
