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
        commands = self.parser.add_subparsers(dest="command", help="available commands")
        actions = [
            ConanInstallAction("conan-install", commands, ConanInstallController()),
            ConfigureAction("configure", commands, ConfigureController()),
            BuildAction("build", commands, BuildController()),
        ]
        self.actions = {
            *actions,
            AllAction("all", self.parser, commands, actions)
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
                key = Command.default_action_name
                args = self.parser.parse_args([key])
            else:
                key = args.command
            action = next(a for a in self.actions if a.name == key)
            action.execute(args)
        except RuntimeError as err:
            sys.exit(f"RuntimeError: {err}")
