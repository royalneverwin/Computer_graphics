# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug

# Include any dependencies generated for this target.
include src/CMakeFiles/Hand.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/Hand.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/Hand.dir/flags.make

src/CMakeFiles/Hand.dir/main.cpp.o: src/CMakeFiles/Hand.dir/flags.make
src/CMakeFiles/Hand.dir/main.cpp.o: ../src/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/Hand.dir/main.cpp.o"
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src && /usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Hand.dir/main.cpp.o -c /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/src/main.cpp

src/CMakeFiles/Hand.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Hand.dir/main.cpp.i"
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src && /usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/src/main.cpp > CMakeFiles/Hand.dir/main.cpp.i

src/CMakeFiles/Hand.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Hand.dir/main.cpp.s"
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src && /usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/src/main.cpp -o CMakeFiles/Hand.dir/main.cpp.s

# Object files for target Hand
Hand_OBJECTS = \
"CMakeFiles/Hand.dir/main.cpp.o"

# External object files for target Hand
Hand_EXTERNAL_OBJECTS =

src/Hand: src/CMakeFiles/Hand.dir/main.cpp.o
src/Hand: src/CMakeFiles/Hand.dir/build.make
src/Hand: third_party/assimp/code/libassimpd.a
src/Hand: lib/libGLEWd.a
src/Hand: third_party/stb/libstbd.a
src/Hand: third_party/glfw/src/libglfw3d.a
src/Hand: /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.1.sdk/usr/lib/libz.tbd
src/Hand: third_party/assimp/contrib/irrXML/libIrrXMLd.a
src/Hand: src/CMakeFiles/Hand.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable Hand"
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Hand.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/Hand.dir/build: src/Hand

.PHONY : src/CMakeFiles/Hand.dir/build

src/CMakeFiles/Hand.dir/clean:
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src && $(CMAKE_COMMAND) -P CMakeFiles/Hand.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/Hand.dir/clean

src/CMakeFiles/Hand.dir/depend:
	cd /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/src /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src /Users/wangxinhao/Documents/大二下/计算机图形学/Homework1_hand-graphics-homework-main/cmake-build-debug/src/CMakeFiles/Hand.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/Hand.dir/depend

