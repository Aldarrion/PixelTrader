# Pixel Trader

Pixel trader is a prototype of a local multiplayer game where two players shoot each other with bows in a small arena (similar to TowerFall Ascension). It is built mostly from scratch on top of a custom engine which serves as my experimentation playground. The game itself uses simple archetype Entity Component System implementation inspied by FLECS.

The most interesting part of the engine is probably the renderer written in Vulkan. There are also custom containers, math library, and anything else I find interesting to try to implement myself. I try to use as few libraries as possible here while still gettings things done.

# Getting started

1. Install [Vulakn SDK](https://vulkan.lunarg.com/) version `1.2.162.1`
1. Add `dxc` to your PATH (or modify the shader compile script)
1. Compile shaders by running `Engine/BuildShaders.bat` (bash script comming soon)
1. Generate CMake and `VK_SDK_DIR` variable to the Vulkan SDK root directory
1. Build and run
