from conan import ConanFile
from conan.tools.build import can_run
from io import StringIO
import json


class TestPackageConan(ConanFile):
    settings = "os", "arch"
    generators = "VirtualRunEnv"
    test_type = "explicit"

    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    def build(self):
        pass  # nothing to build

    def test(self):
        if not can_run(self):
            return        
        
        if self.settings.os == "Windows":
            return  # TODO: Fix "'opencl-language-server.exe' is not recognized as an internal or external command, operable program or batch file."
        
        bin_ext = ".exe" if self.settings.os == "Windows" else ""
        opencl_ls = f"opencl-language-server{bin_ext}"

        output = StringIO()
        self.run(f"{opencl_ls} --version", output)
        assert len(output.getvalue()) > 0
        
        output = StringIO()
        self.run(f"{opencl_ls} --clinfo", output)
        clinfo = json.loads(output.getvalue())
        print(json.dumps(clinfo, indent=2))
