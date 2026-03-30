# The .MF File Format

The MF (Misfits Font) file format aims to concretely describe a font by defining 
every single one of it's glyphs as a collection of coordinates that must be 
connected with lines.

Files of the MF file format must have:
- A header
- An index of glyphs
- A list of coordinates
- The `.mf` file extension.

It is little endian, and the current version is `MSv01`. Public Domain.


## The header.

The header is composed of a singular string that is version-specific, right now 
it is `MSv01`. It may change in the future to cleanly denote breaking changes.

It must be zero-terminated, being exactly 6 bytes in total.


## The index of glyphs

The index of glyphs is defined as tightly packed structure of the following 
shape:

`struct { char codepoint, uint16_t offset }`
- `codepoint`: The glyph character to define geometry for.
- `offset`: The offset (from file start) pointing to geometry.

One must traverse the entire structure until `codepoint` is `\0`.

The index of glyphs must be ordered by `codepoint` from minor to major, so that 
implementations can do a binary search to find the right geometry per codepoint 
in an efficient manner.

The codepoint must be in the ASCII format.


## The geometry.

Each vertex in the geometry must be encoded as a singular byte of an opaque type 
(usually an `uint8_t`) which is split to encode the X, and Y components.

The first 4 bits should be taken and interpreted as the X coordinate, and the 
last 4 bits should be taken and interpreted as the Y coordinate.

Both of them should be mapped to the range of 0 to 15.


## Example implementation flow

### Loading MF files.
1. Check the header, then skip it.
2. Store the glyph index offset or pointer for later

### When trying to render:
1. Iterate through every character in a piece of text
2. Use the glyph index offset to perform a binary search
3. Decode the geometry, coordinate by coordinate
4. Use line rendering routines to draw the geometry
