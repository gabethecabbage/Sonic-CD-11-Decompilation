set(PROJECT_NAME SonicCDDecomp)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "Animation.hpp"
    "Audio.hpp"
    "Collision.hpp"
    "Debug.hpp"
    "Drawing.hpp"
    "Ini.hpp"
    "Input.hpp"
    "Math.hpp"
    "Object.hpp"
    "Palette.hpp"
    "Player.hpp"
    "Reader.hpp"
    "resource.h"
    "RetroEngine.hpp"
    "Scene.hpp"
    "Scene3D.hpp"
    "Script.hpp"
    "Sprite.hpp"
    "String.hpp"
    "Text.hpp"
    "Userdata.hpp"
    "Video.hpp"
)
source_group("Header Files" FILES ${Header_Files})

set(Header_Files__theoraPlay
    "${CMAKE_SOURCE_DIR}/dependencies/all/theoraplay/theoraplay.h"
    "${CMAKE_SOURCE_DIR}/dependencies/all/theoraplay/theoraplay_cvtrgb.h"
)
source_group("Header Files\\theoraPlay" FILES ${Header_Files__theoraPlay})

set(Source_Files
    "Animation.cpp"
    "Audio.cpp"
    "Collision.cpp"
    "Debug.cpp"
    "Drawing.cpp"
    "Ini.cpp"
    "Input.cpp"
    "main.cpp"
    "Math.cpp"
    "Object.cpp"
    "Palette.cpp"
    "Player.cpp"
    "Reader.cpp"
    "RetroEngine.cpp"
    "Scene.cpp"
    "Scene3D.cpp"
    "Script.cpp"
    "Sprite.cpp"
    "String.cpp"
    "Text.cpp"
    "Userdata.cpp"
    "Video.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__theoraPlay
    "${CMAKE_SOURCE_DIR}/dependencies/all/theoraplay/theoraplay.c"
)
source_group("Source Files\\theoraPlay" FILES ${Source_Files__theoraPlay})

set(ALL_FILES
    ${Header_Files}
    ${Header_Files__theoraPlay}
    ${Source_Files}
    ${Source_Files__theoraPlay}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})
set(ROOT_NAMESPACE SonicCDDecomp)

################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "Sonic CD"
    TARGET_NAME_RELEASE "Sonic CD"
)
################################################################################
# Include directories
################################################################################
set(ADDITIONAL_INCLUDE_DIRECTORIES
    "${SDL2_INCLUDE_DIRS};"
    "${THEORA_INCLUDE_DIRS};"
    "${VORBIS_INCLUDE_DIR};"
    "${OGG_INCLUDE_DIR};"
)

include_directories(${ADDITIONAL_INCLUDE_DIRECTORIES})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_SOURCE_DIR}/dependencies/all/theoraplay"
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "${SDL2_LIBRARIES};"
    "${OGGVORBIS_LIBRARIES};"
    "${THEORA_LIBRARIES};"
    "pthread;"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
