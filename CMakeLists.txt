cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(assimp)

find_package(OpenGL REQUIRED)

add_executable(main src/main.cpp src/ModelLoader.cpp src/Scene.cpp src/OrbitalCamera.cpp )
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics assimp::assimp OpenGL::GL)

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
add_custom_command(TARGET main POST_BUILD
    # Remove any previous assets, then copy assets and SFML runtime DLLs to the exe folder
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:main>/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:main>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" $<TARGET_FILE_DIR:main>/assets
    # Copy SFML runtime libraries that the SFML::Graphics target depends on
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SFML::Graphics> $<TARGET_FILE_DIR:main>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SFML::Window> $<TARGET_FILE_DIR:main>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SFML::System> $<TARGET_FILE_DIR:main>
    # Optional: Audio / Network if your project or SFML build provides them
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SFML::Audio> $<TARGET_FILE_DIR:main> || true
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SFML::Network> $<TARGET_FILE_DIR:main> || true
)
