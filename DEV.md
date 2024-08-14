# Development Notes

## Prerequisites

### Common

* Git
* Cmake
* Python3
* Build Tools
  * macOS: Command Line Tools
  * Linux: gcc, g++  
  * Windows: 
    * ["Visual Studio Build Tools 2022"](https://visualstudio.microsoft.com/downloads/)
    * In "Visual Studio Installer" check also "Desktop Development with C++"

## Dependencies

### macOS/Linux

```shell
python3 -m venv .pyenv
source .pyenv/bin/activate
pip3 install -r requirements.txt
export CONAN_HOME=$(pwd)/.conan-home
conan profile detect
```

### Windows

```bat
python.exe -m venv .pyenv
.pyenv/Scripts/activate.bat
pip.exe install -r requirements.txt
set CONAN_HOME=$(pwd)/.conan-home
conan.exe profile detect
```

## Building the project

| | |
|---|---|
| `./build.py conan-install` | Install dependencies and gathers licenses|
| `./build.py configure` | Configure the project |
| `./build.py` | Build the project |
| `./build.py conan-create` | Create the package |
| or | |
| `./build.py all` | Perform all of the above commands |

*Run `./build.py [cmd] --help` to learn more about configurable arguments.*

## macOS

### Build a fat binary (x86_64 + armv8)

```shell
 ./build.py conan-install -o .conan-x86_64 -pr:h profiles/darwin.release.x86_64
 ./build.py configure -t .conan-x86_64/build/Release/generators/conan_toolchain.cmake -b .build-x86_64 -bt Release
 ./build.py build -b .build-x86_64

 ./build.py conan-install -o .conan-armv8 -pr:h profiles/darwin.release.armv8
 ./build.py configure -t .conan-armv8/build/Release/generators/conan_toolchain.cmake -b .build-armv8 -bt Release
 ./build.py build -b .build-armv8

 mkdir -p .build-universal/
 lipo -create -output .build-universal/opencl-language-server .build-x86_64/opencl-language-server .build-armv8/opencl-language-server
 lipo -archs .build-universal/opencl-language-server 
 ```

### Signing

```shell
codesign -s ${DEVELOPER_ID} --timestamp --force --options runtime .build-universal/opencl-language-server
codesign --verify -vvvv .build-universal/opencl-language-server
```

### Notarization

```shell
xcrun altool --list-providers -p "@keychain:AC_PASSWORD"
xcrun altool --notarize-app --primary-bundle-id "com.${AUTHOR}.opencl-language-server" --password "@keychain:AC_PASSWORD" --asc-provider ${PROVIDER} --file opencl-language-server.zip
xcrun altool --notarization-history 0 --asc-provider ${PROVIDER} --password "@keychain:AC_PASSWORD"
xcrun altool --notarization-info ${REQUEST_ID} --password "@keychain:AC_PASSWORD"
```

## Linux

### Cross compilation (x86_64 -> armv8)

```shell
sudo apt install gcc-9-aarch64-linux-gnu g++-9-aarch64-linux-gnu
./build.py conan-install -o .conan-armv8 -pr:h profiles/linux.release.armv8
./build.py configure -t .conan-armv8/build/Release/generators/conan_toolchain.cmake -b .build-armv8 -bt Release
./build.py build -b .build-armv8
```

*ubuntu-20.04, gcc-9*

## Windows

### Debug Configuration

```bat
python.exe build.py conan-install -pr:h profiles/windows.debug.x86_64
python.exe build.py configure -bt Debug
python.exe build.py build -bt Debug
```

### VS Code Debugger Configuration

<details>
<summary>launch.json (example)</summary>

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(Windows) Launch",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/.build/Debug/opencl-language-server.exe",
            "args": ["clinfo"],
            "stopAtEntry": false,
            "cwd": "${fileDirname}",
            "environment": [],
            "console": "externalTerminal"
        }       
    ]
}
```

</details>

### VS Code Build Task

<details>
<summary>tasks.json (example)</summary>

```json
{
    "tasks": [
        {
            "type": "process",
            "label": "Debug Build",
            "command": "python.exe",
            "args": [
                "build.py",
                "build",
                "-bt",
                "Debug"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "problemMatcher": [
                "$msCompile"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
        }
    ],
    "version": "2.0.0"
}
```

</details/>

## CLI

The `opencl-language-server` offers support for file logging, as well as subcommands that allow you to test its functionality without starting the server.

```
Usage: opencl-language-server [OPTIONS] [SUBCOMMAND]

Options:
  -h,--help                   Print this help message and exit
  -e,--enable-file-logging    Enable file logging
  -f,--log-file TEXT [opencl-language-server.log] 
                              Path to log file
  -l,--log-level ENUM:{0,1,2,3,4,5} [0] 
                              Log level
  --stdio                     Use stdio transport channel for the language server
  -v,--version                Show version

Subcommands:
  clinfo                      Show information about available OpenCL devices
  diagnostics                 Provides an OpenCL kernel diagnostics
```
