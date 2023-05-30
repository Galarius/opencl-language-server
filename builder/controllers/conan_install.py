import logging
import shutil
from pathlib import Path
from .wrappers import Conan


class ConanInstallController(object):
    def __init__(self):
        self.conan = Conan()

    def install_conan_dependencies(self, host_profile, output_folder, clean=False):
        logging.debug(f"Installing Conan dependencies for profile '{host_profile}'...")
        self.__validate_install_dir(Path(output_folder), clean)
        self.conan.check_availability()
        self.conan.install_dependencies(host_profile, output_folder)

    def __validate_install_dir(self, output_folder: Path, clean: bool):
        if clean:
            shutil.rmtree(output_folder)
            logging.debug(f"Removed '{output_folder}'")
        if clean or not output_folder.exists():
            output_folder.mkdir(parents=True, exist_ok=True)
            logging.debug(f"Created '{output_folder}'")
