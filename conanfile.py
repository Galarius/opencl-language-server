from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import load, copy
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMake, cmake_layout


import os


class OpenCLLanguageServerConanfile(ConanFile):
    name = "opencl_language_server"
    description = "OpenCL Language Server"
    package_type = "application"
    license = "MIT"
    homepage = "https://github.com/Galarius/opencl-language-server"
    topics = ("opencl", "language-server")
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"fPIC": [True, False], "enable_testing": [True, False]}
    default_options = {"fPIC": True, "enable_testing": False}
    requires = (
        "cli11/[^2.3.2]",
        "nlohmann_json/[^3.11.2]",
        "opencl-clhpp-headers/2022.09.30",
        "spdlog/[^1.11.0]",
        "uriparser/[^0.9.7]"
    )
    exports_sources = (
        "include/**",
        "src/**",
        "tests/**",
        "CMakeLists.txt",
        "version",
        "LICENSE",
    )

    def set_version(self):
        version = load(self, os.path.join(self.recipe_folder, "version"))
        self.version = version.strip()

    def requirements(self):
        if not is_apple_os(self):
            self.requires("opencl-icd-loader/2022.09.30")

    def build_requirements(self):
        if self.options.enable_testing:
            self.test_requires("gtest/[^1.13.0]")

    def validate(self):
        check_min_cppstd(self, 17)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure(
            {
                "ENABLE_TESTING": self.options.enable_testing,
            }
        )
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        build_folder = (
            os.path.join(self.build_folder, str(self.settings.build_type))
            if self.settings.os == "Windows"
            else self.build_folder
        )
        bin_ext = ".exe" if self.settings.os == "Windows" else ""
        copy(
            self,
            f"opencl-language-server{bin_ext}",
            build_folder,
            os.path.join(self.package_folder, "bin"),
        )
        copy(
            self,
            "LICENSE",
            self.source_folder,
            os.path.join(self.package_folder, "licenses"),
        )

    def package_info(self):
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []

        bin_ext = ".exe" if self.settings.os == "Windows" else ""
        opencl_ls_bin = os.path.join(
            self.package_folder, "bin", f"opencl-language-server{bin_ext}"
        ).replace("\\", "/")
        self.runenv_info.define_path("OPENCL_LANGUAGE_SERVER", opencl_ls_bin)
