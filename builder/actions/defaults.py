from pathlib import Path

import platform


def map_uname_arch_to_conan_arch(arch):
    map = {"arm64": "armv8", "amd64": "x86_64"}
    return map[arch] if arch in map.keys() else arch


class Defaults(object):
    _uname = platform.uname()
    system = _uname.system.lower()
    machine = _uname.machine.lower()
    debug_build_type = "Debug"
    release_build_type = "Release"
    default_build_types = release_build_type
    supported_build_types = [debug_build_type, release_build_type]
    output_folder = ".conan-install"
    build_folder = ".build"
    current_architecture = map_uname_arch_to_conan_arch(machine)
    default_host_profile = (
        f"profiles/{system}.{default_build_types.lower()}.{current_architecture}"
    )
    default_package_host_profile = (
        f"profiles/{system}.{release_build_type.lower()}.{current_architecture}"
    )
    default_build_profile = f"profiles/{system}.default"

    @staticmethod
    def get_toolchain_path(output_folder, build_type):
        if Defaults.system == "windows":
            path = (
                Path(output_folder)
                / "build"
                / "generators"
                / "conan_toolchain.cmake"
            )
        else:
            path = (
                Path(output_folder)
                / "build"
                / build_type
                / "generators"
                / "conan_toolchain.cmake"
            )
        return str(path)
