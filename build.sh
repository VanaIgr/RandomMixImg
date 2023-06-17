g++ main.cpp --std=c++20 -O1 -march=native -o executable -lGLEW `pkg-config --cflags glfw3` `pkg-config --static --libs glfw3` -lGL && ./executable
