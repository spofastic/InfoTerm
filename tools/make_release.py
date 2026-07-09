import argparse
import shutil
import subprocess
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import re

ROOT = Path(__file__).resolve().parents[1]
VERSION_FILE = ROOT / "include" / "Version.h"
BUILD_ENV = "esp32_2432s028r"
FIRMWARE_BIN = ROOT / ".pio" / "build" / BUILD_ENV / "firmware.bin"

parser = argparse.ArgumentParser(description="InfoTerm Release-ZIP und OTA-Firmware-Datei erzeugen.")
parser.add_argument(
    "--stable",
    action="store_true",
    help="Markiert die erzeugte Firmware-Datei als 'stable' statt 'nonstable' (nur bei einer vom Nutzer bestaetigten Stable-Version setzen).",
)
args = parser.parse_args()
stability_tag = "stable" if args.stable else "nonstable"


def worktree_state():
    """Returns None if the repo has no commit yet, True if tracked files
    differ from HEAD (dirty), False if clean."""
    head = subprocess.run(
        ["git", "rev-parse", "--verify", "HEAD"],
        cwd=ROOT, capture_output=True, text=True,
    )
    if head.returncode != 0:
        return None
    status = subprocess.run(
        ["git", "status", "--porcelain", "--untracked-files=no"],
        cwd=ROOT, capture_output=True, text=True, check=True,
    ).stdout.strip()
    return bool(status)


# Reproducibility guard (review finding, July 2026): the ZIP ships working-
# tree contents, so a dirty worktree can produce an archive that matches no
# commit/tag. A stable release therefore requires a clean worktree once the
# repo has commits; before the first commit exists this can only warn.
dirty = worktree_state()
if dirty is None:
    print(
        "WARNUNG: Repository hat noch keinen Commit - das ZIP entspricht dem "
        "aktuellen Arbeitsstand und ist nicht gegen einen Tag reproduzierbar."
    )
elif dirty and args.stable:
    raise SystemExit(
        "FEHLER: Arbeitsverzeichnis weicht von HEAD ab - ein Stable-Release "
        "muss aus einem committeten Stand gebaut werden (erst committen/taggen, "
        "dann --stable erneut ausfuehren)."
    )
elif dirty:
    print("WARNUNG: Arbeitsverzeichnis ist nicht sauber - Nonstable-ZIP entspricht keinem Commit.")

text = VERSION_FILE.read_text(encoding="utf-8")
match = re.search(r'INFOTERM_APP_VERSION\s+"([^"]+)"', text)

if not match:
    raise RuntimeError("INFOTERM_APP_VERSION wurde in include/Version.h nicht gefunden.")

version = match.group(1)
zip_version = version.replace(".", "_")

release_dir = ROOT / "releases" / version
release_dir.mkdir(parents=True, exist_ok=True)

zip_path = release_dir / f"InfoTerm_{zip_version}.zip"

# The ZIP ships exactly what git tracks (review finding, July 2026): a
# ROOT.rglob() walk ignored .gitignore, so deliberately excluded content
# (internal process docs, releases/, local artifacts) could leak into the
# published archive. `git ls-files` mirrors the public-repo cut by
# definition. Prerequisite: build-relevant files must be tracked - a fresh
# clone must reproduce the firmware, which is exactly what this enforces.
tracked = subprocess.run(
    ["git", "ls-files"],
    cwd=ROOT,
    capture_output=True,
    text=True,
    check=True,
).stdout.splitlines()

# Belt and braces on top of .gitignore: never ship local secrets, even if
# one of these ever ends up tracked by mistake.
exclude_files = {
    "include/Config.local.h",
    "secrets.h",
    "include/secrets.h",
    "wifi_credentials.h",
    "config.local.h",
}

with ZipFile(zip_path, "w", ZIP_DEFLATED) as archive:
    for rel_posix in tracked:
        if rel_posix in exclude_files:
            continue
        path = ROOT / rel_posix
        if not path.is_file():
            # Staged but deleted from the working tree - nothing to ship.
            continue
        archive.write(path, Path("InfoTerm") / Path(rel_posix))

print(f"Release erstellt: {zip_path} ({len(tracked)} getrackte Dateien geprueft)")

if FIRMWARE_BIN.exists():
    bin_path = release_dir / f"InfoTerm_{zip_version}_{stability_tag}.bin"
    shutil.copy2(FIRMWARE_BIN, bin_path)
    print(f"OTA-Firmware-Datei erstellt: {bin_path}")
else:
    print(
        f"WARNUNG: {FIRMWARE_BIN} nicht gefunden - keine OTA-Firmware-Datei erzeugt. "
        f"Vorher 'platformio run --environment {BUILD_ENV}' ausfuehren."
    )
