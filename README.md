# Overview
This is a very simple CLI tool for generating barcode courses for the Pocket Bot that have information embedded into them. It's the same tool that we used to generate the course seen in the **Barcode** [example program](https://www.midnightmake.com/pages/pocket-bot-example-programs) demo.

## Compiling
We provided a Makefile. On Linux or Mac, a simple

`make`

should be all that is required.

If you're on Windows, we recommend installing and using the [MSYS2 tools and related compiler](https://www.msys2.org). In this case, you'll probably run

`mingw32-make`

to compile the course encoder.

Assuming you have a suitable GCC compiler and your `PATH` is set correctly, you should now see the resulting `course-encoder` executable.

## Running

`course-encoder` accepts as parameters the base filename, and the bytes to encode. For example, the command

`./course-encoder mycourse 5 0 25 55`

will generate 4 bitmaps beginning with the filename `mycourse`, with the values `5`, `0`, `25`, and `55` on individual pages.

Only values between `0` and `63` are accepted.

In addition to the course itself, each page is rendered with a footer containing the page number, the total number of pages, the value encoded, and the intended travel direction.
