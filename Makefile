#SOURCE_FILES specifies which files to compile as part of the project 
SOURCE_FILES = main.cpp

#COMPILER specifies which compiler we're using 
COMPILER = g++

#INCLUDE_PATHS specifies the additional include paths we'll need 
INCLUDE_PATHS = -IE:/Libraries/SDL2-2.0.9/i686-w64-mingw32/include/SDL2 -IE:/Libraries/SDL2_image-2.0.4/i686-w64-mingw32/include/SDL2

#LIBRARY_PATHS specifies the additional library paths we'll need 
LIBRARY_PATHS = -LE:/Libraries/SDL2-2.0.9/i686-w64-mingw32/lib -LE:/Libraries/SDL2_image-2.0.4/i686-w64-mingw32/lib

#COMPILER_FLAGS specifies the additional compilation options we're using 
# -w suppresses all warnings 
# -Wl,-subsystem,windows gets rid of the console window 
COMPILER_FLAGS = -g #-Wl,-subsystem,windows 

#LINKER_FLAGS specifies the libraries we're linking against 
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

#EXECUTABLE specifies the name of our exectuable 
EXECUTABLE = w3d

#This is the target that compiles our executable 
all: $(OBJS) 
	$(COMPILER) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SOURCE_FILES) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(EXECUTABLE)

run: all
	.\$(EXECUTABLE)