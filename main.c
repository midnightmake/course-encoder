#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "font.h"
#include "tomthumb.h"

// - - - defines - - - //

// spacing and sizing properties for the bit encodings
#define NUM_BITS_PER_PAGE 8								// this really shouldn't change
#define PATH_GUIDE_WIDTH 55								// width of beginning and end path
#define PATH_GUIDE_LENGTH 100							// length of starting guide path
#define START_BIT_BLANK_LENGTH 24						// length of blank space indicating beginning of data
#define LOW_BIT_LENGTH (START_BIT_BLANK_LENGTH * 1)		// length of a path indicating a 0 bit
#define HIGH_BIT_LENGTH (START_BIT_BLANK_LENGTH * 2)	// length of a path indicating a 1 bit

// this is the resolution of a 8.5" x 11" 72dpi bitmap, with no margins/borders
#define BITMAP_WIDTH 612
#define BITMAP_HEIGHT 792

// resolution information
#define BITMAP_DPI 72
#define BITMAP_PIXELS_PER_METER 2835

// bitmap header and data sizes
#define BITMAP_INFO_HEADER_SIZE 40
#define BITMAP_DATA_OFFSET 54

// we might have to deal with limitations on file write sizes
#define FILE_CHUNK_SIZE 65535

// - - - types - - - //

// it's very important we pack this structure correctly since we write the
// header information is a single chunked fwrite() single
typedef struct __attribute__((__packed__)) BITMAP
{
	// header
	char signature1;
	char signature2;
	uint32_t fileSize;
	uint32_t reserved1;
	uint32_t dataOffset;

	// info header
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bitsPerPixel;
	uint32_t compression;
	uint32_t imageSize;
	uint32_t xPixelsPerMeter;
	uint32_t yPixelsPerMeter;
	uint32_t colorsUsed;
	uint32_t importantColors;

	// [note]: colour table is skipped for 24-bit colour

	// pixel data
	uint8_t *bytes;
} Bitmap;

// - - - prototypes - - - //

void encodeCourse(int args, char *argv[]);
bool encodeIntoBitmapFile(char *filename, uint8_t page, uint8_t numPages, uint8_t value);
void encodeValue(Bitmap *bitmap, uint8_t page, uint8_t numPages, uint8_t value);
void bitmapText(Bitmap *bitmap, char *text, const Font *font, int16_t offsetX, int16_t offsetY, uint8_t red, uint8_t green, uint8_t blue);
void bitmapRect(Bitmap *bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue);
void bitmapPixel(Bitmap *bitmap, uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue);
Bitmap *bitmapMake(uint32_t width, uint32_t height);
bool saveBitmapToFile(char *filename, Bitmap *bitmap);
void printUsageAndExit();

// - - - implementations - - - //

int main(int args, char *argv[])
{
	// read in args
	if(args > 2)
	{
		encodeCourse(args - 1, &argv[1]);
	}
	else
	{
		printUsageAndExit();
	}

	// exit success
	return 0;
}

void encodeCourse(int args, char *argv[])
{
	char filename[256];
	uint8_t value;
	uint8_t numPages;
	int i;

	// compute number of pages
	numPages = args - 1;

	// make sure values are not out of range
	for(i = 0; i < numPages; ++ i)
	{
		value = atoi(argv[i + 1]);
		if(value >= 64)
		{
			printf("The value %d is out of the supported range. Only values 0 to 64 are supported.\r\n", (int)value);
			exit(1);
		}
	}

	// write each value onto a separate sheet
	for(i = 0; i < numPages; ++ i)
	{
		// build filename
		value = atoi(argv[i + 1]);
		sprintf(filename, "%s_val_%03d_page_%02d.bmp", argv[0], value, i);
		printf("encoding value %03d into %s...", value, filename);

		// encode the value into a bitmap and save it to disk
		if(encodeIntoBitmapFile(filename, i, numPages, value))
		{
			printf("success.\n");
		}
	}

	// outro
	printf("Encoding complete. Please make sure you print with no margins.\n");
}

bool encodeIntoBitmapFile(char *filename, uint8_t page, uint8_t numPages, uint8_t value)
{
	// create a blank bitmap
	Bitmap *bitmap = bitmapMake(BITMAP_WIDTH, BITMAP_HEIGHT);
	if(!bitmap)
	{
		return false;
	}

	// now encode the value into it
	encodeValue(bitmap, page, numPages, value);

	// now save to file
	if(!saveBitmapToFile(filename, bitmap))
	{
		return false;
	}

	// success
	return true;
}

