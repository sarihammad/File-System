# VSFS File System

## Dependencies

This repository uses CMake to configure and build each binary.

The repository depends on `FUSE` being installed (version 2.9).
If you are working locally, I recommend using a virtual machine (VirtualBox, WSL2) with an installation of Linux.
For example, if you have Ubuntu 20.04 installed, you can install the dependencies with: `sudo apt install libfuse-dev pkg-config`.
Make sure that running `fusermount --version` on the command line gives version 2.9.

## Compiling

CMake can configure the project for different build systems and IDEs (type `cmake --help` for a list of generators available for your platform).

You can also work via the command line.
I recommend you create a build directory before invoking CMake to configure the project (`cmake -B`).
For example, I can perform the configuration step from the project root directory:

	cmake -H. -Bcmake-build-release -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake -H. -Bcmake-build-debug -DCMAKE_BUILD_TYPE=Debug

After the configuration step, you can ask CMake to build the project.

	cmake --build cmake-build-release/ --target all
	cmake --build cmake-build-debug/ --target all

## Running

If compilation was successful, you will find the compiled binaries **inside your build directory**.
All executable files can be find inside the `bin` subdirectory of your build directory.
For example: `cmake-build-debug/bin`.
Please refer to the handout for further instructions on running these executables.
