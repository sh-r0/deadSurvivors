# EditText_inator
Small text editor created using Vulkan, C++ and GLFW. In spite of general design trend, it's using gap buffer as it's core instead of rope or piece table data structures. 

## Usage
In order to open file pass it's path as first argument in console or drag onto program's .exe file on Windows. Program supports basic keyboard movements and US-keyboard layout.

## Building
In order to build this project, it's necessary to build it's dependencies and configure CMakeLists.txt file.    
In some cases it may be required to recompile shaders using glslc which should be located in VulkanSDK directory.

## Dependencies
* GLFW - https://www.glfw.org
* Vulkan - https://www.vulkan.org
* stb - https://github.com/nothings/stb
* freetype - https://freetype.org