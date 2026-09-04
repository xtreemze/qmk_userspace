#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
workflow="$repo_root/.github/workflows/build_binaries.yaml"

python3 - "$workflow" <<'PY'
import pathlib
import re
import sys

workflow = pathlib.Path(sys.argv[1])
text = workflow.read_text(encoding="utf-8")


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def job_block(name: str) -> str:
    match = re.search(rf"(?ms)^  {re.escape(name)}:\n(.*?)(?=^  [A-Za-z0-9_-]+:\n|\Z)", text)
    if not match:
        fail(f"workflow job {name!r} is missing")
    return match.group(0)

prefix = text.split("\njobs:\n", 1)[0]
if not re.search(r"(?m)^permissions:\n  contents: read$", prefix):
    fail("workflow-level contents permission must default to read")

build = job_block("build")
publish = job_block("publish")

if "qmk_userspace_build.yml@" in build:
    fail("firmware compilation must not delegate to the mutable nested reusable build wrapper")

container = "ghcr.io/qmk/qmk_cli@sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a"
if f"container: {container}" not in build:
    fail("firmware build must use the audited qmk_cli image digest")
if "qmk_cli:latest" in text:
    fail("workflow must not use mutable qmk_cli:latest")
if "pip install" in build:
    fail("pinned build container must not re-resolve Python dependencies at runtime")

if not re.search(r"(?m)^    permissions:\n      contents: read$", build):
    fail("firmware build must explicitly remain contents-read-only")
if not re.search(r"(?m)^    permissions:\n      contents: write$", publish):
    fail("only the publish job should escalate contents permission to write")

checkout_sha = "3d3c42e5aac5ba805825da76410c181273ba90b1"
upload_sha = "043fb46d1a93c77aae656e7c1c64a875d1fc6a0a"
publish_workflow_sha = "01daf5113fa50804558f21cc074ab99ba84ddeaf"

if f"uses: actions/checkout@{checkout_sha}" not in build:
    fail("local build checkout action must be pinned by commit SHA")
if f"uses: actions/upload-artifact@{upload_sha}" not in build:
    fail("firmware artifact upload action must be pinned by commit SHA")
if re.search(r"uses:\s+actions/(?:checkout|upload-artifact)@v", build):
    fail("local build must not reference mutable major action tags")
if f"uses: qmk/.github/.github/workflows/qmk_userspace_publish.yml@{publish_workflow_sha}" not in publish:
    fail("publish workflow must remain pinned to the audited QMK workflow commit")

print("CI build boundary: local build inputs are pinned and read-only; publish is the sole contents-write job.")
PY
