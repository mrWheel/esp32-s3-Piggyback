#include <stdint.h>
#include <stdio.h>

//-- Host-side font coverage check. Compile and run on the host (no ESP-IDF
//-- needed) to print every glyph of every built-in face for ASCII 32..126:
//--   cc -I . -o font_check tools/font_check/font_check.c \
//--      components/esp32_s3_piggyback/esp32_s3_piggyback_font.c
//--   ./font_check
//-- Exits with status 1 if any face is missing glyph pixels for a printable
//-- ASCII character, so it can also be used in CI.

uint8_t tft_ec11_font_row(unsigned char c, uint8_t row);
uint8_t tft_ec11_font_medium_row(unsigned char c, uint8_t row);
uint8_t tft_ec11_font_large_row(unsigned char c, uint8_t row);

typedef struct
{
    const char *name;
    int width;
    int rows;
    uint8_t left_bit;
    uint8_t (*row_fn)(unsigned char, uint8_t);
} font_face_t;

static const font_face_t faces[] = {
    { "SMALL  5x7 (cell 6x8)",  5, 7,  1 << 4, tft_ec11_font_row },
    { "MEDIUM 7x12 (cell 8x12)", 7, 12, 1 << 6, tft_ec11_font_medium_row },
    { "LARGE  8x16 (cell 9x16)", 8, 16, 1 << 7, tft_ec11_font_large_row }
};

//-- Render one glyph as ASCII art; '#' marks a lit glyph pixel.
static void print_glyph(const font_face_t *face, unsigned char c)
{
    for (int row = 0; row < face->rows; row++)
    {
        uint8_t bits = face->row_fn(c, (uint8_t)row);
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
