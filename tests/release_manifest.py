#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT = REPO_ROOT / "scripts" / "generate-release-manifest.py"


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp = Path(tmp_dir)
        display = tmp / "splitkb_halcyon_ferris_rev1_xtreemze_final_display.uf2"
        encoder = tmp / "splitkb_halcyon_ferris_rev1_xtreemze_final_encoder.uf2"
        output = tmp / "manifest.json"
        display.write_bytes(b"display firmware\n")
        encoder.write_bytes(b"encoder firmware\n")

        subprocess.run(
            [
                "python3",
                str(SCRIPT),
                "--userspace-sha",
                "a" * 40,
                "--vial-qmk-sha",
                "b" * 40,
                "--qmk-cli-image",
                "ghcr.io/qmk/qmk_cli@sha256:" + "c" * 64,
                "--output",
                str(output),
                str(encoder),
                str(display),
            ],
            check=True,
        )

        manifest = json.loads(output.read_text(encoding="utf-8"))
        assert manifest["schema_version"] == 1
        assert manifest["source"] == {
            "repository": "xtreemze/qmk_userspace",
            "userspace_sha": "a" * 40,
            "vial_qmk_sha": "b" * 40,
        }
        assert manifest["build_environment"]["qmk_cli_image"].endswith("c" * 64)
        assert manifest["hardware_acceptance"] == {
            "status": "not-recorded",
            "tracking_issue": "https://github.com/xtreemze/qmk_userspace/issues/7",
        }

        artifacts = manifest["firmware"]
        assert [artifact["module"] for artifact in artifacts] == ["display", "encoder"]
        assert [artifact["file"] for artifact in artifacts] == sorted([display.name, encoder.name])

        expected = {
            display.name: hashlib.sha256(display.read_bytes()).hexdigest(),
            encoder.name: hashlib.sha256(encoder.read_bytes()).hexdigest(),
        }
        for artifact in artifacts:
            path = display if artifact["file"] == display.name else encoder
            assert artifact["sha256"] == expected[artifact["file"]]
            assert artifact["size_bytes"] == path.stat().st_size

        first = output.read_text(encoding="utf-8")
        subprocess.run(
            [
                "python3",
                str(SCRIPT),
                "--userspace-sha",
                "a" * 40,
                "--vial-qmk-sha",
                "b" * 40,
                "--qmk-cli-image",
                "ghcr.io/qmk/qmk_cli@sha256:" + "c" * 64,
                "--output",
                str(output),
                str(display),
                str(encoder),
            ],
            check=True,
        )
        assert output.read_text(encoding="utf-8") == first


if __name__ == "__main__":
    main()
