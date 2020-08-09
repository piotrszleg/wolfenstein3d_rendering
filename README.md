# Wolfenstein 3D like rendering engine 
Copy of 2.5D rendering engine from Wolfenstein 3D game written in C++ using SDL.

Inspired by a video from *The Coding Train* channel [link](https://www.youtube.com/watch?v=vYgIKn7iDH8) which demonstrated the technique using javascript on randomly placed lines.

Comparing to the video it adds procedurally generated rooms and corridors and entities facing the camera.

# Installation
You need to install SDL2 on your computer and correct paths in the Makefile.

Before running the resulting exe you need to either copy the dll files to your system's dll folder or to the same folder as the executable.

Linux package managers usually copy the dynamic library files to correct folders for you.

# Compilation and running
Open your terminal, navigate to this folder and type in `make run`.