void encodeValue(Bitmap *bitmap, uint8_t page, uint8_t numPages, uint8_t value)
{
	uint32_t rectX;
	uint32_t rectY;
	uint32_t rectWidth;
	uint32_t rectHeight;
	uint32_t yPos;
	uint8_t encoded;
	uint32_t codeLength;

	uint32_t i;
	char str[256];

	// clear to white
	memset(bitmap -> bytes, 255, bitmap -> imageSize);

	// add calibration bits to the beginning of the provided value
	value = (value << 2) | (0x01);

	// compute the vertical center so we can center the code no matter what we write
	codeLength = 0;
	encoded = value;
	for(i = 0; i < NUM_BITS_PER_PAGE; ++ i)
	{
		codeLength += (encoded & 0x80 ? HIGH_BIT_LENGTH : LOW_BIT_LENGTH);
		codeLength += START_BIT_BLANK_LENGTH;
		encoded <<= 1;
	}

	// reset our position
	yPos = 0;

	// encode end path
	rectWidth = PATH_GUIDE_WIDTH;
	rectHeight = (bitmap -> height / 2) - (codeLength / 2);
	rectX = (bitmap -> width / 2) - (rectWidth / 2);
	rectY = 0;
	bitmapRect(bitmap, rectX, rectY, rectWidth, rectHeight, 0, 0, 0);

	// reset our position: we advance a bit for a start/calibration blank marker
	yPos += rectHeight + START_BIT_BLANK_LENGTH;

	// write each bit from MSB to LSB, from top to bottom of the page
	encoded = value;
	for(i = 0; i < NUM_BITS_PER_PAGE; ++ i)
	{
		// compute encoded bit rectangle
		rectWidth = PATH_GUIDE_WIDTH;
		rectHeight = (encoded & 0x80 ? HIGH_BIT_LENGTH : LOW_BIT_LENGTH);
		rectX = (bitmap -> width / 2) - (rectWidth / 2);
		rectY = yPos;
		bitmapRect(bitmap, rectX, rectY, rectWidth, rectHeight, 0, 0, 0);

		// advance
		encoded <<= 1;
		yPos += rectHeight + START_BIT_BLANK_LENGTH;
	}

	// write the remainder of the path; because we're doing it in reverse, this is where the bot enters
	rectWidth = PATH_GUIDE_WIDTH;
	rectHeight = bitmap -> height - yPos;
	rectX = (bitmap -> width / 2) - (rectWidth / 2);
	rectY = yPos;
	bitmapRect(bitmap, rectX, rectY, rectWidth, rectHeight, 0, 0, 0);

	// write page and value information at the bottom
	sprintf(str, "page: %d of %d  |  value: %d  |  bot direction ^", (int)(page + 1), (int)numPages, (int)(value >> 2));
	bitmapText(bitmap, str, (const Font*)&tomThumb, 25, bitmap -> height - 20, 0, 0, 0);
}

void bitmapText(Bitmap *bitmap, char *text, const Font *font, int16_t offsetX, int16_t offsetY, uint8_t red, uint8_t green, uint8_t blue)
{
	// retrieve font properties
	uint8_t first = font -> first;
	uint8_t last = font -> last;
	uint8_t *fontBitmap = font -> bitmap;

	// current char properties
	Glyph *glyph;			// glyph
	uint8_t w, h;			// width and height
	uint8_t xa;				// horizontal cursor advance
	int16_t xo, yo;			// x and y offsets
	uint16_t bo;			// bitmap offset for the current char
	uint8_t bits;			// byte we consume as we write one pixel at a time
	uint8_t bit;			// number of remaining bits to write
	uint8_t i, j;			// glyph bit iterators
	int16_t x, y;			// final computed pixel position

	// reset iterator
	char *curr = text;
	char ch;

	// print one char at a time until we're done
	while(*curr)
	{
		// make sure current char is supported by the current font
		ch = *curr;
		if(ch >= first && ch <= last)
		{
			// retrieve current char glyph
			glyph = &font -> glyph[ch - first];
			w = glyph -> width;
			h = glyph -> height;

			// retrieve offsets and cursor advance
			xo = (int8_t)glyph -> xOffset;
			yo = (int8_t)glyph -> yOffset;
			xa = glyph -> xAdvance;
			bo = glyph -> bitmapOffset;

			// iterate through glyph bits
			bits = 0;
			bit = 0;
			for (j = 0; j < h; j ++)
			{
				for (i = 0; i < w; i ++)
				{
					// compute pixel position
					x = offsetX + xo + i;
					y = offsetY + yo + j;

					// advance to next byte if current one is exhausted
					if (!(bit++ & 7))
					{
						bits = fontBitmap[bo++];
					}

					// set current pixel
					if(bits & 0x80)
					{
						bitmapPixel(bitmap, x, y, red, green, blue);
					}

					// advance to next bit
					bits <<= 1;
				}
			}

			// advance cursor
			offsetX += xa;
		}

		// advance to next char
		curr ++;
	}
}


