#!/usr/bin/env python3
"""Regenerate the environment-variables guide page from Doxygen XML.

The SDK documents every configuration key it reads with a Doxygen
`@note Configuration key: \\`SOME__KEY\\` (default: ...) (owner: sdk|api)`
annotation on the accessor that reads it (see Configuration.h). This script
parses the Doxygen XML output (produced by `doxygen tools/Doxyfile`, which
must run first) for that pattern and writes a single Markdown table, so the
guide page can never drift from the annotations it is generated from.

`owner: sdk` means the key is read directly by the ArmoniK.Extensions.Cpp
library (its name is a plain string literal somewhere in this repo's own
library source, listed in SOURCE_DIRS below). `owner: api` means the
accessor is a pass-through to the underlying ArmoniK.Api ControlPlane
object, which owns and reads the key itself; those keys are intentionally
left out of the table (see ArmoniK.Api's own environment-variables
reference, linked in the page) and are only cross-checked here so a wrongly
tagged owner fails the docs build instead of silently drifting.

This annotation convention is for the shipped library's own accessors only
-- it is not used on test-suite code, which reads a few of its own
configuration keys (e.g. `PartitionId`, `Worker__Type`) that are not part
of the SDK's public surface and are deliberately left undocumented here.

To document a new environment variable, add the same `@note Configuration
key: \\`KEY\\` (default: ... | optional) (owner: sdk|api)` annotation to its
accessor and re-run the docs build; no other step is needed.
"""
import glob
import os
import re
import sys
import xml.etree.ElementTree as ET

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
XML_DIR = os.path.join(REPO_ROOT, ".docs", "content", "cpp", "doxygen", "xml")
OUTPUT_PATH = os.path.join(REPO_ROOT, ".docs", "content", "guide", "4.environment-variables.md")
GITHUB_BLOB = "https://github.com/aneoconsulting/ArmoniK.Extensions.Cpp/blob/main"

# First-party library source directories to scan when cross-checking an "owner"
# tag. Deliberately excludes build/, install/ and .docs/ (which may hold
# vendored or generated copies of ArmoniK.Api headers) and the *.Test
# directories (whose own config keys are a separate, undocumented concern --
# see the module docstring).
SOURCE_DIRS = [
    "ArmoniK.SDK.Common",
    "ArmoniK.SDK.Client",
    "ArmoniK.SDK.Worker",
    "ArmoniK.SDK.DynamicWorker",
]

SKIP_FILES = {"index.xml", "Doxyfile.xml"}
KEY_RE = re.compile(r"Configuration key:\s*`([A-Za-z0-9_]+)`(.*)")
OWNER_RE = re.compile(r"owner:\s*(api|sdk)", re.IGNORECASE)
DEFAULT_RE = re.compile(r"default:\s*([^()]+?)\s*\)", re.IGNORECASE)
OPTIONAL_RE = re.compile(r"\boptional\b", re.IGNORECASE)


def flatten(el):
  """Join all text of an element, including inside nested tags such as <ref>."""
  return "".join(el.itertext()).strip()


def iter_memberdefs():
  for path in sorted(glob.glob(os.path.join(XML_DIR, "*.xml"))):
    if os.path.basename(path) in SKIP_FILES:
      continue
    try:
      root = ET.parse(path).getroot()
    except ET.ParseError:
      continue
    for memberdef in root.iter("memberdef"):
      yield memberdef


def extract_entries():
  entries = {}
  for memberdef in iter_memberdefs():
    name_el = memberdef.find("qualifiedname")
    if name_el is None:
      name_el = memberdef.find("name")
    symbol = flatten(name_el) if name_el is not None else "?"

    brief_el = memberdef.find("briefdescription/para")
    description = flatten(brief_el) if brief_el is not None else ""

    loc = memberdef.find("location")
    file_ref = loc.get("file") if loc is not None else None
    line = loc.get("line") if loc is not None else None

    key_match = None
    other_notes = []
    for note in memberdef.iter("simplesect"):
      if note.get("kind") != "note":
        continue
      para = note.find("para")
      if para is None:
        continue
      text = flatten(para)
      match = KEY_RE.search(text)
      if match and key_match is None:
        key_match = match
      else:
        other_notes.append(text)

    if key_match is None:
      continue

    var_name, tail = key_match.group(1), key_match.group(2)
    where = f"{file_ref}:{line}" if file_ref else symbol

    owner_match = OWNER_RE.search(tail)
    if not owner_match:
      sys.exit(f"Configuration key `{var_name}` ({where}) is missing an (owner: sdk|api) tag")
    owner = owner_match.group(1).lower()

    default_match = DEFAULT_RE.search(tail)
    default_value = default_match.group(1).strip() if default_match else None
    optional = bool(OPTIONAL_RE.search(tail))

    description = " ".join([description] + other_notes) if other_notes else description
    entries.setdefault(var_name, {
        "var": var_name,
        "owner": owner,
        "default": default_value,
        "optional": optional,
        "description": description,
        "symbol": symbol,
        "file": file_ref,
        "line": line,
    })
  return entries


