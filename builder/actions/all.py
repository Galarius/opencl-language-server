from .action import Action

import argparse


class AllAction(Action):
    def __init__(self, name, root_parser, parser, actions):
        super(AllAction, self).__init__(name, None)
        self.actions = actions
        self.root_parser = root_parser
        subparser = parser.add_parser(
            name,
            help=f"run commands: { list(map(lambda a:a.name, actions)) }",
        )
        subparser.add_argument(
            "-c",
            "--clean",
            action="store_true",
            help="perform fresh actions",
        )
        subparser.add_argument(
            "-w",
            "--with-tests",
            action="store_true",
            help="configure test target",
        )
        subparser.add_argument(
            "-v",
            "--verbose",
            action="store_true",
            help="enable verbose output",
        )

    def execute(self, args):
        all_args_dict = vars(args)
        for action in self.actions:
            if action.name == self.name:
                continue
            command_args_dict = vars(self.root_parser.parse_args([action.name]))
            command_args_dict.update(all_args_dict)
            command_new_args = argparse.Namespace(**command_args_dict)
            action.execute(command_new_args)
