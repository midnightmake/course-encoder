#pragma once

#include <stdint.h>

// font glyph data, based on Adafruit font structures
typedef struct GLYPH {
	uint16_t bitmapOffset;		// pointer into bitmap data
	uint8_t width;				// width in pixels
	uint8_t height;				// height in pixels
	uint8_t xAdvance;			// horizontal advance in pixels
	int8_t xOffset;				// x distance from cursor position to upper-left corner
	int8_t yOffset;				// y distance from cursor position to upper-left corner
} Glyph;

// font data, based on Adafruit font structures
typedef struct FONT {
	uint8_t *bitmap;			// bitmap data
	Glyph *glyph;				// glyph data
	uint16_t first;				// first ASCII char
	uint16_t last;				// last ASCII char
	uint8_t yAdvance;			// newline size
} Font;
