cmake_minimum_required(VERSION 3.20.0)

project(deadSurvivors_inator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)

add_compile_definitions(CRT_SECURE_NO_WARNINGS)

set(CMAKE_SOURCE_DIR "${MY_ROOT}/src")
include_directories("${MY_ROOT}/src")
include_directories(${Vulkan_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS} ${Freetype_INCLUDE_DIRS})
include_directories("${glmIncDir}/glm")
list(APPEND sourcesList "src/main.cpp" "src/renderer.cpp" "src/gameManager.cpp" "src/utils.cpp"
    "src/levelManager.cpp" "src/spells.cpp" "src/saveFiles.cpp")

add_executable(${PROJECT_NAME} ${sourcesList})

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw Freetype::Freetype)
