import shutil


class Wrapper(object):
    def __init__(self, executable):
        self.executable = executable

    def check_availability(self):
        if shutil.which(self.executable) is None:
            raise RuntimeError(f"'{self.executable}' is not found on the PATH")
