"""Stage the developer DMG, notices and complete working source snapshot."""
from pathlib import Path
import shutil
import subprocess
import tarfile

root = Path(__file__).resolve().parent.parent
stage = root / "build/dmg-stage"
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir(parents=True)
shutil.copytree(root / "build/Aurora JTTY.app", stage / "Aurora JTTY.app")
(stage / "Applications").symlink_to("/Applications")
for name in ("COPYING", "THIRD_PARTY_NOTICES.md", "docs/PUBLICATION-REQUIREMENTS.md", "docs/MACOS-INSTALL.md", "upstream/wsjtx/TRADEMARK.md", "upstream/wsjtx/DOCS-LICENSE.md"):
    shutil.copy2(root / name, stage / Path(name).name)
files = subprocess.check_output(["git", "ls-files", "-z"], cwd=root).decode().split("\0")
with tarfile.open(stage / "Aurora-JTTY-1.1-source.tar.gz", "w:gz") as archive:
    for name in files:
        path = root / name
        if not name or name.startswith("installers/") or path.is_dir():
            continue
        archive.add(path, arcname="Aurora-JTTY-1.1/" + name, recursive=False)
    # Include the pinned upstream contents, not merely the Git submodule pointer.
    for path in sorted((root / "upstream/wsjtx").rglob("*")):
        rel = path.relative_to(root)
        if ".git" in rel.parts or not path.is_file():
            continue
        upstream_rel = path.relative_to(root / "upstream/wsjtx")
        # Non-source samples and unused demonstration binaries are not needed to build this app.
        if upstream_rel.parts[0] == "samples" and upstream_rel.parts[1] != "JTTY":
            continue
        if str(upstream_rel).startswith("contrib/QDarkStyleSheet/screenshots/"):
            continue
        if path.suffix.lower() in (".dll", ".exe"):
            continue
        archive.add(path, arcname="Aurora-JTTY-1.1/" + str(rel), recursive=False)
print(stage)
