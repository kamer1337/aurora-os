# Aurora OS - Crystalline 5x7 Font Encoding

## Overview

This document describes the 5x7 bitmap font encoding added to Aurora OS as a compact, futuristic alternative to the existing 8x8 font. The font features a modern, angular, crystalline aesthetic inspired by sci-fi and alien technology while maintaining excellent readability.

## Font Specifications

- **Size**: 5 pixels wide × 7 pixels tall
- **Style**: Angular, crystalline, futuristic aesthetic
- **Encoding**: Each character uses 7 bytes, with 5 bits per byte (bits 0-4)
- **Character Set**: Full ASCII printable characters (32-126)
  - Uppercase letters: A-Z (sharp, angular forms)
  - Lowercase letters: a-z (compact, modern curves)
  - Digits: 0-9 (diamond and crystal shapes)
  - Punctuation and symbols: ! @ # $ % ^ & * ( ) - + = [ ] { } ; : ' " , . / < > ? | \ ~ `

## Design Philosophy

The Crystalline 5x7 font combines:
- **Angular Geometry**: Sharp edges and precise angles for a tech-forward look
- **Crystal Patterns**: Diamond and geometric shapes reminiscent of crystalline structures
- **Alien Aesthetics**: Unique character forms that feel otherworldly yet familiar
- **Readability**: Despite the futuristic styling, all characters remain clearly distinguishable

## Character Pattern Examples

### Letter 'A' (ASCII 65) - Sharp Crystalline Triangle
```
Binary Pattern (5x7):
  █      → 0x04
 █ █     → 0x0A
█   █    → 0x11
█   █    → 0x11
█████    → 0x1F
█   █    → 0x11
█   █    → 0x11

Encoded: {0x04, 0x0A, 0x11, 0x11, 0x1F, 0x11, 0x11}
```

### Digit '0' (ASCII 48) - Diamond Crystal with Inner Details
```
Binary Pattern (5x7):
 ███     → 0x0E
█ █ █    → 0x15
█ █ █    → 0x15
█ █ █    → 0x15
█ █ █    → 0x15
█ █ █    → 0x15
 ███     → 0x0E

Encoded: {0x0E, 0x15, 0x15, 0x15, 0x15, 0x15, 0x0E}
```

### Digit '5' (ASCII 53) - Angular Transformation
```
Binary Pattern (5x7):
█████    → 0x1F
█        → 0x10
████     → 0x1E
    █    → 0x01
    █    → 0x01
█   █    → 0x11
 ███     → 0x0E

Encoded: {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}
```

## API Functions

### `framebuffer_draw_char_5x7()`
Draws a single character using the 5x7 font.

**Signature:**
```c
void framebuffer_draw_char_5x7(uint32_t x, uint32_t y, char c, 
                               color_t fg_color, color_t bg_color);
```

**Parameters:**
- `x`: X coordinate (left edge)
- `y`: Y coordinate (top edge)
- `c`: Character to draw (ASCII 0-127)
- `fg_color`: Foreground color for the character
- `bg_color`: Background color

### `framebuffer_draw_string_5x7()`
Draws a string using the 5x7 font.

**Signature:**
```c
void framebuffer_draw_string_5x7(uint32_t x, uint32_t y, const char* str, 
                                 color_t fg_color, color_t bg_color);
```

**Parameters:**
- `x`: X coordinate (left edge)
- `y`: Y coordinate (top edge)
- `str`: Null-terminated string to draw
- `fg_color`: Foreground color for the text
- `bg_color`: Background color

**Features:**
- Automatic character spacing (6 pixels per character = 5 for glyph + 1 for spacing)
- Line wrapping when text exceeds screen width
- Support for newline (`\n`) and tab (`\t`) characters

## Usage Example

```c
#include "kernel/gui/framebuffer.h"

// Initialize framebuffer
framebuffer_init(1024, 768, 32);

// Draw text with 5x7 font
framebuffer_draw_string_5x7(10, 10, "Hello World!", 
                            COLOR_WHITE, COLOR_BLACK);

// Draw comparison with 8x8 font
framebuffer_draw_string(10, 30, "8x8: The quick brown fox", 
                       COLOR_WHITE, COLOR_BLACK);
framebuffer_draw_string_5x7(10, 40, "5x7: The quick brown fox", 
                            COLOR_GREEN, COLOR_BLACK);
```

## Advantages of Crystalline 5x7 Font

1. **Compact Size**: 45% fewer pixels per character than 8x8 font (5×7=35 pixels vs 8×8=64 pixels)
2. **More Text**: Can fit ~60% more characters on screen
3. **Lower Memory**: Uses fewer bytes per character (7 bytes vs 8 bytes)
4. **Futuristic Aesthetic**: Angular, crystalline design evokes sci-fi and alien technology
5. **Modern Look**: Perfect for cyberpunk, space-themed, and tech-forward UIs
6. **Maintains Readability**: Despite stylistic design, patterns remain clear and distinct

## Comparison Table

| Feature | 8x8 Font | 5x7 Font |
|---------|----------|----------|
| Width | 8 pixels | 5 pixels |
| Height | 8 pixels | 7 pixels |
| Total pixels | 64 | 35 |
| Bytes per char | 8 | 7 |
| Chars per line (1024px) | 128 | 170 |
| Lines per screen (768px) | 96 | 109 |

**Note:** Character spacing calculations:
- 8x8 font: 8 pixels per character with no spacing = 1024/8 = 128 characters per line
- 5x7 font: 6 pixels per character (5 for glyph + 1 for spacing) = 1024/6 = 170 characters per line
- Vertical spacing: Both fonts use their respective heights (8 or 7 pixels) per line without additional vertical spacing

## Implementation Details

The 5x7 font is implemented as a static array in `kernel/gui/framebuffer.c`:

```c
static const uint8_t font5x7[128][7] = {
    // Character definitions with crystalline, angular patterns...
};
```

Each character's pattern is defined with:
- **Angular geometry** for a futuristic, tech-forward appearance
- **Crystal-inspired shapes** particularly visible in digits (diamond '0', etc.)
- **Sharp edges and precise forms** creating an alien-like aesthetic
- **Clear distinction** between similar characters (e.g., 'O' vs '0', 'I' vs 'l', '1' vs 'l')
- **Balanced visual weight** across the character set
- **Consistent baseline alignment** for lowercase letters
- **Proper descenders** for characters like 'g', 'j', 'p', 'q', 'y'

### Design Details

**Uppercase Letters**: Feature sharp angles, crystalline triangles, and geometric precision
**Lowercase Letters**: Maintain modern curves with angular touches
**Digits**: Designed with diamond/crystal shapes and angular paths
**Symbols**: Enhanced with futuristic geometric patterns

## Testing

Font tests are included in `tests/font_tests.c` and validate:
- Proper rendering of all character classes (uppercase, lowercase, digits, symbols)
- Correct spacing and alignment
- Color rendering
- String wrapping behavior
- Comparison with existing 8x8 font

Run tests with:
```bash
make test
```

## Future Enhancements

Potential improvements for future versions:
- Variable-width font support for even more efficient space usage
- Bold and italic variants
- Additional font sizes (3x5 for ultra-compact, 7x9 for enhanced readability)
- Unicode character support
- Anti-aliasing for smoother appearance on high-DPI displays
