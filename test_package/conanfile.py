from conan import ConanFile
from conan.tools.build import can_run
from io import StringIO
import json
import os


class TestPackageConan(ConanFile):
    settings = "os", "arch"
    generators = "VirtualRunEnv"
    test_type = "explicit"

    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    def build(self):
        pass  # nothing to build

    def _run_json(self, command):
        """Run a command and parse its stdout as JSON."""
        output = StringIO()
        self.run(command, output)
        text = output.getvalue()
        try:
            return json.loads(text)
        except json.JSONDecodeError as err:
            self.output.error(f"Failed to parse JSON from command: {command}\nOutput was:\n{text}")
            raise err
 
    def _run_expect_failure(self, command, expected_message):
        """Run a command expected to fail, but do not raise if exit code is zero
        """
        output = StringIO()
        ret = self.run(f"{command} 2>&1", output, ignore_errors=True)
        text = output.getvalue()
        if ret != 0:
            assert expected_message in text, (
                f"Expected message '{expected_message}' not found in output of: {command}\n"
                f"Got:\n{text}"
            )
 
    def test(self):
        if not can_run(self):
            return
 
        bin_ext = ".exe" if self.settings.os == "Windows" else ""
        opencl_ls = f"opencl-language-server{bin_ext}"
 
        kernel_path = os.path.join(self.source_folder, "..", "tests", "fixtures", "kernel.cl")
        assert os.path.exists(kernel_path), f"Fixture kernel not found: {kernel_path}"
 
        output = StringIO()
        self.run(f"{opencl_ls} --version", output)
        assert len(output.getvalue()) > 0
 
        # clinfo
        clinfo = self._run_json(f"{opencl_ls} clinfo -p")
        # There are no OpenCL devices in the GitHub-hosted runners, so we expect no OpenCL information to be reported. 
        # GitHub-hosted macOS runners usually have just one platform reported by clinfo, but Linux and Windows runners have none.
        assert len(clinfo["PLATFORMS"]) >= 0

        # diagnostics
        self._run_expect_failure(
            f'{opencl_ls} diagnostics -k "{kernel_path}" --json',
            "Failed to get diagnostics: missing OpenCL device",
        )
 
        # completions
        completions = self._run_json(
            f'{opencl_ls} completion -k "{kernel_path}" --json --cl-std cl1.2 --line 79 --column 20'
        )
        assert len(completions) > 0
        assert completions[0]["kind"] is not None
 
        # definitions
        definitions = self._run_json(
            f'{opencl_ls} definition -k "{kernel_path}" --json --cl-std cl1.2 --line 8 --column 8'
        )
        assert len(definitions) == 1
        assert definitions[0]["targetUri"].endswith("kernel.cl")
        assert definitions[0]["targetRange"]["start"]["line"] == 23
        assert definitions[0]["targetRange"]["start"]["character"] == 0
        assert definitions[0]["targetRange"]["end"]["line"] == 34
        assert definitions[0]["targetRange"]["end"]["character"] == 1
        assert definitions[0]["targetSelectionRange"]["start"]["line"] == 23
        assert definitions[0]["targetSelectionRange"]["start"]["character"] == 4
        assert definitions[0]["targetSelectionRange"]["end"]["line"] == 23
        assert definitions[0]["targetSelectionRange"]["end"]["character"] == 14
 
        # type definitions
        type_definitions = self._run_json(
            f'{opencl_ls} typedef -k "{kernel_path}" --json --cl-std cl1.2 --line 48 --column 14'
        )
        assert len(type_definitions) == 1
        assert type_definitions[0]["targetUri"].endswith("kernel.cl")
        assert type_definitions[0]["targetRange"]["start"]["line"] == 5
        assert type_definitions[0]["targetRange"]["start"]["character"] == 8
        assert type_definitions[0]["targetRange"]["end"]["line"] == 5
        assert type_definitions[0]["targetRange"]["end"]["character"] == 33
        assert type_definitions[0]["targetSelectionRange"]["start"]["line"] == 5
        assert type_definitions[0]["targetSelectionRange"]["start"]["character"] == 8
        assert type_definitions[0]["targetSelectionRange"]["end"]["line"] == 5
        assert type_definitions[0]["targetSelectionRange"]["end"]["character"] == 13
 
        # declarations
        declarations = self._run_json(
            f'{opencl_ls} declaration -k "{kernel_path}" --json --cl-std cl1.2 --line 28 --column 23'
        )
        assert len(declarations) == 1
        assert declarations[0]["targetUri"].endswith("kernel.cl")
        assert declarations[0]["targetRange"]["start"]["line"] == 12
        assert declarations[0]["targetRange"]["start"]["character"] == 8
        assert declarations[0]["targetRange"]["end"]["line"] == 12
        assert declarations[0]["targetRange"]["end"]["character"] == 48
        assert declarations[0]["targetSelectionRange"]["start"]["line"] ==12
        assert declarations[0]["targetSelectionRange"]["start"]["character"] == 8
        assert declarations[0]["targetSelectionRange"]["end"]["line"] == 12
        assert declarations[0]["targetSelectionRange"]["end"]["character"] == 14
