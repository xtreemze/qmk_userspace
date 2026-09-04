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

if "qmk/.github/.github/workflows/" in text:
    fail("build and publish must not delegate to externally nested reusable workflows")

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
if "runs-on: ubuntu-24.04" not in build or "runs-on: ubuntu-24.04" not in publish:
    fail("build and publish jobs must stay on the explicit Ubuntu 24.04 runner family")

checkout_sha = "3d3c42e5aac5ba805825da76410c181273ba90b1"
upload_sha = "043fb46d1a93c77aae656e7c1c64a875d1fc6a0a"
download_sha = "3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c"
github_script_sha = "ed597411d8f924073f98dfc5c65a23a2325f34cd"
release_sha = "efb35369e0ad2afab669f228072c1b0d510eae64"

if f"uses: actions/checkout@{checkout_sha}" not in build:
    fail("local build checkout action must be pinned by commit SHA")
if f"uses: actions/upload-artifact@{upload_sha}" not in build:
    fail("firmware artifact upload action must be pinned by commit SHA")
if f"uses: actions/download-artifact@{download_sha}" not in publish:
    fail("firmware artifact download action must be pinned by commit SHA")
if f"uses: actions/github-script@{github_script_sha}" not in publish:
    fail("release/tag update action must be pinned by commit SHA")
if f"uses: softprops/action-gh-release@{release_sha}" not in publish:
    fail("release creation action must be pinned to the resolved v3 commit")

if re.search(r"uses:\s+(?:actions/(?:checkout|upload-artifact|download-artifact|github-script)|softprops/action-gh-release)@v", text):
    fail("workflow must not reference mutable major tags for owned build/release actions")
if "gh release" in publish:
    fail("publish must not depend on the mutable runner gh CLI for release lifecycle")
if "deleteRelease(" in publish or "deleteRef(" in publish:
    fail("publish must not delete the latest release/tag before replacement is validated")

validation_marker = "- name: Validate firmware artifact"
mutation_marker = f"uses: actions/github-script@{github_script_sha}"
release_marker = f"uses: softprops/action-gh-release@{release_sha}"
if validation_marker not in publish:
    fail("publish must validate the downloaded firmware set before mutating latest")
if not (publish.index(validation_marker) < publish.index(mutation_marker) < publish.index(release_marker)):
    fail("firmware validation must precede tag/release mutation")

expected_assets = [
    "splitkb_halcyon_ferris_rev1_xtreemze_final_display.uf2",
    "splitkb_halcyon_ferris_rev1_xtreemze_final_encoder.uf2",
]
for asset in expected_assets:
    if asset not in publish:
        fail(f"publish validation must require expected asset {asset}")

if "updateRef({" not in publish or "force: true" not in publish or "createRef({" not in publish:
    fail("moving latest tag must be updated or created at the exact integration SHA")
if "sha: context.sha" not in publish:
    fail("latest tag mutation must use the exact workflow commit")

if "fail_on_unmatched_files: true" not in publish:
    fail("release creation must fail rather than silently publishing without firmware files")
if "target_commitish: ${{ github.sha }}" not in publish:
    fail("moving latest release must target the exact workflow commit")
if not re.search(r"(?m)^          files: \|\n            \*\*/\*\.uf2\s*$", publish):
    fail("RP2040 release must publish only the UF2 artifact class")
if re.search(r"(?m)^\s+\*\*/\*\.(?:hex|bin)\s*$", publish):
    fail("release must not require unmatched HEX/BIN globs for RP2040 targets")

print("CI boundary: build is read-only and digest-pinned; publish is validation-first, non-destructive, write-scoped, and action-pinned.")
PY
