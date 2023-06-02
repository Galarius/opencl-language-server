from .execute import exec
from .wrapper import Wrapper


class Conan(Wrapper):
    def __init__(self):
        super(Conan, self).__init__("conan")

    def install_dependencies(self, host_profile, install_folder, with_tests):
        exec(
            [
                self.executable,
                "install",
                ".",
                f"--output-folder={install_folder}",
                *self.__get_configuration_args(host_profile, with_tests),
            ],
            check=True,
        )

    def __get_configuration_args(self, host_profile, with_tests):
        args = [
            "--update",
            f"--profile:host={host_profile}",
            "--profile:build=default",
            "--build=missing",
        ]
        if with_tests:
            args.extend(
                [
                    "-o",
                    "opencl-language-server/*:enable_testing=True",
                ]
            )
        return args
