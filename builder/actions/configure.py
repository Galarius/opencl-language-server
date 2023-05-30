from .action import Action
from .defaults import Defaults


class ConfigureAction(Action):
    def __init__(self, name, parser, controller):
        super(ConfigureAction, self).__init__(name, controller)
        subparser = parser.add_parser(
            name,
            help="configure the project",
        )
        subparser.add_argument(
            "-c",
            "--clean",
            action="store_true",
            help="perform fresh configuration",
        )
        subparser.add_argument(
            "-bt",
            "--build-type",
            choices=Defaults.supported_configurations,
            default=Defaults.default_configuration,
            help="build type [default: %(default)s]",
        )
        subparser.add_argument(
            "-b",
            "--build-folder",
            default=Defaults.build_folder,
            help="the root build folder for Cmake generated and build files [default: %(default)s]",
        )
        subparser.add_argument(
            "-t",
            "--toolchain-path",
            default=Defaults.toolchain_path,
            help="toolchain file path [default: %(default)s]",
        )
        subparser.add_argument(
            "-v",
            "--verbose",
            action="store_true",
            help="c",
        )

    def execute(self, args):
        kwargs = vars(args)
        kwargs["compiler_flags"] = Defaults.compiler_flags
        self.controller.build(**kwargs)
