# OpenCL Language Server

## Supported Capabilities:

- [x] `textDocument/publishDiagnostics`

## Build

[Build Instructions](_dev/build.md)

## Prerequisites

OpenCL Language Server requires OpenCL Runtime [[Intel](https://software.intel.com/en-us/articles/opencl-drivers), [NVidia](http://www.nvidia.com/Download/index.aspx), [AMD](http://support.amd.com/en-us/download)]

## Dependencies

* [Catch2](https://github.com/catchorg/Catch2.git)
* [Boost.Json](https://github.com/boostorg/json.git)

## Parameters

You can configure diagnostics with `json-rpc` request during the intitialization:

```json
{
    "jsonrpc": "2.0",
    "id": 0,
    "method": "initialize",
    "params": {
        "initializationOptions": {
            "configuration": {
                "buildOptions": [],
                "deviceID": 0,
                "maxNumberOfProblems": 100
            }
        }
    }
}
```

### Options

* `buildOptions` - Build options to be used for building the program. The list of [supported](https://registry.khronos.org/OpenCL/sdk/2.1/docs/man/xhtml/clBuildProgram.html) build options.

* `deviceID` - OpenCL device identifier or 0 (auto selection). 

    *Run `./opencl-language-server --clinfo` to get information about available OpenCL devices including identifiers.*

* `maxNumberOfProblems` - Controls the maximum number of problems produced by the language server.

To enable file logging, pass the following parameters:

```
./opencl-language-server --enable-file-tracing --filename <path to the log file> --level <level [0-4]>
```

*Level: `0 - Trace, 1 - Debug, 2 - Info, 3 - Warn, 4 - Error`*

## Clients

[vscode-opencl](https://github.com/Galarius/vscode-opencl)

## License

[MIT License](https://raw.githubusercontent.com/Galarius/opencl-language-server/main/LICENSE.txt)

## Disclaimer

OpenCL is the trademark of Apple Inc.
