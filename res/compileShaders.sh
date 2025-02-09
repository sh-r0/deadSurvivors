#!/bin/bash

glslc -fshader-stage=vertex shaders_code/vertex.glsl -o shaders/vertex.spv 
glslc -fshader-stage=fragment shaders_code/fragment.glsl -o shaders/fragment.spv

glslc -fshader-stage=vertex shaders_code/pVertex.glsl -o shaders/pVertex.spv 
glslc -fshader-stage=fragment shaders_code/pFragment.glsl -o shaders/pFragment.spv
