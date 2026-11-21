cmake_minimum_required(VERSION 3.16)

project(AllegroScore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network Xml)

include(FetchContent)

# --- Verovio (MusicXML/MEI -> SVG C++ toolkit) ---
FetchContent_Declare(
    verovio
    GIT_REPOSITORY https://github.com/rism-digital/verovio.git
    GIT_TAG        master
)
set(VEROVIO_BUILD_CLI OFF CACHE INTERNAL "" FORCE)
set(VEROVIO_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(BUILD_AS_LIBRARY ON CACHE INTERNAL "" FORCE)
set(NO_HUMDRUM_SUPPORT ON CACHE INTERNAL "" FORCE)
set(NO_DARMS_SUPPORT ON CACHE INTERNAL "" FORCE)
set(NO_EDIT_SUPPORT OFF CACHE INTERNAL "" FORCE)
set(NO_RUNTIME ON CACHE INTERNAL "" FORCE)
FetchContent_Populate(verovio)

add_subdirectory(${verovio_SOURCE_DIR}/cmake ${CMAKE_CURRENT_BINARY_DIR}/verovio-build)

# --- nanosvg (SVG -> QImage rasterizer, header-only) ---
FetchContent_Declare(
    nanosvg
    GIT_REPOSITORY https://github.com/memononen/nanosvg.git
    GIT_TAG        master
)
FetchContent_Populate(nanosvg)

# --- AllegroScore target ---
add_executable(AllegroScore
    src/main.cpp
    src/MainWindow.cpp
    src/VerovioScoreRenderer.cpp
    src/AIService.cpp
    src/ScoreDocument.cpp
    src/NoteEntryToolbar.cpp
    include/MainWindow.h
    include/ScoreRenderer.h
    include/VerovioScoreRenderer.h
    include/AIService.h
    include/ScoreDocument.h
    include/NoteEntryToolbar.h
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
    Qt6::Xml
    verovio
)

install(TARGETS AllegroScore RUNTIME DESTINATION bin)