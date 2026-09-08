#!/usr/bin/env python3
"""Generate deterministic provenance metadata for built Halcyon firmware artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


SCHEMA_VERSION = 1


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def infer_module(path: Path) -> str:
    stem = path.stem
    if stem.endswith("_display"):
        return "display"
    if stem.endswith("_encoder"):
        return "encoder"
    return "unknown"


def build_manifest(args: argparse.Namespace) -> dict[str, object]:
    firmware = []
    for path in sorted((Path(value) for value in args.firmware), key=lambda item: item.name):
        if not path.is_file():
            raise FileNotFoundError(f"firmware artifact does not exist: {path}")
        firmware.append(
            {
                "file": path.name,
                "module": infer_module(path),
                "sha256": sha256_file(path),
                "size_bytes": path.stat().st_size,
            }
        )

    if not firmware:
        raise ValueError("at least one firmware artifact is required")

    return {
        "schema_version": SCHEMA_VERSION,
        "source": {
            "repository": args.repository,
            "userspace_sha": args.userspace_sha,
            "vial_qmk_sha": args.vial_qmk_sha,
        },
        "build_environment": {
            "qmk_cli_image": args.qmk_cli_image,
        },
        "hardware_acceptance": {
            "status": args.hardware_acceptance_status,
            "tracking_issue": args.hardware_acceptance_issue,
        },
        "firmware": firmware,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", default="xtreemze/qmk_userspace")
    parser.add_argument("--userspace-sha", required=True)
    parser.add_argument("--vial-qmk-sha", required=True)
    parser.add_argument("--qmk-cli-image", required=True)
    parser.add_argument("--hardware-acceptance-status", default="not-recorded")
    parser.add_argument("--hardware-acceptance-issue", default="https://github.com/xtreemze/qmk_userspace/issues/7")
    parser.add_argument("--output", default="firmware-release-manifest.json")
    parser.add_argument("firmware", nargs="+")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    manifest = build_manifest(args)
    output = Path(args.output)
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
