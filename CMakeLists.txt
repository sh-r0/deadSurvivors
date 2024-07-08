cmake_minimum_required(VERSION 3.20.0)
set (CMAKE_CXX_STANDARD 20)

project(deadSurvivors_inator)
add_compile_definitions(CRT_SECURE_NO_WARNINGS)

set(vkIncDir "C:/VulkanSDK/1.3.239.0/Include")
set(vkLibDir "C:/VulkanSDK/1.3.239.0/Lib")
set(glfwIncDir "D:/apis/cpp/glfw/include")
set(glfwLibDir "D:/apis/cpp/glfw/lib-vc2022-64")
set(ftLibDir "D:/apis/cpp/freetype/freetype-2.13.2/objs/x64")
set(ftIncDir "D:/apis/cpp/freetype/freetype-2.13.2/include")

list(APPEND LibsList "${vkLibDir}/vulkan-1.lib" "${glfwLibDir}/glfw3.lib" "${ftLibDir}/freetype.lib")

set(CMAKE_SOURCE_DIR "${MY_ROOT}/src")
include_directories("${MY_ROOT}/src")
include_directories("${ftIncDir}")
include_directories("${vkIncDir}")
include_directories("${glmIncDir}/glm")
include_directories("${glfwIncDir}")
list(APPEND sourcesList "src/main.cpp" "src/renderer.hpp" "src/renderer.cpp")

add_executable(${PROJECT_NAME} ${sourcesList})
list(APPEND libsDirList ${vkLibDir} ${glfwLibDir})
link_directories(${PROJECT_NAME} ${libsDirList})
target_link_libraries(${PROJECT_NAME} ${LibsList})