import argparse
import logging
import sys

from .controllers import *
from .actions import *


class Command(object):
    default_action_name = "build"

    def __init__(self):
        self.parser = argparse.ArgumentParser(
            formatter_class=argparse.RawTextHelpFormatter
        )
        self.parser.add_argument(
            "-v", "--verbose", action="store_true", help="enable verbose output"
        )
        self.all_actions = [
            "conan-install",
            "configure",
            "build",
        ]
        commands = self.parser.add_subparsers(dest="command", help="available commands")
        commands.add_parser(
            "all",
            help=f"run the following sequence of commands:\n{';'.join(self.all_actions)}",
        )

        build_controller = BuildController()
        self.actions = {
            "conan-install": ConanInstallAction(
                "conan-install", commands, ConanInstallController()
            ),
            "configure": ConfigureAction("configure", commands, ConfigureController()),
            "build": BuildAction("build", commands, build_controller),
        }

    def get_arguments(self):
        return self.parser.parse_args()

    def help(self):
        self.parser.print_help()

    def run(self):
        args = self.get_arguments()
        log_level = logging.DEBUG if args.verbose else logging.INFO
        logging.basicConfig(
            format="builder:%(levelname)s: %(message)s", level=log_level
        )

        try:
            if not args.command:
                name = Command.default_action_name
                args = self.parser.parse_args([name])
                action = self.actions[name]
                action.execute(args)
            elif args.command in self.actions.keys():
                action = self.actions[args.command]
                action.execute(args)
            elif args.command == "all":
                for name in self.all_actions:
                    action = self.actions[name]
                    args = self.parser.parse_args([name])
                    action.execute(args)
        except RuntimeError as err:
            sys.exit(f"RuntimeError: {err}")
