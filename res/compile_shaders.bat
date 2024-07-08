:: SET GLSLC.EXE PATH E.G. ...\VulkanSDK\1.3.239.0\Bin\glslc.exe
set VkPath=""
%VkPath% -fshader-stage=vertex shaders_code\vertex.glsl -o shaders\vertex.spv 
%VkPath% -fshader-stage=fragment shaders_code\fragment.glsl -o shaders\fragment.spv

pause