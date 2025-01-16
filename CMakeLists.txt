cmake_minimum_required(VERSION 3.20.0)
set (CMAKE_CXX_STANDARD 20)

project(deadSurvivors_inator)
add_compile_definitions(CRT_SECURE_NO_WARNINGS)

#								CONFIG INFO
set(vkIncDir "")		#vulkan include dir
set(vkLibDir "")			#vulkan library dir
set(glfwIncDir "")	#glfw
set(glfwLibDir "")
set(ftLibDir "")			#freetype
set(ftIncDir "")
#-----------------------CONFIG INFO

list(APPEND LibsList "${vkLibDir}/vulkan-1.lib" "${glfwLibDir}/glfw3.lib" "${ftLibDir}/freetype.lib")

set(CMAKE_SOURCE_DIR "${MY_ROOT}/src")
include_directories("${MY_ROOT}/src")
include_directories("${ftIncDir}")
include_directories("${vkIncDir}")
include_directories("${glmIncDir}/glm")
include_directories("${glfwIncDir}")
list(APPEND sourcesList "src/main.cpp" "src/renderer.cpp" "src/gameManager.cpp" "src/utils.cpp"
"src/levelManager.cpp" "src/spells.cpp")

add_executable(${PROJECT_NAME} ${sourcesList})
list(APPEND libsDirList ${vkLibDir} ${glfwLibDir})
link_directories(${PROJECT_NAME} ${libsDirList})
target_link_libraries(${PROJECT_NAME} ${LibsList})