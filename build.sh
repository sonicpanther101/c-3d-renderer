cd build

cmake -D GLFW_BUILD_WAYLAND=0 ..

make

.\C++-renderer.exe