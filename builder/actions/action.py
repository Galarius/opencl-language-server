class Action(object):
    """A bridge between cli options and action controller"""

    def __init__(self, name, controller):
        """
        Args:
            name: action name
            controller: action controller
        """
        self.name = name
        self.controller = controller

    def execute(self, arguments):
        raise NotImplementedError
