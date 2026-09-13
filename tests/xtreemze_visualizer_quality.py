#!/usr/bin/env python3
"""Quality-control contract for the Ferris Layer Atlas information architecture."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISUALIZER = ROOT / "docs/visualizer"
INDEX = (VISUALIZER / "index.html").read_text(encoding="utf-8")
STYLES = (VISUALIZER / "atlas-enhancements.css").read_text(encoding="utf-8")
QUALITY_STYLES = (VISUALIZER / "atlas-quality.css").read_text(encoding="utf-8")
QUALITY = (VISUALIZER / "atlas-quality.js").read_text(encoding="utf-8")


def main() -> None:
    # Page hierarchy and assistive-technology boundaries.
    assert '<a class="skip-link" href="#atlasWorkspace">' in INDEX
    assert 'id="atlasWorkspace" tabindex="-1" aria-busy="true"' in INDEX
    assert '<section class="stage" aria-labelledby="layerTitle">' in INDEX
    assert 'class="stage" aria-live=' not in INDEX
    assert 'id="status" class="status" role="status" aria-live="polite"' in INDEX
    assert 'class="detail" aria-labelledby="selectionTitle"' in INDEX
    assert INDEX.index('class="stage"') < INDEX.index('class="detail"')

    # Secondary explanatory material should not compete with the primary atlas.
    assert '<details class="geometry-note">' in INDEX
    assert '<details class="legend-disclosure">' in INDEX
    assert '<link rel="stylesheet" href="atlas-quality.css">' in INDEX
    assert 'atlas-quality.js' in INDEX

    # Loading must gate controls until both the canonical layout and behavior
    # definitions are available. This prevents early render/HID interaction races.
    # Disabled layer controls retain full text contrast while the progress cursor
    # communicates the transient loading state.
    assert 'id="hideAlpha" type="checkbox" checked disabled' in INDEX
    assert 'id="rawMode" type="checkbox" disabled' in INDEX
    assert "button.disabled=!ready" in QUALITY
    assert "$(id).disabled=!ready" in QUALITY
    assert "function behaviorReady()" in QUALITY
    assert "rgb.disabled=!(ready&&behaviorReady())" in QUALITY
    assert "aria-busy',String(!ready&&!failed)" in QUALITY
    assert '.layer-button:disabled{opacity:1;color:var(--text);cursor:progress;transform:none}' in QUALITY_STYLES

    # Layer chooser is grouped and keyboard navigable rather than a flat button wall.
    assert "group.className='layer-family'" in QUALITY
    assert "group.setAttribute('role','group')" in QUALITY
    assert "ArrowLeft" in QUALITY and "ArrowRight" in QUALITY
    assert "Home" in QUALITY and "End" in QUALITY
    assert "aria-current" in QUALITY
    assert '.family{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}' in STYLES

    # Selection feedback must remain near the keyboard and visibly persistent,
    # while layer/display changes must clear stale inspector content. The outline
    # is independent of RGB box-shadow styling so connected profiles cannot hide it.
    assert "detail.after(panel)" in QUALITY
    assert "classList.add('is-selected')" in QUALITY
    assert "function clearSelection()" in QUALITY
    assert "$('detailRaw').textContent='—'" in QUALITY
    assert '.key.is-selected{' in STYLES
    assert '.key.is-selected{outline:2px solid var(--accent);outline-offset:1px}' in QUALITY_STYLES

    # Long behavior inventories are progressively disclosed while current-layer
    # behavior remains immediately visible.
    assert "other configured combos" in QUALITY
    assert "No combo chord is fully present on this layer." in QUALITY
    assert '.other-combos{' in STYLES

    # Inherited keys remain subordinate without becoming unreadable or unselectable.
    assert '.key.transparent{opacity:.46;border-style:dashed}' in STYLES
    assert '.key.transparent:hover,.key.transparent:focus-visible{opacity:.82}' in STYLES

    # Responsive organization and usable touch targets are explicit contracts.
    assert '@media(max-width:1100px)' in STYLES
    assert '@media(max-width:700px)' in STYLES
    assert '.layer-family .layer-button{min-height:40px}' in STYLES
    assert '.switch,.rgb-connect{min-height:40px}' in STYLES

    print("Ferris visualizer quality contract passed.")


if __name__ == "__main__":
    main()
