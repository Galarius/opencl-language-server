from pathlib import Path
import logging
import shutil


def validate_folder(folder: Path, clean: bool):
    if clean and folder.exists():
        shutil.rmtree(folder)
        logging.debug(f"Removed '{folder}'")
    if clean or not folder.exists():
        folder.mkdir(parents=True, exist_ok=True)
        logging.debug(f"Created '{folder}'")