void bitmapRect(Bitmap *bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue)
{
	uint32_t i, j;

	for(j = y; j < y + height; ++ j)
	{
		for(i = x; i < x + width; ++ i)
		{
			bitmapPixel(bitmap, i, j, red, green, blue);
		}
	}
}

void bitmapPixel(Bitmap *bitmap, uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue)
{
	// flip vertically
	y = bitmap -> height - y;

	// make sure pixel coordinate is in range
	if(x < bitmap -> width && y < bitmap -> height)
	{
		// write the pixel
		bitmap -> bytes[(y * bitmap -> width * 3) + (x * 3) + 0] = blue;
		bitmap -> bytes[(y * bitmap -> width * 3) + (x * 3) + 1] = green;
		bitmap -> bytes[(y * bitmap -> width * 3) + (x * 3) + 2] = red;
	}
}

Bitmap *bitmapMake(uint32_t width, uint32_t height)
{
	Bitmap *bitmap = NULL;

	// attempt to allocate space for the bitmap
	bitmap = (Bitmap*)malloc(sizeof(Bitmap));
	if(bitmap)
	{
		// write header information
		bitmap -> signature1 = 'B';
		bitmap -> signature2 = 'M';
		bitmap -> fileSize = BITMAP_DATA_OFFSET + (width * height * 3);
		bitmap -> reserved1 = 0;
		bitmap -> dataOffset = BITMAP_DATA_OFFSET;

		// write info header
		bitmap -> size = BITMAP_INFO_HEADER_SIZE;
		bitmap -> width = width;
		bitmap -> height = height;
		bitmap -> planes = 1;
		bitmap -> bitsPerPixel = 24;
		bitmap -> compression = 0;
		bitmap -> imageSize = (width * height * 3);
		bitmap -> xPixelsPerMeter = BITMAP_PIXELS_PER_METER;
		bitmap -> yPixelsPerMeter = BITMAP_PIXELS_PER_METER;
		bitmap -> colorsUsed = 256;				// since we use 8-bit colors
		bitmap -> importantColors = 0;			// 0 means all colours

		// color table
		// [not needed]

		// allocate space for the pixels
		bitmap -> bytes = (uint8_t*)malloc(width * height * 3);
		if(!bitmap -> bytes)
		{
			// free bitmap space since we're out of memory for the image data; failure
			printf("failed: could not allocate pixel data\n");
			free(bitmap);
			return NULL;
		}
	}
	else
	{
		printf("failed: could not allocate bitmap structure\n");
	}

	// did we succeed?
	return bitmap;
}

bool saveBitmapToFile(char *filename, Bitmap *bitmap)
{
	FILE *file;
	uint8_t *data;
	size_t bytesLeft;
	size_t chunkSize;
	size_t written;

	// attempt to open the file for writing
	file = fopen(filename, "wb");
	if(file != NULL)
	{
		// write header information
		errno = 0;
		data = (uint8_t*)bitmap;
		bytesLeft = bitmap -> fileSize;
		chunkSize = bitmap -> dataOffset;
		written = fwrite(data, 1, chunkSize, file);
		if(written != chunkSize)
		{
			printf("failed: could not write header information; errno was %d\n", (int)errno);
			fclose(file);
			return false;
		}

		// now write the pixel data in chunks
		data = bitmap -> bytes;
		bytesLeft -= chunkSize;
		while(bytesLeft)
		{
			// compute size of chunk to write, and write it
			errno = 0;
			chunkSize = (bytesLeft > FILE_CHUNK_SIZE ? FILE_CHUNK_SIZE : bytesLeft);
			written = fwrite(data, 1, chunkSize, file);

			// make sure write was successful
			if(written != chunkSize)
			{
				printf("failed: chunk size was %d, but %d was written; errno was %d\n", (int)chunkSize, (int)written, (int)errno);
				fclose(file);
				return false;
			}

			// next chunk
			data += written;
			bytesLeft -= written;
		}
	}

	// success
	return true;
}

void printUsageAndExit()
{
	printf("usage:   course-encoder <output-name> <byte 0> ... [byte n-1]\n");
	printf("example: course-encoder bytes 5 255 180\n\n");
	printf("This utility generates bitmap file(s) encoding the given value(s) with\n");
	printf("one value on each page. When printed, these pages can be read by the\n");
	printf("Pocket Bot's reflectance sensors as it drives over them.\n");
	exit(0);
}
