from .execute import exec
from .wrapper import Wrapper


class Conan(Wrapper):
    def __init__(self):
        super(Conan, self).__init__("conan")

    def install_dependencies(self, host_profile, install_folder):
        exec(
            [
                self.executable,
                "install",
                ".",
                f"--output-folder={install_folder}",
                *self.__get_configuration_args(host_profile),
            ],
            check=True,
        )

    def __get_configuration_args(self, host_profile):
        return [
            "--update",
            f"--profile:host={host_profile}",
            "--profile:build=default",
            "--build=missing",
        ]
