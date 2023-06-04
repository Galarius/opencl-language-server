from .action import Action
from .defaults import Defaults


class ConanInstallAction(Action):
    def __init__(self, name, parser, controller):
        super(ConanInstallAction, self).__init__(name, controller)
        subparser = parser.add_parser(
            name,
            help="install the requirements specified in a recipe (conanfile.py)",
        )
        subparser.add_argument(
            "-o",
            "--output-folder",
            default=Defaults.output_folder,
            help="the root output folder for Conan to store generated and build files [default: %(default)s]",
        )
        subparser.add_argument(
            "-c",
            "--clean",
            action="store_true",
            help="perform fresh installation",
        )
        subparser.add_argument(
            "-pr:h",
            "--host-profile",
            type=str,
            default=Defaults.default_host_profile,
            help="a Conan profile to apply to the host machine [default: %(default)s]",
        )
        subparser.add_argument(
            "-pr:b",
            "--build-profile",
            type=str,
            default=Defaults.default_build_profile,
            help="a Conan profile to apply to the build machine [default: %(default)s]",
        )
        subparser.add_argument(
            "-w",
            "--with-tests",
            action="store_true",
            help="install test-related requirements",
        )

    def execute(self, args):
        self.controller.install_conan_dependencies(
            args.host_profile,
            args.build_profile,
            args.output_folder,
            args.with_tests,
            args.clean,
        )
