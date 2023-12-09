#!/usr/bin/env python

import freetype
import re
from sys import argv

FONT_FILE_PATH = argv[1]
FONT_PIXEL_SIZE = int(argv[2])
FIRST_CHAR = ' '
LAST_CHAR = '~'
TAB_STR = ' ' * 2

def main():
    ClangGenerator(FONT_FILE_PATH, FONT_PIXEL_SIZE)

class ClangGenerator(object):
    def __init__(self, fontFile_path, pixelSize_int):
        self.pixelSize_int = pixelSize_int
        self.face = freetype.Face(fontFile_path)
        self.face.set_pixel_sizes(0, pixelSize_int)
        self.h_file = open(self._getHeaderName(), 'w')
        self.c_file = open(self._getSourceName(), 'w')
        self.generateCHeader()
        self.generateCSource()

    def generateCHeader(self):
        self.h_file.write('// Header (.h) for font: {}\n\n'.format(self._getFriendlyName()))
        self.h_file.write('#include <stdint.h>\n\n'.format(self._getFriendlyName()))
        self.h_file.write('extern const int TALLEST_CHAR_PIXELS;\n\n'.format(self._getHeightOfTallestCharacter()))
        self.h_file.write('extern const uint8_t {}_pixels[];\n\n'.format(self._getSafeName()))
        self.h_file.write('struct font_char {\n')
        self.h_file.write('{}int offset;\n'.format(TAB_STR))
        self.h_file.write('{}int w;\n'.format(TAB_STR))
        self.h_file.write('{}int h;\n'.format(TAB_STR))
        self.h_file.write('{}int left;\n'.format(TAB_STR))
        self.h_file.write('{}int top;\n'.format(TAB_STR))
        self.h_file.write('{}int advance;\n'.format(TAB_STR))
        self.h_file.write('};\n\n')
        self.h_file.write('extern const struct font_char {}_lookup[];\n'.format(self._getSafeName()))

    def generateCSource(self):
        self.c_file.write('// Source (.c) for font: {}\n\n'.format(self._getFriendlyName()))
        self.c_file.write('#include "{}"\n\n'.format(self._getHeaderName()))
        self.c_file.write('const int TALLEST_CHAR_PIXELS = {};\n\n'.format(self._getHeightOfTallestCharacter()))
        self._generateLookupTable()
        self._generatePixelTable()

    def _generateLookupTable(self):
        self.c_file.write('const struct font_char {}_lookup[] = {{\n'.format(self._getSafeName()))
        self.c_file.write('{}// offset, width, height, left, top, advance\n'.format(TAB_STR))
        offset = 0
        for j in range(128):
            if j in range(ord(FIRST_CHAR), ord(LAST_CHAR) + 1):
                char = chr(j)
                char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
                self._generateLookupEntryForChar(char, offset, w, h, left, top, advance)
                offset += w * h
            else:
                self.c_file.write('{}{{0, 0, 0, 0, 0}},\n'.format(TAB_STR))
        self.c_file.write('{}{{0, 0, 0, 0, 0}}\n'.format(TAB_STR))
        self.c_file.write('};\n\n')

    def _generateLookupEntryForChar(self, char, offset, w, h, left, top, advance):
        self.c_file.write('{}{{{}, {}, {}, {}, {}, {}}}, // {} ({})\n'
                          .format(TAB_STR, offset, w, h, left, top, advance, char, ord(char)))

    def _generatePixelTable(self):
        self.c_file.write('const uint8_t {}_pixels[] = {{\n'.format(self._getSafeName()))
        self.c_file.write('{}// width, height, left, top, advance\n'.format(TAB_STR))
        for i in range(ord(FIRST_CHAR), ord(LAST_CHAR) + 1):
            char = chr(i)
            self._generatePixelTableForChar(char)
        self.c_file.write('{}0x00\n'.format(TAB_STR))
        self.c_file.write('};\n\n')

    def _generatePixelTableForChar(self, char):
        char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
        self.c_file.write('{}// {} ({})\n'.format(TAB_STR, char, ord(char)))
        if not buf:
            return ''
        for y in range(char_bmp.rows):
            self.c_file.write('{}'.format(TAB_STR))
            self._hexLine(buf, y, w, pitch)
            self.c_file.write(' // ')
            self._asciiArtLine(buf, y, w, pitch)
            self.c_file.write('\n')

    def _getHeightOfTallestCharacter(self):
        tallest = 0
        for i in range(ord(FIRST_CHAR), ord(LAST_CHAR) + 1):
            char = chr(i)
            char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
            if top - h  > tallest:
                tallest = top - h
        return tallest

    def _getChar(self, char):
        char_bmp = self._renderChar(char) # Side effect: updates self.face.glyph
        assert(char_bmp.pixel_mode == 2) # 2 = FT_PIXEL_MODE_GRAY
        assert(char_bmp.num_grays == 256) # 256 = 1 byte per pixel
        w, h = char_bmp.width, char_bmp.rows
        left = self.face.glyph.bitmap_left
        top = FONT_PIXEL_SIZE - self.face.glyph.bitmap_top
        advance = self.face.glyph.linearHoriAdvance >> 16
        buf = char_bmp.buffer # very slow (which is misleading)
        return char_bmp, buf, w, h, left, top, advance, char_bmp.pitch

    def generateCSourceForLine(self, char):
        pass

    def _renderChar(self, char):
        self.face.load_char(char, freetype.FT_LOAD_RENDER)
        return self.face.glyph.bitmap

    def _hexLine(self, buf, y, w, pitch):
        for x in range(w):
            v = buf[x + y * pitch]
            self.c_file.write('0x{:02x},'.format(v))

    def _asciiArtLine(self, buf, y, w, pitch):
        for x in range(w):
            v = buf[x + y * pitch]
            self.c_file.write(" .:-=+*#%@"[int(v / 26)])

    def _getHeaderName(self):
        return '{}.h'.format(self._getSafeName())

    def _getSourceName(self):
        return '{}.c'.format(self._getSafeName())

    def _getSafeName(self):
        return '{}_font'.format(re.sub(r'[^\w\d]+', '_', self._getFriendlyName().lower()))

    def _getFriendlyName(self):
        return '{} {} {}'.format(self.face.family_name, self.face.style_name, self.pixelSize_int)

if __name__ == '__main__':
    main()
