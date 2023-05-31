from .action import Action
from .defaults import Defaults


class BuildAction(Action):
    def __init__(self, name, parser, controller):
        super(BuildAction, self).__init__(name, controller)
        subparser = parser.add_parser(
            name,
            help="build the project",
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
            "--verbose",
            action="store_true",
            help="enable verbose output",
        )

    def execute(self, args):
        kwargs = vars(args)
        self.controller.build(**kwargs)
