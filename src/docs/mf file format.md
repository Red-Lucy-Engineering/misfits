# The .MF File Format

The MF (Misfits Font) file format aims to concretely describe a font by defining
every single one of its glyphs as a collection of coordinates that must be
connected with lines.

Files of the MF file format must have:
- A header
- An index of glyphs
- A list of coordinates
- The `.mf` file extension.

It is little endian, and the current version is `MSv01`. Public Domain.


## The header

The header is composed of the version string `MSv01`, zero-terminated, taking
exactly 6 bytes. It is followed by a `uint16_t` encoding the total number of
entries in the glyph index. This count excludes the null terminator entry.

| Field        | Type       | Size    | Description                          |
|--------------|------------|---------|--------------------------------------|
| version      | char[6]    | 6 bytes | Version string, zero-terminated      |
| glyph_count  | uint16_t   | 2 bytes | Number of entries in the glyph index |

Total header size: 8 bytes.


## The index of glyphs

The glyph index immediately follows the header. It is a tightly packed array (no 
padding) of `glyph_count` entries, each of the following shape:

`struct { uint8_t codepoint; uint16_t offset; uint8_t vertex_count; }`

- `codepoint`: The ASCII codepoint of the glyph to define geometry for.
- `offset`: The byte offset from the start of the file pointing to the start of
  the glyph's geometry.
- `vertex_count`: The number of vertices in this glyph's geometry.

The index must be ordered by `codepoint` in ascending order, so that
implementations can perform a binary search to find the right entry efficiently.
`glyph_count` gives the exact bounds needed to do so.


## The geometry

Each glyph's geometry is a sequence of vertices stored at the offset indicated
by its index entry. The number of vertices to read is given by `vertex_count`
in the index entry.

Each vertex is encoded as a single `uint8_t`, split into two 4-bit components:

- **High nibble (bits 7–4):** X coordinate, in the range 0–15.
- **Low nibble (bits 3–0):** Y coordinate, in the range 0–15.

The vertices are interpreted as a polyline: each vertex connects to the next
with a straight line.


## Example implementation flow

### Loading MF files
1. Read and validate the header string (`MSv01`).
2. Read `glyph_count` from the header.
3. Read `glyph_count` index entries into a packed array for binary search.

### When trying to render
1. Iterate through every character in a piece of text.
2. Binary search the glyph index using `glyph_count` as the bounds.
3. Seek to the geometry offset from the index entry.
4. Decode `vertex_count` vertices and draw the stroke as a polyline.
5. Use line rendering routines to draw each stroke.
