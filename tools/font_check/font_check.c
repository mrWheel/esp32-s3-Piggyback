#include <stdint.h>
#include <stdio.h>

//-- Host-side font coverage check. Compile and run on the host (no ESP-IDF
//-- needed) to print every glyph of every built-in face for ASCII 32..126:
//--   cc -I . -o font_check tools/font_check/font_check.c \
//--      components/esp32_s3_piggyback/esp32_s3_piggyback_font.c
//--   ./font_check
//-- Exits with status 1 if any face is missing glyph pixels for a printable
//-- ASCII character, if space is not blank, if an accented letter does not
//-- fold onto its base glyph, if an unknown byte does not render the box
//-- placeholder, or if a glyph has lit pixels outside its face width, so it
//-- can also be used in CI.

uint8_t tft_ec11_font_row(unsigned char c, uint8_t row);
uint8_t tft_ec11_font_medium_row(unsigned char c, uint8_t row);
uint8_t tft_ec11_font_large_row(unsigned char c, uint8_t row);
uint16_t tft_ec11_font_xlarge_row(unsigned char c, uint8_t row);
uint16_t tft_ec11_font_xxlarge_row(unsigned char c, uint8_t row);

typedef struct
{
    const char *name;
    int width;
    int rows;
    uint16_t left_bit;
    uint16_t (*row_fn)(unsigned char, uint8_t);
} font_face_t;

//-- The three original faces return one byte per row; wrap them so every
//-- row function has the same uint16_t signature for the check below.
static uint16_t row_small(unsigned char c, uint8_t row)
{
    return tft_ec11_font_row(c, row);
}

static uint16_t row_medium(unsigned char c, uint8_t row)
{
    return tft_ec11_font_medium_row(c, row);
}

static uint16_t row_large(unsigned char c, uint8_t row)
{
    return tft_ec11_font_large_row(c, row);
}

static const font_face_t faces[] = {
    { "SMALL   5x7  (cell 6x8)",   5, 7,  1 << 4,  row_small },
    { "MEDIUM  7x12 (cell 8x12)",  7, 12, 1 << 6,  row_medium },
    { "LARGE   8x16 (cell 9x16)",  8, 16, 1 << 7,  row_large },
    { "XLARGE  12x24 (cell 13x24)",  12, 24, 1 << 15, tft_ec11_font_xlarge_row },
    { "XXLARGE 16x32 (cell 17x32)",  16, 32, 1 << 15, tft_ec11_font_xxlarge_row }
};

//-- Render one glyph as ASCII art; '#' marks a lit glyph pixel.
static void print_glyph(const font_face_t *face, unsigned char c)
{
    for (int row = 0; row < face->rows; row++)
    {
        uint16_t bits = face->row_fn(c, (uint8_t)row);
        putchar(' ');
        for (int col = 0; col < face->width; col++)
        {
            putchar((bits & (face->left_bit >> col)) ? '#' : '.');
        }
        putchar('\n');
    }
}

static int glyph_has_ink(const font_face_t *face, unsigned char c)
{
    for (int row = 0; row < face->rows; row++)
    {
        if (face->row_fn(c, (uint8_t)row) != 0)
        {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    int failures = 0;

    for (size_t f = 0; f < sizeof(faces) / sizeof(faces[0]); f++)
    {
        const font_face_t *face = &faces[f];
        printf("\n=== face %s ===\n", face->name);

        for (int c = 32; c <= 126; c++)
        {
            printf("%c (0x%02X)\n", (c == 32) ? 'S' : c, c);
            print_glyph(face, (unsigned char)c);
        }

        //-- Coverage: every printable ASCII character except space must have
        //-- at least one lit pixel; space must be completely blank.
        for (int c = 33; c <= 126; c++)
        {
            if (!glyph_has_ink(face, (unsigned char)c))
            {
                printf("FAIL %s: glyph 0x%02X '%c' is empty\n", face->name, c, c);
                failures++;
            }
        }
        if (glyph_has_ink(face, ' '))
        {
            printf("FAIL %s: space glyph is not blank\n", face->name);
            failures++;
        }

        //-- No glyph row may light a pixel outside the face width (bits to
        //-- the right of the last column must be 0, the renderer masks them
        //-- but a stray bit would indicate bad table data).
        int high = 15;
        while (high > 0 && ((face->left_bit >> high) & 1u) == 0) high--;
        uint16_t valid_mask = (uint16_t)(((1u << face->width) - 1u) << (high - face->width + 1));
        uint16_t outside_mask = (uint16_t)~valid_mask;
        for (int c = 32; c <= 126; c++)
        {
            for (int row = 0; row < face->rows; row++)
            {
                uint16_t bits = face->row_fn((unsigned char)c, (uint8_t)row);
                if (bits & outside_mask)
                {
                    printf("FAIL %s: glyph 0x%02X has pixels outside the %d px face width (row %d = 0x%04X)\n",
                           face->name, c, face->width, row, bits);
                    failures++;
                    break;
                }
            }
        }

        //-- Accented Latin-1 letters must fold onto their base glyph.
        for (int row = 0; row < face->rows; row++)
        {
            if (face->row_fn(0xE9, (uint8_t)row) != face->row_fn('E', (uint8_t)row))
            {
                printf("FAIL %s: 0xE9 (e-acute) does not fold onto 'E' at row %d\n", face->name, row);
                failures++;
                break;
            }
        }

        //-- Unknown bytes must render the hollow box placeholder.
        if (!glyph_has_ink(face, 0xA7))
        {
            printf("FAIL %s: unknown byte 0xA7 renders blank instead of a box\n", face->name);
            failures++;
        }
    }

    printf("\n%s\n", failures ? "FONT CHECK FAILED" : "FONT CHECK OK: full ASCII 32..126 coverage on all faces");
    return failures ? 1 : 0;
}
