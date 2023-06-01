from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import load
from conan.tools.apple import is_apple_os

import os


class OpenCLLanguageServerConanfile(ConanFile):
    name = "opencl-language-server"
    description = "OpenCL Language Server"
    license = "MIT"
    homepage = "https://github.com/Galarius/opencl-language-server"
    topics = ("opencl", "language-server")
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    requires = (
        "nlohmann_json/[^3.11.2]",
        "catch2/[^3.3.2]",
        "spdlog/[^1.11.0]",
        "opencl-clhpp-headers/2022.09.30",
    )

    def set_version(self):
        version = load(self, os.path.join(self.recipe_folder, "version"))
        self.version = version.strip()

    def requirements(self):
        if not is_apple_os(self):
            self.requires("opencl-icd-loader/2022.09.30")
    
    def validate(self):
        check_min_cppstd(self, 17)