def load_source_text():
  chunks = []
  for source_dir in SOURCE_DIRS:
    for ext in ("*.cpp", "*.h", "*.hpp"):
      for path in glob.glob(os.path.join(REPO_ROOT, source_dir, "**", ext), recursive=True):
        with open(path, encoding="utf-8", errors="ignore") as f:
          chunks.append(f.read())
  return "\n".join(chunks)


def validate_owners(entries, source_text):
  errors = []
  for e in entries.values():
    literal_present = f'"{e["var"]}"' in source_text
    if e["owner"] == "sdk" and not literal_present:
      errors.append(
          f"`{e['var']}` is tagged (owner: sdk) but is not read as a string literal "
          f"anywhere in {'/'.join(SOURCE_DIRS)} -- looks like an (owner: api) pass-through instead")
    if e["owner"] == "api" and literal_present:
      errors.append(
          f"`{e['var']}` is tagged (owner: api) but is read as a string literal directly in "
          f"this repo's library source -- looks like an (owner: sdk) key instead")
  if errors:
    sys.exit("Owner-tag validation failed:\n" + "\n".join(f"  - {msg}" for msg in errors))


def escape_cell(text):
  return text.replace("|", "\\|")


def format_default(entry):
  if entry["default"] is not None:
    return f"`{entry['default']}`"
  if entry["optional"]:
    return "*(optional)*"
  return "*(required)*"


def render(entries):
  rows = sorted((e for e in entries.values() if e["owner"] == "sdk"), key=lambda e: e["var"])
  lines = [
      "<!-- AUTO-GENERATED by tools/docs/generate_env_vars_table.py -- do not edit by hand. -->",
      "<!-- Regenerated from `@note Configuration key:` Doxygen annotations on every docs build. -->",
      "",
      "# Environment variables",
      "",
      "Every configuration key read directly by the ArmoniK.Extensions.Cpp library itself (via "
      "`Configuration::add_env_configuration()`, or the equivalent JSON configuration key). This "
      "table is generated from the `Configuration key:` notes attached to the accessors in the "
      "C++ source: to document a new environment variable, annotate its accessor the same way "
      "and it appears here automatically on the next docs build.",
      "",
      "Some `ControlPlane` accessors (`getEndpoint`, `getUserCertPemPath`, ...) are plain "
      "pass-throughs to the underlying [ArmoniK.Api](https://github.com/aneoconsulting/ArmoniK.Api) "
      "client library, which owns and reads those keys itself (`GrpcClient__Endpoint`, "
      "`GrpcClient__CertPem`/`CertP12`/`KeyPem`/`CaCert`, `GrpcClient__AllowUnsafeConnection`, plus "
      "the gRPC channel's own keep-alive/retry/backoff/timeout keys). Those are intentionally left "
      "out of this table; see "
      "[ArmoniK.Api's environment variables reference]"
      "(https://armonikapi.readthedocs.io/en/latest/content/usage/envars/ArmoniK.Api.EnvVars.html) "
      "for them.",
      "",
      "| Variable | Default | Description | Defined in |",
      "|---|---|---|---|",
  ]
  for e in rows:
    default = format_default(e)
    if e["file"]:
      link = f"[`{e['symbol']}`]({GITHUB_BLOB}/{e['file']}#L{e['line']})"
    else:
      link = f"`{e['symbol']}`"
    lines.append(f"| `{e['var']}` | {escape_cell(default)} | {escape_cell(e['description'])} | {escape_cell(link)} |")
  lines.append("")
  return "\n".join(lines)


def main():
  if not os.path.isdir(XML_DIR):
    sys.exit(f"Doxygen XML not found at {XML_DIR}; run `doxygen tools/Doxyfile` first")

  entries = extract_entries()
  if not entries:
    sys.exit("No `Configuration key:` annotations found in the Doxygen XML")

  validate_owners(entries, load_source_text())

  sdk_count = sum(1 for e in entries.values() if e["owner"] == "sdk")
  os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
  with open(OUTPUT_PATH, "w") as f:
    f.write(render(entries))
  print(f"Wrote {sdk_count} environment variable(s) to {OUTPUT_PATH} "
        f"({len(entries) - sdk_count} owner:api key(s) excluded)")


if __name__ == "__main__":
  main()
