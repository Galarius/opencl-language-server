# OpenCL Language Server

![Build status](https://github.com/galarius/opencl-language-server/actions/workflows/build.yml/badge.svg)

## Supported Capabilities:

- [x] [`textDocument/publishDiagnostics`](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_publishDiagnostics)
- [x] [`textDocument/completion`](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_completion)
- [x] [`textDocument/definition`](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.18/specification/#textDocument_definition)
- [x] [`textDocument/typeDefinition`](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.18/specification/#textDocument_typeDefinition)
- [x] [`textDocument/declaration`](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.18/specification/#textDocument_declaration)

## Prerequisites

OpenCL Language Server requires OpenCL Runtime [[Intel](https://software.intel.com/en-us/articles/opencl-drivers), [NVidia](http://www.nvidia.com/Download/index.aspx), [AMD](http://support.amd.com/en-us/download)]

* [Latest Microsoft Visual C++ Redistributable Version](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170) (Windows)

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
                "maxNumberOfProblems": 127
            }
        }
    }
}
```

### Options

|||
| --- | --- |
| `buildOptions` | Options to be utilized when building the program. The list of [supported](https://registry.khronos.org/OpenCL/sdk/2.1/docs/man/xhtml/clBuildProgram.html) build options. |
| `deviceID` | Device ID or 0 (automatic selection) of the OpenCL device to be used for diagnostics. |
| |  *Run `./opencl-language-server clinfo` to get information about available OpenCL devices including identifiers.* |
| `maxNumberOfProblems` | Controls the maximum number of errors parsed by the language server. |

## CLI

The language server communicates with a client using JSON-RPC protocol.  
Optionally, you can use subcommands to access functionality without starting the
server.

```
.build/Debug/opencl-language-server [OPTIONS] [SUBCOMMANDS]


OPTIONS:
  -h,     --help              Print this help message and exit
  -e,     --enable-file-logging 
                              Enable file logging
  -f,     --log-file TEXT [opencl-language-server.log]  
                              Path to log file
  -l,     --log-level ENUM:{0,1,2,3,4,5} [0]  
                              Log level
          --stdio             Use stdio transport channel for the language server
  -v,     --version           Show version

SUBCOMMANDS:
  clinfo                      Show information about available OpenCL devices
  diagnostics                 Provides an OpenCL kernel diagnostics
  completion                  Provides an OpenCL kernel completions
  definition                  Resolves the definition location of a symbol at a given text
                              document position
  typedef                     Resolves the type definition location of a symbol at a given text 
                              document position
  declaration                 Resolves the declaration location of a symbol at a given text 
                              document position
```

## Clients

[vscode-opencl](https://github.com/Galarius/vscode-opencl) - OpenCL for Visual Studio Code

## Development

See [development notes](DEV.md).

## License

[MIT License](https://raw.githubusercontent.com/Galarius/opencl-language-server/main/LICENSE)

## Disclaimer

OpenCL is the trademark of Apple Inc.
