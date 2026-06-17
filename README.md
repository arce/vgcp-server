# VGCP Server Renderer

A lightweight vector graphics rendering server with FLTK backend and socket/pipe communication. It parses a custom command-based protocol to create, manipulate, and display scalable vector graphics with support for symbols, styles, transforms, and event handling.

## Features

- **Vector Graphics Primitives**: Rectangle, Circle, Ellipse, Line, Polygon, Polyline, Text, Path, Image
- **Symbol System**: Define reusable symbols with viewboxes and instantiate them with overrides
- **Styles & Inheritance**: CSS-like styling with selector rules (type, class, ID) and inheritance
- **Transform Support**: Matrix, Translate, Rotate, Scale, SkewX, SkewY with composition
- **Image Support**: Base64-encoded PNG, JPEG, GIF, WebP via data URIs
- **Event System**: Mouse events (click, dblclick, mousedown, mouseup, mousemove, mouseenter, mouseleave) broadcast to clients
- **Dual Communication**: TCP socket (port 3891 default) or named pipe
- **Session Management**: Multiple concurrent clients supported

## Protocol

The server listens for commands in the following format:

```
COMMAND key1=value1 key2=value2 ...
```

Each command must be terminated with a newline (`\n`). Commands are case-sensitive.

### Core Commands

| Command | Description |
|---------|-------------|
| `VIEW w=800 h=600 [bg=#000000]` | Set viewport dimensions and background color |
| `CLR` | Clear all elements, keep styles and symbols |
| `RST` | Reset everything (elements, styles, symbols, viewport) |
| `BYE` | Disconnect client |

### Element Commands

| Command | Description |
|---------|-------------|
| `RCT id=foo x=10 y=10 w=100 h=100 [rx=0] [ry=0]` | Rectangle with optional rounded corners |
| `CIR id=foo cx=50 cy=50 r=25` | Circle |
| `ELL id=foo cx=50 cy=50 rx=30 ry=20` | Ellipse |
| `LIN id=foo x1=0 y1=0 x2=100 y2=100` | Line |
| `PAT id=foo d="M10,10 L90,90 ..."` | Path (SVG path syntax) |
| `PGN id=foo pts="0,0 100,0 50,100"` | Polygon |
| `PLN id=foo pts="0,0 100,0 50,100"` | Polyline |
| `TXT id=foo x=50 y=50 txt="Hello" [ff=Helvetica] [fs=14] [ta=s]` | Text (s=start, m=middle, e=end) |
| `IMG id=foo x=10 y=10 href="data:image/png;base64,..." [w=100] [h=100]` | Image via data URI |
| `GRP id=foo` | Group (container for hierarchy) |

### Common Element Attributes

| Attribute | Description |
|-----------|-------------|
| `id` | Unique identifier (required) |
| `grp` | Parent group ID (default: "root") |
| `pos` | `f` = insert at front, `l` = append (default) |
| `class` | Space-separated class names for styling |
| `tr` | Transform string (e.g., `t(10,20) r(45) s(2)`) |
| `fill` | Fill color (named, hex, or `none`) |
| `stroke` | Stroke color (named, hex, or `none`) |
| `sw` | Stroke width |
| `op` | Opacity (0.0 to 1.0, multiplicative with parent) |

### Symbol Commands

| Command | Description |
|---------|-------------|
| `SYM id=foo vb="0 0 100 100" [pr=meet]` | Start symbol definition |
| `SYM id=end` | End symbol definition |
| `USE id=inst sym=foo x=0 y=0 [w=100] [h=100]` | Instantiate a symbol |
| `SYM-DEL id=foo` | Delete a symbol |
| `SYM-DEL all=t` | Delete all symbols |

### Style Commands

