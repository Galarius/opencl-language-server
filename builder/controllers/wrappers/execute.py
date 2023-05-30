import logging
import subprocess


def exec(cmd, **kwargs):
    logging.info(f"Executing cmd: {' '.join(cmd)}")
    logging.debug(f"with kwargs: {kwargs}")
    return subprocess.run(cmd, **kwargs)
