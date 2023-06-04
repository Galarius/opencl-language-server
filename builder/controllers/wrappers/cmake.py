from .execute import exec
from .wrapper import Wrapper
import os


class Cmake(Wrapper):
    def __init__(self):
        super(Cmake, self).__init__("cmake")

    def configure(
        self, configuration, build_folder, toolchain_path, with_tests, verbose, env=None
    ):
        enable_testing = "ON" if with_tests else "OFF"
        cmd = [
            self.executable,
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}",
            f"-DENABLE_TESTING={enable_testing}",
            f"-DCMAKE_BUILD_TYPE={configuration}",
        ]
        if verbose:
            cmd.extend(["--log-level=TRACE", "-Wdev"])
        cmd.extend(["-S", ".", "-B", build_folder])
        if not env:
            env = os.environ.copy()
        exec(cmd, env=env, check=True)

    def build(self, build_folder, verbose):
        cmd = [self.executable, "--build", build_folder]
        if verbose:
            cmd.extend(["--verbose"])
        exec(cmd, check=True)
