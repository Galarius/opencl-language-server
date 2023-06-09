from .utils import validate_folder
from .wrappers import Cmake
from pathlib import Path


class BuildController(object):
    __acceptable_keys = [
        "build_folder",
        "build_type",
        "verbose",
    ]

    def build(self, **kwargs):
        [self.__setattr__(key, kwargs.get(key)) for key in self.__acceptable_keys]
        validate_folder(Path(self.build_folder), False)
        cmake = Cmake()
        cmake.check_availability()
        cmake.build(self.build_folder, self.build_type, self.verbose)
