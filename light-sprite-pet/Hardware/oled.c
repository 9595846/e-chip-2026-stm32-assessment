#include "oled.h"
#include "board.h"
#include "config.h"
#include "zh_glyphs.h"
#include <string.h>

static uint8_t g_frame[1024];
static uint8_t g_sent[1024];
static uint8_t g_online;
static uint32_t g_retry_at;
static uint32_t g_last_draw;
static unsigned g_scan_chunk;
static uint8_t g_status_visible;
static uint8_t g_status_fullness;
static uint8_t g_status_happiness;
static uint8_t g_status_energy;
static uint8_t g_status_sleeping;
#if APP_STAGE >= 6
static uint8_t g_diag_visible;
static uint8_t g_diag_mpu_online;
static uint16_t g_diag_light;
static int16_t g_diag_ax, g_diag_ay, g_diag_az;
static int16_t g_diag_gx, g_diag_gy, g_diag_gz;
#endif

static const uint8_t g_letters[26][5] = {
    {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0,0x41,0x7f,0x41,0},
    {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
    {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
};

static const uint8_t g_digits[10][5] = {
    {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1e}
};

static void pixel(int x, int y, uint8_t on)
{
    uint8_t *cell;
    if (x < 0 || x >= 128 || y < 0 || y >= 64) {
        return;
    }
    cell = &g_frame[(unsigned)(y / 8) * 128u + (unsigned)x];
    if (on) {
        *cell |= (uint8_t)(1u << (unsigned)(y % 8));
    } else {
        *cell &= (uint8_t)~(1u << (unsigned)(y % 8));
    }
}

static void line(int x0, int y0, int x1, int y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        int twice;
        pixel(x0, y0, 1u);
        if (x0 == x1 && y0 == y1) { break; }
        twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

static void circle(int cx, int cy, int radius)
{
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        pixel(cx+x,cy+y,1u); pixel(cx+y,cy+x,1u); pixel(cx-y,cy+x,1u); pixel(cx-x,cy+y,1u);
        pixel(cx-x,cy-y,1u); pixel(cx-y,cy-x,1u); pixel(cx+y,cy-x,1u); pixel(cx+x,cy-y,1u);
        y++;
        if (error < 0) { error += 2*y + 1; }
        else { x--; error += 2*(y-x) + 1; }
    }
}

static void fill_circle(int cx, int cy, int radius)
{
    int y;
    for (y = -radius; y <= radius; y++) {
        int x;
        for (x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) { pixel(cx+x, cy+y, 1u); }
        }
    }
}

static uint8_t ascii_column(char value, unsigned column)
{
    if (value >= 'a' && value <= 'z') { value = (char)(value - 'a' + 'A'); }
    if (value >= 'A' && value <= 'Z') { return g_letters[value-'A'][column]; }
    if (value >= '0' && value <= '9') { return g_digits[value-'0'][column]; }
    if (value == '-') { return 0x08u; }
    if (value == '+') { return column == 2u ? 0x3eu : 0x08u; }
    if (value == ':') { return column == 2u ? 0x24u : 0u; }
    if (value == '.') { return column == 2u ? 0x60u : 0u; }
    return 0u;
}

static unsigned draw_ascii(unsigned x, unsigned y, char value)
{
    unsigned column;
    unsigned bit;
    for (column = 0u; column < 5u; column++) {
        uint8_t bits = ascii_column(value, column);
        for (bit = 0u; bit < 7u; bit++) {
            if ((bits & (uint8_t)(1u << bit)) != 0u) {
                pixel((int)(x + column), (int)(y + bit + 4u), 1u);
            }
        }
    }
    return x + 6u;
}

static unsigned draw_chinese(unsigned x, unsigned y, uint16_t code)
{
    unsigned glyph_index;
    unsigned row;
    unsigned column;
    for (glyph_index = 0u; glyph_index < sizeof(g_zh_glyphs)/sizeof(g_zh_glyphs[0]); glyph_index++) {
        if (g_zh_glyphs[glyph_index].code == code) {
            for (row = 0u; row < 16u; row++) {
                for (column = 0u; column < 16u; column++) {
                    if ((g_zh_glyphs[glyph_index].rows[row] & (uint16_t)(0x8000u >> column)) != 0u) {
                        pixel((int)(x + column), (int)(y + row), 1u);
                    }
                }
            }
            break;
        }
    }
    return x + 16u;
}

static void draw_text(unsigned x, unsigned y, const char *text)
{
    const uint8_t *p = (const uint8_t *)text;
    while (*p != 0u && x < 128u) {
        if (*p < 0x80u) {
            x = draw_ascii(x, y, (char)*p++);
        } else if ((p[0] & 0xf0u) == 0xe0u && p[1] != 0u && p[2] != 0u) {
            uint16_t code = (uint16_t)(((uint16_t)(p[0] & 15u) << 12) |
                                       ((uint16_t)(p[1] & 63u) << 6) |
                                       (uint16_t)(p[2] & 63u));
            x = draw_chinese(x, y, code);
            p += 3;
        } else {
            p++;
        }
    }
}

static const char *face_name(PetFace face)
{
    static const char *const names[PET_FACE_COUNT] = {
        "\xE6\x99\xAE""\xE9\x80\x9A""", "\xE5\xBC\x80""\xE5\xBF\x83""", "\xE9\xA5\xA5""\xE9\xA5\xBF""", "\xE5\x9B\xB0""\xE5\x80\xA6""", "\xE7\x9D\xA1""\xE7\x9C\xA0""", "\xE6\x83\x8A""\xE8\xAE\xB6""",
        "\xE7\x94\x9F""\xE6\xB0\x94""", "\xE7\x9C\xA9""\xE6\x99\x95""", "\xE5\xB7\xA6""\xE7\x9C\x8B""", "\xE5\x8F\xB3""\xE7\x9C\x8B""", "\xE8\xBF\x9B""\xE9\xA3\x9F""", "\xE5\xA5\xB6""\xE9\xBE\x99"""
    };
    return names[(unsigned)face < PET_FACE_COUNT ? (unsigned)face : 0u];
}

static void smile_mouth(void)
{
    line(48, 46, 54, 51); line(54, 51, 63, 54); line(63, 54, 65, 54);
    line(65, 54, 74, 51); line(74, 51, 80, 46);
}

static void normal_eyes(int pupil_offset, uint8_t blink)
{
    if (blink) {
        line(35, 33, 51, 33); line(77, 33, 93, 33);
    } else {
        circle(43, 33, 7); circle(85, 33, 7);
        fill_circle(43 + pupil_offset, 34, 2); fill_circle(85 + pupil_offset, 34, 2);
    }
}

/* A 1-bit, original simplified little dragon mascot for the special page. */
static void draw_nailong_mascot(uint32_t now)
{
    int bob = ((now / 450u) & 1u) != 0u ? 1 : 0;
    int y = 2 + bob;

    /* horns, round head, eyes and cheek marks */
    line(77, y + 7, 82, y); line(82, y, 87, y + 7);
    line(105, y + 7, 110, y); line(110, y, 115, y + 7);
    circle(96, y + 18, 17);
    fill_circle(88, y + 15, 3); fill_circle(104, y + 15, 3);
    pixel(86, y + 14, 0u); pixel(102, y + 14, 0u);
    line(91, y + 27, 96, y + 30); line(96, y + 30, 101, y + 27);
    fill_circle(77, y + 23, 2); fill_circle(115, y + 23, 2);

    /* small body, arms, feet and tail: kept inside 128x64 OLED area */
    line(84, y + 34, 82, y + 48); line(82, y + 48, 110, y + 48); line(110, y + 48, 108, y + 34);
    line(83, y + 39, 75, y + 44); line(108, y + 39, 116, y + 44);
    line(87, y + 48, 84, y + 55); line(84, y + 55, 91, y + 55);
    line(104, y + 48, 107, y + 55); line(107, y + 55, 114, y + 55);
    line(109, y + 45, 120, y + 50); line(120, y + 50, 117, y + 55);
}

static void draw_nailong_page(uint32_t now)
{
    draw_text(4u, 7u, "\xE5\xA5\xB6""\xE9\xBE\x99""");
    line(3, 25, 50, 25);
    draw_text(5u, 34u, "HI!");
    draw_nailong_mascot(now);
}

static void draw_value(unsigned x, unsigned y, uint8_t value)
{
    char text[4];
    unsigned used = 0u;
    if (value >= 100u) { text[used++] = '1'; value = (uint8_t)(value - 100u); }
    if (used != 0u || value >= 10u) { text[used++] = (char)('0' + value / 10u); }
    text[used++] = (char)('0' + value % 10u);
    text[used] = '\0';
    draw_text(x, y, text);
}

static void draw_bar(unsigned y, uint8_t value)
{
    unsigned x;
    unsigned fill = ((unsigned)value * 84u) / 100u;
    line(13, (int)y, 99, (int)y); line(13, (int)y + 9, 99, (int)y + 9);
    line(13, (int)y, 13, (int)y + 9); line(99, (int)y, 99, (int)y + 9);
    for (x = 0u; x < fill; x++) {
        line((int)(15u + x), (int)y + 2, (int)(15u + x), (int)y + 7);
    }
}

static void draw_status_page(void)
{
    draw_text(0u, 0u, "\xE5\xAE\xA0""\xE7\x89\xA9""\xE7\x8A\xB6""\xE6\x80\x81""");
    if (g_status_sleeping) { draw_text(80u, 0u, "\xE7\x9D\xA1""\xE7\x9C\xA0"""); }
    draw_text(2u, 20u, "F"); draw_bar(19u, g_status_fullness); draw_value(105u, 20u, g_status_fullness);
    draw_text(2u, 36u, "H"); draw_bar(35u, g_status_happiness); draw_value(105u, 36u, g_status_happiness);
    draw_text(2u, 52u, "E"); draw_bar(51u, g_status_energy); draw_value(105u, 52u, g_status_energy);
}

#if APP_STAGE >= 6
static void draw_signed(unsigned x, unsigned y, int16_t value)
{
    char text[7];
    unsigned used = 0u;
    int32_t number = value;
    uint32_t divisor = 10000u;
    uint8_t started = 0u;
    if (number < 0) { text[used++] = '-'; number = -number; }
    while (divisor != 0u) {
        uint8_t digit = (uint8_t)((uint32_t)number / divisor);
        if (digit != 0u || started || divisor == 1u) {
            text[used++] = (char)('0' + digit);
            started = 1u;
        }
        number %= (int32_t)divisor;
        divisor /= 10u;
    }
    text[used] = '\0';
    draw_text(x, y, text);
}

static void draw_diagnostic_page(void)
{
    draw_text(0u, 0u, "\xE8\xAF\x8A""\xE6\x96\xAD""");
    draw_text(38u, 5u, "MPU:");
    draw_text(62u, 5u, g_diag_mpu_online ? "OK" : "ERR");
    draw_text(87u, 5u, "L:");
    draw_signed(99u, 5u, (int16_t)g_diag_light);
    draw_text(0u, 19u, "AX:"); draw_signed(18u, 19u, g_diag_ax);
    draw_text(65u, 19u, "AY:"); draw_signed(83u, 19u, g_diag_ay);
    draw_text(0u, 31u, "AZ:"); draw_signed(18u, 31u, g_diag_az);
    draw_text(65u, 31u, "GX:"); draw_signed(83u, 31u, g_diag_gx);
    draw_text(0u, 43u, "GY:"); draw_signed(18u, 43u, g_diag_gy);
    draw_text(65u, 43u, "GZ:"); draw_signed(83u, 43u, g_diag_gz);
    draw_text(0u, 56u, "SCL:B10 SDA:B11");
}

static void draw_startup(uint32_t now)
{
    unsigned x;
    unsigned fill = (unsigned)((now * 112u) / 1800u);
    if (fill > 112u) { fill = 112u; }
    draw_text(32u, 8u, "\xE5\x85\x89""\xE5\xBD\xB1""\xE7\xB2\xBE""\xE7\x81\xB5""");
    circle(48, 39, 5); circle(80, 39, 5);
    line(53,50,59,54); line(59,54,69,54); line(69,54,75,50);
    line(7,61,120,61);
    for (x = 0u; x < fill; x++) { pixel((int)(8u + x), 60, 1u); }
}
#endif

static void draw_face(PetFace face, uint32_t now)
{
    uint8_t blink = (uint8_t)((now % 4000u) < 140u);
    switch (face) {
    case PET_FACE_NORMAL:
        normal_eyes(0, blink); smile_mouth();
        pixel(27,44,1u); pixel(29,44,1u); pixel(99,44,1u); pixel(101,44,1u);
        break;
    case PET_FACE_HAPPY:
        line(34,34,43,28); line(43,28,52,34); line(76,34,85,28); line(85,28,94,34);
        smile_mouth(); fill_circle(27,43,2); fill_circle(101,43,2);
        break;
    case PET_FACE_HUNGRY:
        circle(43,33,6); circle(85,33,6); fill_circle(43,35,2); fill_circle(85,35,2);
        circle(64,50,5); line(58,57,70,57);
        break;
    case PET_FACE_SLEEPY:
        line(35,34,51,35); line(77,35,93,34); line(56,50,63,47); line(63,47,72,50);
        draw_text(100,19,"Z");
        break;
    case PET_FACE_SLEEP:
        line(34,34,42,38); line(42,38,51,34); line(76,34,85,38); line(85,38,94,34);
        line(58,50,70,50); draw_text(97,18,"ZZ");
        break;
    case PET_FACE_SURPRISED:
        circle(43,34,8); circle(85,34,8); fill_circle(43,34,2); fill_circle(85,34,2); circle(64,52,7);
        line(104,24,104,30); line(101,27,107,27);
        break;
    case PET_FACE_ANGRY:
        line(34,27,51,33); line(77,33,94,27); fill_circle(43,36,3); fill_circle(85,36,3);
        line(50,54,57,49); line(57,49,71,49); line(71,49,78,54);
        break;
    case PET_FACE_DIZZY:
        line(36,28,50,40); line(50,28,36,40); line(78,28,92,40); line(92,28,78,40);
        line(50,51,57,47); line(57,47,64,52); line(64,52,71,47); line(71,47,78,51);
        break;
    case PET_FACE_LOOK_LEFT:
        normal_eyes(-3, 0u); line(53,50,73,50);
        break;
    case PET_FACE_LOOK_RIGHT:
        normal_eyes(3, 0u); line(55,50,75,50);
        break;
    default:
        line(35,34,51,34); line(77,34,93,34);
        if (((now / 300u) & 1u) != 0u) { circle(64,50,8); }
        else { line(53,47,64,53); line(64,53,75,47); }
        break;
    }
}

static void render(PetFace face, uint8_t auto_mode, uint32_t now)
{
    memset(g_frame, 0, sizeof(g_frame));
#if APP_STAGE >= 6
    if (now < 1800u) { draw_startup(now); return; }
    if (g_diag_visible) { draw_diagnostic_page(); return; }
#endif
    if (g_status_visible) {
        draw_status_page();
        return;
    }
    if (face == PET_FACE_NAILONG) {
        draw_nailong_page(now);
        return;
    }
    draw_text(0u, 0u, "\xE5\x85\x89""\xE5\xBD\xB1""\xE7\xB2\xBE""\xE7\x81\xB5""");
    draw_text(68u, 0u, face_name(face));
    line(0, 17, 127, 17);
    pixel(2,19,1u); pixel(125,19,1u);
    draw_face(face, now);
    if (auto_mode) { draw_text(116u, 5u, "A"); }
#if APP_STAGE >= 6
    if (!g_diag_mpu_online) { draw_text(122u, 5u, "!"); }
#endif
}

void oled_reset_state(void)
{
    g_online = 0u;
    g_retry_at = 0u;
    g_last_draw = 0u;
    g_scan_chunk = 0u;
    memset(g_frame, 0, sizeof(g_frame));
    memset(g_sent, 0xff, sizeof(g_sent));
}

uint8_t oled_is_online(void)
{
    return g_online;
}

void oled_set_pet_status(uint8_t visible, uint8_t fullness,
                         uint8_t happiness, uint8_t energy, uint8_t sleeping)
{
    g_status_visible = visible ? 1u : 0u;
    g_status_fullness = fullness;
    g_status_happiness = happiness;
    g_status_energy = energy;
    g_status_sleeping = sleeping ? 1u : 0u;
}

void oled_set_diagnostics(uint8_t visible, uint16_t light, uint8_t mpu_online,
                          int16_t ax, int16_t ay, int16_t az,
                          int16_t gx, int16_t gy, int16_t gz)
{
#if APP_STAGE >= 6
    g_diag_visible = visible ? 1u : 0u;
    g_diag_light = light;
    g_diag_mpu_online = mpu_online ? 1u : 0u;
    g_diag_ax = ax; g_diag_ay = ay; g_diag_az = az;
    g_diag_gx = gx; g_diag_gy = gy; g_diag_gz = gz;
#else
    (void)visible; (void)light; (void)mpu_online;
    (void)ax; (void)ay; (void)az; (void)gx; (void)gy; (void)gz;
#endif
}

void oled_task(PetFace face, uint8_t auto_mode, uint32_t now)
{
    static const uint8_t init[] = {
        0x00,0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,
        0x20,0x02,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xD9,0xF1,0xDB,0x40,
        0xA4,0xA6,0xAF
    };
    uint8_t command[4];
    uint8_t data[17];
    unsigned attempt;

    if (!g_online) {
        if ((int32_t)(now - g_retry_at) < 0) { return; }
        g_retry_at = now + 1000u;
        if (!bus_write(OLED_ADDRESS, init, sizeof(init))) { return; }
        g_online = 1u;
        memset(g_sent, 0xff, sizeof(g_sent));
    }
    if ((uint32_t)(now - g_last_draw) >= 80u) {
        g_last_draw = now;
        render(face, auto_mode, now);
    }
    for (attempt = 0u; attempt < 64u; attempt++) {
        unsigned offset = g_scan_chunk * 16u;
        unsigned column;
        g_scan_chunk = (g_scan_chunk + 1u) % 64u;
        if (memcmp(g_frame + offset, g_sent + offset, 16u) == 0) { continue; }
        column = offset % 128u + OLED_COLUMN_OFFSET;
        command[0] = 0x00u;
        command[1] = (uint8_t)(0xb0u + offset / 128u);
        command[2] = (uint8_t)(column & 15u);
        command[3] = (uint8_t)(0x10u | (column >> 4));
        data[0] = 0x40u;
        memcpy(data + 1u, g_frame + offset, 16u);
        if (!bus_write(OLED_ADDRESS, command, sizeof(command)) ||
            !bus_write(OLED_ADDRESS, data, sizeof(data))) {
            g_online = 0u;
            g_retry_at = now + 1000u;
            return;
        }
        memcpy(g_sent + offset, g_frame + offset, 16u);
        return;
    }
}

