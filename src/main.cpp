#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"

std::stringstream debugLogStream;

namespace {
constexpr int kWidth = 1920;
constexpr int kHeight = 1080;
constexpr int kGameX = 240;
constexpr int kGameWidth = 1440;

struct Glyph { char ch; uint8_t rows[7]; };
static const Glyph kFont[] = {
    {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
    {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
    {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
    {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
    {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
    {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
    {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
    {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
    {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
    {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
    {':',{0,4,4,0,4,4,0}}, {'-',{0,0,0,31,0,0,0}},
    {'/',{1,2,2,4,8,8,16}}, {' ',{0,0,0,0,0,0,0}}
};

Color rgb(int r, int g, int b)
{
    Color c = {
        static_cast<uint8_t>(std::max(0, std::min(255, r))),
        static_cast<uint8_t>(std::max(0, std::min(255, g))),
        static_cast<uint8_t>(std::max(0, std::min(255, b)))
    };
    return c;
}

Color blend(Color a, Color b, float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    return rgb(
        static_cast<int>(a.r + (b.r - a.r) * t),
        static_cast<int>(a.g + (b.g - a.g) * t),
        static_cast<int>(a.b + (b.b - a.b) * t)
    );
}

void rect(Scene2D *s, int x, int y, int w, int h, Color c)
{
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(kWidth, x + w);
    const int y1 = std::min(kHeight, y + h);
    if (x1 > x0 && y1 > y0) s->DrawRectangle(x0, y0, x1 - x0, y1 - y0, c);
}

void text(Scene2D *s, const char *value, int x, int y, int scale, Color color)
{
    for (const char *p = value; *p; ++p) {
        char ch = *p;
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        const Glyph *glyph = &kFont[sizeof(kFont) / sizeof(kFont[0]) - 1];
        for (const Glyph &candidate : kFont) {
            if (candidate.ch == ch) { glyph = &candidate; break; }
        }
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph->rows[row] & (1u << (4 - col)))
                    rect(s, x + col * scale, y + row * scale, scale, scale, color);
            }
        }
        x += 6 * scale;
    }
}

void panel(Scene2D *s, int x, int y, int w, int h, Color fill, Color edge)
{
    rect(s, x, y, w, h, fill);
    rect(s, x, y, w, 3, edge);
    rect(s, x, y + h - 3, w, 3, edge);
    rect(s, x, y, 3, h, edge);
    rect(s, x + w - 3, y, 3, h, edge);
}

int gx(int sourceX) { return kGameX + (sourceX * 9) / 2; }
int gy(int sourceY) { return (sourceY * 9) / 2; }
void gameRect(Scene2D *s, int x, int y, int w, int h, Color c)
{
    const int x0 = gx(x), y0 = gy(y);
    const int x1 = gx(x + w), y1 = gy(y + h);
    rect(s, x0, y0, x1 - x0, y1 - y0, c);
}

void drawBackdrop(Scene2D *s, bool enabled, int glow)
{
    rect(s, 0, 0, kWidth, kHeight, rgb(5, 7, 17));
    if (!enabled) return;

    const Color top = rgb(25, 38, 68);
    const Color middle = rgb(38, 19, 55);
    const Color bottom = rgb(8, 23, 38);

    for (int i = 0; i < 24; ++i) {
        const float t = i / 23.0f;
        const Color c = t < 0.5f
            ? blend(top, middle, t * 2.0f)
            : blend(middle, bottom, (t - 0.5f) * 2.0f);
        const Color dim = blend(rgb(4, 5, 12), c, 0.42f + glow * 0.015f);
        const int y0 = (i * kHeight) / 24;
        const int y1 = ((i + 1) * kHeight) / 24;
        rect(s, 0, y0, kGameX, y1 - y0 + 1, dim);
        rect(s, kGameX + kGameWidth, y0, kGameX, y1 - y0 + 1, dim);
    }

    // Soft horizontal light bands make the side-fill read as a blurred scene,
    // while keeping every pixel of the actual game viewport untouched.
    for (int i = 0; i < 9; ++i) {
        const int y = 85 + i * 112;
        const int h = 22 + (i % 3) * 8;
        const int r = 35 + (i * 11) % 45;
        const int g = 32 + (i * 7) % 35;
        const int b = 86 + (i * 13) % 70;
        const Color band = rgb(r, g, b);
        rect(s, 0, y, kGameX, h, blend(rgb(5, 7, 17), band, 0.18f));
        rect(s, kGameX + kGameWidth, y, kGameX, h, blend(rgb(5, 7, 17), band, 0.18f));
    }

    rect(s, kGameX - 5, 0, 5, kHeight, rgb(27, 34, 61));
    rect(s, kGameX + kGameWidth, 0, 5, kHeight, rgb(27, 34, 61));
}

void drawFighter(Scene2D *s, int x, int y, Color mainColor, Color highlight)
{
    gameRect(s, x + 5, y, 12, 13, highlight);       // head
    gameRect(s, x + 2, y + 14, 18, 28, mainColor); // torso
    gameRect(s, x - 4, y + 18, 7, 24, highlight);  // left arm
    gameRect(s, x + 19, y + 18, 7, 24, highlight);// right arm
    gameRect(s, x + 4, y + 42, 6, 27, mainColor); // leg
    gameRect(s, x + 13, y + 42, 6, 27, mainColor);
    gameRect(s, x - 1, y + 67, 12, 5, highlight);
    gameRect(s, x + 11, y + 67, 12, 5, highlight);
}

void drawGameTestPattern(Scene2D *s, bool grid, bool safeFrame)
{
    gameRect(s, 0, 0, 320, 240, rgb(8, 11, 27));

    // A stylized, original test scene: no commercial game artwork is bundled.
    for (int i = 0; i < 12; ++i) {
        const float t = i / 11.0f;
        const Color c = blend(rgb(16, 28, 59), rgb(83, 27, 85), t);
        gameRect(s, 0, i * 13, 320, 14, c);
    }

    gameRect(s, 0, 134, 320, 106, rgb(12, 15, 28));
    gameRect(s, 0, 142, 320, 3, rgb(58, 43, 86));

    for (int i = 0; i < 9; ++i) {
        const int y = 151 + i * i * 1;
        gameRect(s, 0, y, 320, 1, rgb(30 + i * 2, 39 + i * 2, 67 + i * 3));
    }
    for (int i = -6; i <= 6; ++i)
        gameRect(s, 158 + i * 16, 145, 1, 95, rgb(30, 39, 64));

    // Bright central focus and symmetry markers help reveal distortion.
    gameRect(s, 152, 66, 16, 72, rgb(20, 26, 44));
    gameRect(s, 155, 73, 10, 58, rgb(45, 77, 111));
    gameRect(s, 126, 116, 68, 3, rgb(242, 183, 87));
    gameRect(s, 158, 105, 4, 24, rgb(255, 224, 145));

    drawFighter(s, 62, 60, rgb(25, 129, 190), rgb(105, 215, 241));
    drawFighter(s, 226, 60, rgb(178, 59, 111), rgb(255, 165, 179));

    gameRect(s, 16, 16, 288, 3, rgb(241, 191, 94));
    gameRect(s, 16, 20, 288, 1, rgb(94, 151, 190));
    gameRect(s, 16, 222, 288, 2, rgb(241, 191, 94));

    text(s, "KI1 WIDE TEST", gx(20), gy(29), 5, rgb(255, 239, 196));
    text(s, "320 X 240", gx(20), gy(205), 3, rgb(173, 197, 226));

    if (grid) {
        for (int x = 40; x < 320; x += 40)
            gameRect(s, x, 0, 1, 240, rgb(45, 75, 105));
        for (int y = 40; y < 240; y += 40)
            gameRect(s, 0, y, 320, 1, rgb(45, 75, 105));
        gameRect(s, 159, 0, 2, 240, rgb(245, 220, 150));
        gameRect(s, 0, 119, 320, 2, rgb(245, 220, 150));
    }

    if (safeFrame) {
        gameRect(s, 8, 8, 304, 1, rgb(255, 255, 255));
        gameRect(s, 8, 231, 304, 1, rgb(255, 255, 255));
        gameRect(s, 8, 8, 1, 224, rgb(255, 255, 255));
        gameRect(s, 311, 8, 1, 224, rgb(255, 255, 255));
    }
}

void drawTestScene(Scene2D *s, bool backdrop, bool grid,
                   bool safeFrame, int glow)
{
    drawBackdrop(s, backdrop, glow);
    drawGameTestPattern(s, grid, safeFrame);

    // Keep all test labels inside the 240-pixel side gutters so the
    // central 4:3 content area remains completely unobstructed.
    panel(s, 16, 24, 208, 104, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "KI1 16:9", 28, 42, 3, rgb(240, 247, 255));
    text(s, "TEST", 28, 76, 3, rgb(240, 247, 255));

    panel(s, 16, 176, 208, 392, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "CROSS", 28, 197, 3, rgb(232, 237, 248));
    text(s, "SIDES", 28, 228, 3, rgb(167, 194, 226));
    text(s, "TRIANGLE", 28, 285, 2, rgb(232, 237, 248));
    text(s, "GRID", 28, 316, 3, rgb(167, 194, 226));
    text(s, "SQUARE", 28, 373, 3, rgb(232, 237, 248));
    text(s, "SAFE", 28, 404, 3, rgb(167, 194, 226));
    text(s, "LEFT RIGHT", 28, 461, 2, rgb(232, 237, 248));
    text(s, "LIGHT", 28, 492, 3, rgb(167, 194, 226));
    text(s, "CIRCLE", 28, 525, 3, rgb(232, 237, 248));

    panel(s, 1696, 24, 208, 104, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "4:3 AREA", 1708, 42, 3, rgb(240, 247, 255));
    text(s, "1440X1080", 1708, 78, 2, rgb(167, 194, 226));
}

void renderBuffer(Scene2D *s, int buffer, bool backdrop, bool grid,
                  bool safeFrame, int glow)
{
    s->SetActiveFrameBuffer(buffer);
    drawTestScene(s, backdrop, grid, safeFrame, glow);
}

bool initController(int &handle)
{
    if (scePadInit() != 0) return false;

    OrbisUserServiceInitializeParams params = {};
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    if (sceUserServiceInitialize(&params) != 0) return false;

    int32_t userId = -1;
    if (sceUserServiceGetInitialUser(&userId) != 0 || userId < 0) return false;

    handle = scePadOpen(userId, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    return handle >= 0;
}
} // namespace

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("KI1 Widescreen Test - aspect-preserving 16:9 preview.\n");

    // Keep the graphics context alive for the complete application lifetime.
    // The upstream sample declares a destructor but does not define one.
    Scene2D *scene = new Scene2D(kWidth, kHeight, 4);
    if (!scene->Init(0x02000000, 2)) {
        printf("Video output initialization failed.\n");
        for (;;) sceKernelUsleep(250000);
    }

    int padHandle = -1;
    const bool controllerReady = initController(padHandle);
    if (controllerReady) printf("DualShock 4 input ready.\n");
    else printf("Controller unavailable; default widescreen preview will continue.\n");

    bool backdrop = true;
    bool grid = false;
    bool safeFrame = false;
    int glow = 5;
    uint32_t previousButtons = 0;
    int frameId = 0;
    int activeBuffer = 0;

    // Draw the first buffer, present it, then prepare the second buffer.
    renderBuffer(scene, activeBuffer, backdrop, grid, safeFrame, glow);
    scene->SubmitFlip(frameId);
    scene->FrameWait(frameId);
    ++frameId;
    activeBuffer = 1;
    renderBuffer(scene, activeBuffer, backdrop, grid, safeFrame, glow);

    for (;;) {
        uint32_t pressed = 0;
        if (controllerReady && padHandle >= 0) {
            OrbisPadData pad = {};
            if (scePadReadState(padHandle, &pad) == 0) {
                pressed = pad.buttons & ~previousButtons;
                previousButtons = pad.buttons;
            } else {
                previousButtons = 0;
            }
        }

        bool changed = false;
        if (pressed & ORBIS_PAD_BUTTON_CROSS) { backdrop = !backdrop; changed = true; }
        if (pressed & ORBIS_PAD_BUTTON_TRIANGLE) { grid = !grid; changed = true; }
        if (pressed & ORBIS_PAD_BUTTON_SQUARE) { safeFrame = !safeFrame; changed = true; }
        if (pressed & ORBIS_PAD_BUTTON_LEFT) {
            const int next = std::max(0, glow - 1);
            changed = changed || next != glow;
            glow = next;
        }
        if (pressed & ORBIS_PAD_BUTTON_RIGHT) {
            const int next = std::min(10, glow + 1);
            changed = changed || next != glow;
            glow = next;
        }
        if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
            backdrop = true;
            grid = false;
            safeFrame = false;
            glow = 5;
            changed = true;
        }

        // Paint only the buffer that is not currently being scanned out.
        if (changed) {
            renderBuffer(scene, activeBuffer, backdrop, grid, safeFrame, glow);
        }

        scene->SetActiveFrameBuffer(activeBuffer);
        scene->SubmitFlip(frameId);
        scene->FrameWait(frameId);
        activeBuffer = 1 - activeBuffer;

        // After the flip, the old front buffer is now safe to prepare for
        // the next frame. This keeps both buffers visually in sync.
        if (changed) {
            renderBuffer(scene, activeBuffer, backdrop, grid, safeFrame, glow);
        }
        ++frameId;
    }

    return 0;
}
