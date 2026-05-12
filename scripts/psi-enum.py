#!/usr/bin/env python

import argparse
import re


def to_snake_case(s: str):
    return "_".join(
        filter(
            lambda s: len(s) > 0,
            re.sub(r"([A-Z]+)([a-z0-9]+)", r"\1\2-", s).split("-"),
        )
    ).lower()


class CppEnum:
    def __init__(self, name: str):
        self.name = name
        self.snake_name = to_snake_case(self.name)
        self.codes = []
        self.names = []

    def validate(self):
        if len(self.codes) != len(self.names):
            raise RuntimeError(
                f"lengths don't match: {self.codes} ({len(self.codes)}) and {self.names} ({len(self.names)})"
            )

    def enum_to_string(self, default=None):
        if not default:
            default = self.codes[-1]

        s = f"std::string {self.snake_name}_to_string({self.name} v) "
        s += "{\n"
        s += f"  using enum {self.name};\n\n"
        s += "  switch(v) {\n"

        for i in range(0, len(self.codes)):
            s += f"  case {self.names[i]}:\n"
            s += f'    return "{self.codes[i]}";\n'

        s += "  default:\n"
        s += f'   return "{default}";\n'
        s += "  }\n"
        s += "}\n"
        return s

    def enum_from_string(self, default=None):
        if not default:
            default = self.names[-1]

        s = f"{self.name} {self.snake_name}_from_string(const std::string_view& s) "
        s += "{\n"

        s += f"  using enum {self.name};\n\n"
        for i in range(0, len(self.codes)):
            if i == 0:
                s += "  if "
            else:
                s += "  } else if "

            s += f'(s == "{self.codes[i]}") '
            s += "{\n"
            s += f"    return {self.names[i]};\n"

        s += "  } else {\n"
        s += f"    return {default};\n"
        s += "  }\n}\n"
        return s


if __name__ == "__main__":
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        prog="psi-enum",
        description="Code generator",
    )

    parser.add_argument("file")
    parser.add_argument("--msi", action="store_true")
    parser.add_argument("--default-name", type=str)
    parser.add_argument("--default-code", type=str)

    args = parser.parse_args()
    enums = []

    with open(args.file) as file:
        for line in file:
            if m := re.match(r"^\s*enum\s+class\s+(\w+)\s", line):
                enums.append(CppEnum(m.group(1)))
            if m := re.match(r"^\s+///\s+([A-Z]{2}:[0-9]+)\n", line) and args.msi:
                enums[-1].codes.append(m.group(1))
            elif m := re.match(r"^\s+([A-Z][A-Za-z0-9]+),", line):
                enums[-1].names.append(m.group(1))

                if not args.msi:
                    enums[-1].codes.append(to_snake_case(m.group(1)))

    for enum in enums:
        enum.validate()
        print(enum.enum_to_string(default=args.default_code))
        print(enum.enum_from_string(default=args.default_name))
