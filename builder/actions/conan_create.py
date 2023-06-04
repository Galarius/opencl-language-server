from .action import Action
from .defaults import Defaults


class ConanCreateAction(Action):
    def __init__(self, name, parser, controller):
        super(ConanCreateAction, self).__init__(name, controller)
        subparser = parser.add_parser(
            name,
            help="create the opencl-language-server Conan package",
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

    def execute(self, args):
        self.controller.create(args.host_profile, args.build_profile)
