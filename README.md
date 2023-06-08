# OpenCL Language Server

## Supported Capabilities:

- [x] `textDocument/publishDiagnostics`

## Prerequisites

OpenCL Language Server requires OpenCL Runtime [[Intel](https://software.intel.com/en-us/articles/opencl-drivers), [NVidia](http://www.nvidia.com/Download/index.aspx), [AMD](http://support.amd.com/en-us/download)]

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

|||
| --- | --- |
| `buildOptions` | Build options to be used for building the program. The list of [supported](https://registry.khronos.org/OpenCL/sdk/2.1/docs/man/xhtml/clBuildProgram.html) build options. |
| `deviceID` | Device ID or 0 (automatic selection) of the OpenCL device to be used for diagnostics. |
| |  *Run `./opencl-language-server --clinfo` to get information about available OpenCL devices including identifiers.* |
| `maxNumberOfProblems` | Controls the maximum number of problems produced by the language server. |

To enable file logging, pass the following parameters:

## Development

See [development notes](DEV.md).

## License

[MIT License](https://raw.githubusercontent.com/Galarius/opencl-language-server/main/LICENSE)

## Disclaimer

OpenCL is the trademark of Apple Inc.
