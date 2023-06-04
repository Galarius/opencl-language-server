# Development Notes

## Prerequisites

```shell
python3 -m venv .pyenv
source .pyenv/bin/activate
pip3 install -r requirements.txt
export CONAN_HOME=$(pwd)/.conan-home
conan profile detect
```

## Building the project

| | |
|---|---|
| `./build.py conan-install` | Install dependencies |
| `./build.py configure` | Configure the project |
| `./build.py` | Build the project |
| `./build.py conan-create` | Create the package |
| or | |
| `./build.py all` | Perform all of the above commands |

*Run `./build.py [cmd] --help` to learn more about configurable arguments.*

## macOS

### Build a fat binary (x86_64 + armv8)

```shell
 ./build.py conan-install -o .conan-x86_64 -pr:h profiles/darwin.release.x86_64 -pr:b profiles/darwin.default
 ./build.py configure -t .conan-x86_64/build/Release/generators/conan_toolchain.cmake -b .build-x86_64 -bt Release
 ./build.py build -b .build-x86_64 -bt Release 

 ./build.py conan-install -o .conan-armv8 -pr:h profiles/darwin.release.armv8 -pr:b profiles/darwin.default
 ./build.py configure -t .conan-armv8/build/Release/generators/conan_toolchain.cmake -b .build-armv8 -bt Release
 ./build.py build -b .build-armv8 -bt Release 

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
