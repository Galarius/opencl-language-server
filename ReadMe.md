# OpenCL Language Server

- [x] `textDocument/publishDiagnostics`

## Build

[Build Instructions](_dev/build.md)

## Prerequisites

OpenCL Language Server requires OpenCL Runtime [[Intel](https://software.intel.com/en-us/articles/opencl-drivers), [NVidia](http://www.nvidia.com/Download/index.aspx), [AMD](http://support.amd.com/en-us/download)]

## Dependencies

* [Catch2](https://github.com/catchorg/Catch2.git)
* [Boost.Json](https://github.com/boostorg/json.git)

## Parameters

To enable server logging, pass the following options on the startup: `--enable-file-tracing --filename <path to the log file> --level <0-4>`

You can also configure diagnostics with json-rpc request during the intitialization:

```json
"initializationOptions": {
    "configuration": {
        "buildOptions": [],
        "maxNumberOfProblems": 100
    }
}
```

## Clients

[vscode-opencl](https://github.com/Galarius/vscode-opencl)

## License

[MIT License](https://raw.githubusercontent.com/Galarius/opencl-language-server/main/LICENSE.txt)

## Disclaimer

OpenCL is the trademark of Apple Inc.
