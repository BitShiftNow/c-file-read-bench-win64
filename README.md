# C File Reading Benchmark

This benchmark is comparing the runtime of various C file reading methods for Windows.


## Info

Build Platform: `Windows 11 Home 64-bit - 23H2 - 22631.4169`

Compiler: `Microsoft (R) C/C++ Optimizing Compiler Version 19.38.33135 for x64`


## How to build

Make sure you have `cl.exe` installed and it can be used from the terminal (run `cl /help` to test).
To obtain the latest version of the MSVC compiler check out [PortableBuildTools](https://github.com/Data-Oriented-House/PortableBuildTools).

Go to the main directory of the source code and run `build` to execute the `build.bat` script. This will build the product in debug mode. The resulting build artefacts can be found under `build\debug`. To build the product in release mode run `build release` instead. The resulting build artefacts can be found under `build\release`.

To run the built executable right after the build step use `build run` (`build debug run` or `build release run` if you want to target specific versions. The default is `debug`).

To run the built executable with the [RAD Debugger](https://github.com/EpicGamesExt/raddebugger) attached use `build rundbg` (`build debug rundbg` or `build release rundbg` if you want to target specific versions. The default is `debug`).

To create an executable that is ready for distribution use `build release publish`. The resulting build artefacts can be found under `build\publish`.


## License

Unless stated otherwise in individual source files the license is `Public Domain (www.unlicense.org)`
