# Conan deployer that gathers licenses into the output_folder.
# Execute "conan install . --output-folder=.conan-install --deployer=licenses --build=missing"
# or use "./build.py conan-install"

from conan.tools.files import load, copy
from pathlib import Path
import logging


def deploy(graph, output_folder, **kwargs):
    licenses_dir = Path(output_folder) / "licenses"
    for _, dep in graph.root.conanfile.dependencies.items():
        src = Path(dep.package_folder) / "licenses"
        dst = licenses_dir / str(dep)
        dst.mkdir(exist_ok=True, parents=True)
        logging.info(f"Copy '{src}/*' to '{dst}'")
        copy(graph.root.conanfile, "*", src, dst)

    root_dir = Path(graph.root.path).parent
    version = load(graph.root.conanfile, root_dir / "version").strip()
    ocls_output = licenses_dir / "opencl-language-server" / version
    ocls_output.mkdir(exist_ok=True, parents=True)
    logging.info(f"Copy '{root_dir}/LICENSE' to '{ocls_output}'")
    copy(graph.root.conanfile, "LICENSE", root_dir, ocls_output)