| Command | Description |
|---------|-------------|
| `STY sel=.class fill=red stroke=blue sw=2` | Set style rule (type, .class, or #id selector) |
| `STY-DEL sel=.class,#id` | Delete style rules |
| `STY-DEL all=t` | Delete all styles |
| `STY-GET sel=.class` | Get style rule |
| `STY-GET all=t` | Get all style rules |

### Query Commands

| Command | Description |
|---------|-------------|
| `GET id=foo,bar` | Get element details |
| `GET all=t` | Get all elements |
| `EX id=foo` | Check if element exists |
| `ATTR id=foo fill=blue` | Update element attributes |
| `DEL id=foo,bar` | Delete elements |
| `DEL all=t` | Delete all elements |

### Color Support

Colors can be specified as:
- **Named colors**: `red`, `blue`, `green`, `yellow`, `black`, `white`, `orange`, `cyan`, `magenta`, `pink`, `gold`, `purple`, `gray`, `grey`, `brown`, `lime`, `navy`
- **Hex colors**: `#ff0000`, `#f00`, `#ff000080` (with alpha)
- **None**: `none` (transparent)

## Build

### Dependencies

- FLTK 1.3 or later
- C++17 compiler
- POSIX system (Linux/Unix)

### Compilation

```bash
g++ -std=c++17 -o vgcp_server vgcp_server.cpp \
    -lfltk -lfltk_images -lpthread -lm \
    -I/path/to/fltk/include
```

## Usage

### Start Server

```bash
# Socket mode (default port 3891)
./vgcp_server

# Socket mode with custom port
./vgcp_server --port 8080
./vgcp_server 8080

# Pipe mode
./vgcp_server --mode pipe --pipe /tmp/vgcp_pipe
```

### Connect to Server

**Socket mode:**
```bash
nc localhost 3891
```

**Pipe mode:**
```bash
echo "VIEW w=800 h=600 bg=#2a2a2a" > /tmp/vgcp_pipe
```

### Example Session

```
VIEW w=800 h=600 bg=#1a1a2e

STY sel=.btn fill=#e94560 sw=2
STY sel=.btn:hover fill=#ff6b81

RCT id=rect1 x=100 y=100 w=200 h=100 fill=#16213e stroke=#e94560 sw=2 class=btn
TXT id=label1 x=200 y=160 txt="Click me" ff=Helvetica fs=18 fill=white ta=m class=btn

CIR id=circle1 cx=500 cy=150 r=50 fill=#0f3460 stroke=#e94560 sw=3
TXT id=label2 x=500 y=160 txt="Circle" ff=Helvetica fs=14 fill=white ta=m

SYM id=icon vb="0 0 50 50"
  PAT id=star d="M25,5 L30,20 L45,20 L33,30 L38,45 L25,35 L12,45 L17,30 L5,20 L20,20 Z" fill=gold
SYM id=end

USE id=star1 sym=icon x=650 y=100 w=60 h=60
USE id=star2 sym=icon x=650 y=200 w=40 h=40 op=0.7
```

## Architecture

The renderer is built around a `VGCPDocument` class that manages:

- **Element Storage**: Flat array with hierarchical relationships via parent indices
- **Symbol System**: Named symbol templates with their own element trees
- **Style Resolution**: Cascade rules: element attributes → ID selectors → class selectors → type selectors → inherited
- **Transform Pipeline**: Matrix multiplication with parent transforms
- **Image Caching**: Images are cached by element ID and reused

### Drawing Pipeline

1. Parse command → update document state
2. `draw()` → rebuild hierarchy → traverse root elements
3. For each element: resolve styles → apply transform → render with FLTK
4. Children inherit parent's styles and transforms

### Event Flow

1. Mouse event occurs on `VGCPWidget`
2. Hit-test against elements (back-to-front)
3. Broadcast event to all connected clients:
   ```
   EVENT type=click target=rect1 x=150 y=120 gx=150 gy=120 button=left
   ```

## File Structure

| File | Description |
|------|-------------|
| `vgcp_server.cpp` | Entry point, server initialization |
| `server.hpp` | Socket/pipe communication, client session management |
| `document.hpp` | Core document state, command processing |
| `renderer.hpp` | Drawing implementation, hierarchy rebuilding |
| `widget.hpp` | FLTK widget with event handling |
| `types.hpp` | Element and Symbol data structures |
| `transform.hpp` | Transform matrix operations |
| `color.hpp` | Color parsing (named, hex, alpha) |
| `utils.hpp` | Utility functions (parse_int, base64_decode) |
| `globals.hpp` | Global state declarations |

## Notes

- Images must be provided as data URIs with Base64 encoding
- SVG paths support: M, L, H, V, C, S, Q, T, A, Z (absolute and relative)
- Path commands are automatically parsed and rendered with `fl_curve` for bezier curves
- Symbol overrides allow customization of nested element attributes
- Opacity is multiplied across parent-child hierarchy
- The server is designed for single-threaded FLTK main loop with non-blocking I/O

## License

This project is open source. Use it freely for your own purposes.