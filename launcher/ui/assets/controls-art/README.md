# Controller artwork

Adapted from the owner-provided Idea 3 refined prototype, 2026-10-10.
The editable SVG sources retain the prototype silhouettes. Dynamic highlights,
bindings and hit targets are rendered separately by RmlUi. PNG previews accompany
uncompressed 32-bit TGA assets for the current DX11 texture loader.

Do not bake gameplay action labels into these images.

## Prototype fidelity assets

chips.png / chips.tga contain the original prototype's 124 physical-button
glyph variants (Xbox, PlayStation, generic and mouse, selected and unselected).
chips.json records atlas rectangles. They were rendered from the owner-supplied
Chip.dc.html at 44 logical pixels and 2x density. Keyboard labels remain native
text, so arbitrary bindings are supported.

highlights.svg retains the exact editable selection paths from XboxArt, PSArt
and KMDiagram; PNG/TGA and highlights.json are its packed native equivalents.
The generated C++ rectangle tables include native atlas positions and percentage
placement over each device illustration. Hit targets and gameplay action labels
remain native RmlUi elements. Physical symbols may be baked into the glyph atlas;
gameplay action labels must not be.

The supplied HTML prototype is a design-time reference only. No React, browser
engine, prototype simulation buttons or network font loading ships in the UI.

The toolbar controller outline (toolbar-pad.svg) is taken from the supplied
ControlsPrototype template. ui-details.svg contains the matching radio and
scope symbols and scalable selection frames. Both have 4x raster copies for
RmlUi's texture loader. The selection frames use native nine-patch decorators,
so resizing stretches the straight edges and preserves the rounded corners.
