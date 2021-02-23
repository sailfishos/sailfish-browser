#!/usr/bin/env python3
#
# Generates translation strings from gecko property files for lupdate
# to process

import os, os.path, sys, re, dataclasses, functools

PREFIX = "sailfish_components_webview_popups-la"

class Generator:
    RE = re.compile(r"(?P<name>[\w-]+)\s*=\s*(?P<text>.*)\s*")
    SPLITTER = re.compile(r"([A-Za-z0-9][a-z0-9]*)")
    PLACEHOLDER = re.compile(r"%(?:([0-9]+)\$)?S")

    def __init__(self, file):
        self.file = file

    @property
    @functools.cache
    def name(self):
        """Basename from self.file without file extension and
        with dashes replaced by underscores"""
        return os.path.splitext(os.path.basename(self.file))[0].replace("-", "_")

    @staticmethod
    def _convert_key(name):
        """Convert from camelCase to snake_case"""
        return "_".join(Generator.SPLITTER.findall(name)).lower()

    @staticmethod
    def _convert_text(text):
        """Replace gecko's placeholders with Qt's"""
        def replacer(match):
            return "%1" if match.group(1) is None else "%" + match.group(1)
        new = Generator.PLACEHOLDER.sub(replacer, text)
        while new != text:
            text = new
            new = Generator.PLACEHOLDER.sub(replacer, text)
        return new

    def process_line(self, line):
        match = self.RE.match(line)
        if match is not None:
            return Translation(self.name,
                               self._convert_key(match.group('name')),
                               self._convert_text(match.group('text')))

    def __iter__(self):
        try:
            with open(self.file) as file:
                for line in file:
                    result = self.process_line(line)
                    if result is not None:
                        yield result
        except UnicodeDecodeError:
            print(f"Invalid character in file: {self.file}", file=sys.stderr)
            raise

@dataclasses.dataclass
class Translation:
    name: str
    key: str
    text: str

    def __str__(self):
        return f"""
//% "{self.text}"
const auto {self.name}_{self.key} = qtTrId("{PREFIX}-{self.name}_{self.key}");"""

def generate(translations):
    print("/* Generated dummy file, do not edit */")
    for file in translations:
        for translation in Generator(file):
            print(translation)
    print("/* End of translations */")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <property file> ...", file=sys.stderr)
        sys.exit(2)
    for file in sys.argv[1:]:
        if not os.path.isfile(file):
            print(f"{sys.argv[1]}: Is not a file", file=sys.stderr)
            sys.exit(1)
    generate(sys.argv[1:])
