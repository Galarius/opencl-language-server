from .execute import exec
from .wrapper import Wrapper


class Conan(Wrapper):
    def __init__(self):
        super(Conan, self).__init__("conan")

    def install_dependencies(
        self, host_profile, build_profile, install_folder, with_tests
    ):
        exec(
            [
                self.executable,
                "install",
                ".",
                "--update",
                f"--output-folder={install_folder}",
                *self.__get_configuration_args(host_profile, build_profile, with_tests),
            ],
            check=True,
        )

    def create(self, host_profile, build_profile):
        exec(
            [
                self.executable,
                "create",
                ".",
                *self.__get_configuration_args(host_profile, build_profile),
            ],
            check=True,
        )

    def __get_configuration_args(self, host_profile, build_profile, with_tests=False):
        args = [
            f"--profile:host={host_profile}",
            f"--profile:build={build_profile}",
            "--build=missing",
        ]
        if with_tests:
            args.extend(
                [
                    "-o",
                    "opencl_language_server/*:enable_testing=True",
                ]
            )
        return args
