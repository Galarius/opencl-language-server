import platform


def map_uname_arch_to_conan_arch(arch):
    map = {"arm64": "armv8"}
    return map[arch] if arch in map.keys() else arch


class Defaults(object):
    _uname = platform.uname()
    system = _uname.system.lower()
    machine = _uname.machine.lower()
    debug_configuration = "Debug"
    release_configuration = "Release"
    default_configuration = debug_configuration
    supported_configurations = [debug_configuration, release_configuration]
    output_folder = ".conan"
    build_folder = ".build"
    toolchain_path = f"{output_folder}/conan_toolchain.cmake"
    current_architecture = map_uname_arch_to_conan_arch(machine)
    default_host_profile = (
        f"profiles/{system}.{default_configuration.lower()}.{current_architecture}"
    )
    _compiler_flags_map = {
        "windows": ["/WX", "/W4", "/EHsc"],
        "linux": [
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wimplicit-fallthrough",
            "-static-libstdc++",
            "-static-libgcc",
        ],
        "darwin": ["-Wall", "-Wextra", "-Werror", "-Wimplicit-fallthrough"],
    }
    compiler_flags = _compiler_flags_map[system]
