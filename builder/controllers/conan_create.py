from .wrappers import Conan

import logging


class ConanCreateController(object):
    def __init__(self):
        self.conan = Conan()

    def create(self, host_profile, build_profile):
        logging.debug(
            f"Creating Conan package for profile host:'{host_profile}', build:'{build_profile}'..."
        )
        self.conan.check_availability()
        self.conan.create(host_profile, build_profile)
