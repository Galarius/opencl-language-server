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
        
        output = StringIO()
        self.run("opencl-language-server --version", output)
        assert len(output.getvalue()) > 0
        
        output = StringIO()
        self.run("opencl-language-server --clinfo", output)
        clinfo = json.loads(output.getvalue())
        print(json.dumps(clinfo, indent=2))
