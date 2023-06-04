from .utils import validate_folder
from .wrappers import Cmake
from pathlib import Path


class ConfigureController(object):
    __acceptable_keys = [
        "clean",
        "build_type",
        "build_folder",
        "toolchain_path",
        "with_tests",
        "verbose",
    ]

    def build(self, **kwargs):
        [self.__setattr__(key, kwargs.get(key)) for key in self.__acceptable_keys]
        validate_folder(Path(self.build_folder), self.clean)
        cmake = Cmake()
        cmake.check_availability()
        cmake.configure(
            self.build_type,
            self.build_folder,
            self.toolchain_path,
            self.with_tests,
            self.verbose,
        )
