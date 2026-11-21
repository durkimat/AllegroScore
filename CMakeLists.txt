cmake_minimum_required(VERSION 3.16)

project(AllegroScore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

include(FetchContent)

# --- Build all libraries statically so everything is embedded in the executable ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# --- Verovio (MusicXML/MEI -> SVG C++ toolkit) ---
FetchContent_Declare(
    verovio
    GIT_REPOSITORY https://github.com/rism-digital/verovio.git
    GIT_TAG        master
)
set(VEROVIO_BUILD_CLI OFF CACHE INTERNAL "" FORCE)
set(VEROVIO_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(VEROVIO_USE_STATIC_LIBS ON CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(verovio)

# --- nanosvg (SVG -> QImage rasterizer, header-only) ---
FetchContent_Declare(
    nanosvg
    GIT_REPOSITORY https://github.com/memononen/nanosvg.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(nanosvg)

# --- AllegroScore target ---
add_executable(AllegroScore
    src/main.cpp
    src/MainWindow.cpp
    src/VerovioScoreRenderer.cpp
    src/AIService.cpp
    include/MainWindow.h
    include/ScoreRenderer.h
    include/VerovioScoreRenderer.h
    include/AIService.h
)

target_include_directories(AllegroScore PRIVATE include
    ${nanosvg_SOURCE_DIR}/src
    ${verovio_SOURCE_DIR}/include
    ${verovio_SOURCE_DIR}/include/crc
    ${verovio_SOURCE_DIR}/include/midi
    ${verovio_SOURCE_DIR}/include/hum
    ${verovio_SOURCE_DIR}/include/json
    ${verovio_SOURCE_DIR}/include/pugi
    ${verovio_SOURCE_DIR}/include/tuning-library
    ${verovio_SOURCE_DIR}/include/zip
    ${verovio_SOURCE_DIR}/include/vrv
    ${verovio_SOURCE_DIR}/libmei/dist
    ${verovio_SOURCE_DIR}/libmei/addons
)

target_link_libraries(AllegroScore PRIVATE
    Qt6::Widgets
    Qt6::Network
    verovio
    pugixml
)

install(TARGETS AllegroScore RUNTIME DESTINATION bin)