#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"
#include "ki1_core_port.h"

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

uint16_t mapArcadeButtons(uint32_t buttons)
{
    uint16_t mapped = 0;

    if (buttons & ORBIS_PAD_BUTTON_CROSS)    mapped |= 0x0001; // punch 1
    if (buttons & ORBIS_PAD_BUTTON_SQUARE)   mapped |= 0x0002; // punch 2
    if (buttons & ORBIS_PAD_BUTTON_TRIANGLE) mapped |= 0x0004; // punch 3
    if (buttons & ORBIS_PAD_BUTTON_CIRCLE)   mapped |= 0x0008; // kick 1
    if (buttons & ORBIS_PAD_BUTTON_L1)       mapped |= 0x0010; // kick 2
    if (buttons & ORBIS_PAD_BUTTON_R1)       mapped |= 0x0020; // kick 3

    if (buttons & ORBIS_PAD_BUTTON_UP)    mapped |= 0x0040;
    if (buttons & ORBIS_PAD_BUTTON_DOWN)  mapped |= 0x0080;
    if (buttons & ORBIS_PAD_BUTTON_LEFT)  mapped |= 0x0100;
    if (buttons & ORBIS_PAD_BUTTON_RIGHT) mapped |= 0x0200;
    if (buttons & ORBIS_PAD_BUTTON_OPTIONS) mapped |= 0x0400; // start
    if (buttons & ORBIS_PAD_BUTTON_L3)      mapped |= 0x0800; // coin

    return mapped;
}

void drawArcadeScene(Scene2D *s, const uint32_t *frame, int glow)
{
    drawBackdrop(s, true, glow);
    s->DrawRGBImage(kGameX, 0, kGameWidth, kHeight, frame, 320, 240);

    panel(s, 16, 24, 208, 104, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "KILLER INSTINCT", 28, 42, 2, rgb(240, 247, 255));
    text(s, "KI1 ARCADE", 28, 78, 2, rgb(167, 194, 226));

    panel(s, 16, 176, 208, 392, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "D PAD", 28, 197, 3, rgb(232, 237, 248));
    text(s, "MOVE", 28, 228, 3, rgb(167, 194, 226));
    text(s, "CROSS SQUARE", 28, 285, 2, rgb(232, 237, 248));
    text(s, "TRIANGLE", 28, 316, 2, rgb(232, 237, 248));
    text(s, "CIRCLE L1 R1", 28, 373, 2, rgb(232, 237, 248));
    text(s, "OPTIONS START", 28, 430, 2, rgb(167, 194, 226));
    text(s, "L3 COIN", 28, 487, 2, rgb(167, 194, 226));

    panel(s, 1696, 24, 208, 104, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "4:3 AREA", 1708, 42, 3, rgb(240, 247, 255));
    text(s, "1440X1080", 1708, 78, 2, rgb(167, 194, 226));

    panel(s, 1696, 176, 208, 392, rgb(8, 12, 25), rgb(65, 172, 213));
    text(s, "CORE", 1708, 197, 3, rgb(232, 237, 248));
    text(s, "INTERPRETER", 1708, 228, 2, rgb(167, 194, 226));
    text(s, "ARCADE CORE", 1708, 285, 2, rgb(240, 190, 120));
    text(s, "AUDIO OUTPUT", 1708, 342, 2, rgb(167, 194, 226));
    text(s, "PENDING", 1708, 373, 2, rgb(167, 194, 226));
}


struct FolderPicker {
    std::string path = "/mnt/usb0";
    std::vector<std::string> folders;
    size_t selected = 0;
    std::string message = "OPEN YOUR USB, THEN THE FOLDER WITH KI1 FILES";
};

bool isDirectory(const std::string &path)
{
    struct stat st = {};
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool ensureDirectory(const std::string &path)
{
    if (isDirectory(path)) return true;
    if (mkdir(path.c_str(), 0777) == 0) return true;
    return errno == EEXIST && isDirectory(path);
}

void bootLog(FILE *logFile, const char *stage)
{
    printf("[BOOT] %s\\n", stage);
    if (logFile) {
        fprintf(logFile, "%s\\n", stage);
        fflush(logFile);
    }
}

std::string joinPath(const std::string &base, const std::string &name)
{
    if (base == "/") return base + name;
    return base + "/" + name;
}

void refreshFolders(FolderPicker &picker)
{
    picker.folders.clear();
    DIR *dir = opendir(picker.path.c_str());
    if (!dir) {
        picker.message = "CANNOT OPEN THIS LOCATION";
        picker.selected = 0;
        return;
    }

    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        if (isDirectory(joinPath(picker.path, name)))
            picker.folders.push_back(name);
    }
    closedir(dir);

    std::sort(picker.folders.begin(), picker.folders.end());
    picker.selected = 0;
    picker.message = picker.folders.empty()
        ? "NO SUBFOLDERS - TRIANGLE SELECTS THIS FOLDER"
        : "X OPEN FOLDER - TRIANGLE USE THIS FOLDER";
}

bool prepareInternalStorage(FolderPicker &picker)
{
    // Keep user files outside the read-only App0 package.
    const char *paths[] = {
        "/data/KillerRemakeP4",
        "/data/KillerRemakeP4/roms",
        "/data/KillerRemakeP4/roms/kinst"
    };
    for (const char *path : paths) {
        if (!ensureDirectory(path)) {
            picker.message = "CANNOT CREATE INTERNAL DATA FOLDER";
            return false;
        }
    }

    picker.path = "/data/KillerRemakeP4/roms/kinst";
    refreshFolders(picker);
    picker.message = "COPY KI1 FILES HERE THEN TRIANGLE";
    return true;
}

void pickerEnter(FolderPicker &picker)
{
    if (picker.selected >= picker.folders.size()) return;
    const std::string next = joinPath(picker.path, picker.folders[picker.selected]);
    if (next.size() >= 1000 || !isDirectory(next)) {
        picker.message = "CANNOT OPEN SELECTED FOLDER";
        return;
    }
    picker.path = next;
    refreshFolders(picker);
}

void pickerUp(FolderPicker &picker)
{
    if (picker.path == "/" || picker.path.empty()) return;
    const size_t slash = picker.path.find_last_of('/');
    if (slash == 0) picker.path = "/";
    else if (slash != std::string::npos) picker.path.resize(slash);
    refreshFolders(picker);
}

void drawFolderPicker(Scene2D *s, const FolderPicker &picker)
{
    drawBackdrop(s, true, 5);
    panel(s, 150, 70, 1620, 930, rgb(7, 11, 24), rgb(65, 172, 213));
    text(s, "KILLER INSTINCT", 205, 115, 7, rgb(244, 247, 255));
    text(s, "SELECT THE FOLDER WITH YOUR ARCADE FILES", 205, 205, 3, rgb(174, 201, 231));

    std::string shownPath = picker.path;
    if (shownPath.size() > 86)
        shownPath = "..." + shownPath.substr(shownPath.size() - 83);
    text(s, shownPath.c_str(), 205, 265, 3, rgb(255, 210, 125));

    const size_t visibleRows = 10;
    const size_t first = picker.selected >= visibleRows
        ? picker.selected - visibleRows + 1 : 0;

    if (picker.folders.empty()) {
        text(s, "NO SUBFOLDERS FOUND", 245, 375, 4, rgb(225, 230, 240));
    } else {
        for (size_t row = 0; row < visibleRows && first + row < picker.folders.size(); ++row) {
            const size_t index = first + row;
            const int y = 340 + static_cast<int>(row) * 55;
            const bool active = index == picker.selected;
            if (active)
                panel(s, 190, y - 8, 1540, 45, rgb(30, 44, 71), rgb(255, 194, 91));
            const std::string name = picker.folders[index].substr(0, 76);
            text(s, name.c_str(), 255, y, 3,
                 active ? rgb(255, 245, 220) : rgb(190, 207, 229));
        }
    }

    text(s, picker.message.c_str(), 205, 905, 2, rgb(255, 201, 117));
    text(s, "UP DOWN MOVE   X OPEN   CIRCLE UP   TRIANGLE SELECT", 205, 945, 2,
         rgb(185, 204, 231));
    text(s, "OPTIONS INTERNAL DATA   SQUARE USB", 205, 975, 2,
         rgb(185, 204, 231));
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
    printf("Killer Instinct PS4 - select game folder.\\n");

    // Create the writable data folder first, then record each boot milestone.
    const bool dataRootReady = ensureDirectory("/data/KillerRemakeP4");
    FILE *bootLogFile = dataRootReady
        ? fopen("/data/KillerRemakeP4/boot.log", "w") : nullptr;
    bootLog(bootLogFile, "APP MAIN ENTERED");

    const bool romsReady = dataRootReady &&
        ensureDirectory("/data/KillerRemakeP4/roms");
    const bool internalDataReady = romsReady &&
        ensureDirectory("/data/KillerRemakeP4/roms/kinst");
    bootLog(bootLogFile, internalDataReady
        ? "INTERNAL GAME FOLDER READY"
        : "INTERNAL GAME FOLDER CREATION FAILED");

    bootLog(bootLogFile, "BEFORE VIDEO INITIALIZATION");
    Scene2D *scene = new Scene2D(kWidth, kHeight, 4);
    if (!scene->Init(0x0C000000, 2)) {
        bootLog(bootLogFile, "VIDEO INITIALIZATION FAILED");
        printf("Video output initialization failed.\\n");
        for (;;) sceKernelUsleep(250000);
    }
    bootLog(bootLogFile, "VIDEO INITIALIZATION OK");

    FolderPicker picker;
    // The internal app-data folder has already been prepared during startup.
    picker.path = "/data/KillerRemakeP4/roms/kinst";
    picker.message = internalDataReady
        ? "INTERNAL DATA FOLDER READY"
        : "FAILED TO CREATE INTERNAL DATA FOLDER";

    // Record rendering milestones to identify where a black screen begins.
    bootLog(bootLogFile, "BEFORE DRAWING STARTUP FRAMES");
    for (int buffer = 0; buffer < 2; ++buffer) {
        scene->SetActiveFrameBuffer(buffer);
        drawFolderPicker(scene, picker);
    }
    bootLog(bootLogFile, "STARTUP FRAMEBUFFERS DRAWN");
    int frameId = 0;
    scene->SetActiveFrameBuffer(0);
    bootLog(bootLogFile, "BEFORE FIRST VIDEO FLIP");
    scene->SubmitFlip(frameId);
    scene->FrameWait(frameId);
    bootLog(bootLogFile, "FIRST VIDEO FLIP COMPLETED");
    scene->FrameBufferSwap();
    ++frameId;

    // Initialize the controller after the startup frame. Do not scan USB
    // or /mnt here: directory I/O can stall before the UI loop is running.
    int padHandle = -1;
    const bool controllerReady = initController(padHandle);
    if (controllerReady) printf("DualShock controller ready.\n");
    else printf("Controller initialization failed.\n");

    uint32_t previousButtons = 0;
    bool gameReady = false;
    bool storageScanned = false;
    int glow = 5;
    static uint32_t gameFrame[320 * 240];
    double frameAccumulator = 0.0;
    char coreError[512] = {};

    bootLog(bootLogFile, "MAIN INPUT LOOP REACHED");
    for (;;) {
        uint32_t pressed = 0;
        uint32_t currentButtons = 0;
        if (controllerReady && padHandle >= 0) {
            OrbisPadData pad = {};
            if (scePadReadState(padHandle, &pad) == 0) {
                currentButtons = pad.buttons;
                pressed = currentButtons & ~previousButtons;
                previousButtons = currentButtons;
            } else {
                previousButtons = 0;
            }
        }

        bool pickerChanged = false;
        if (!gameReady) {
            if (pressed & ORBIS_PAD_BUTTON_UP) {
                if (picker.selected > 0) --picker.selected;
                pickerChanged = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) {
                if (picker.selected + 1 < picker.folders.size()) ++picker.selected;
                pickerChanged = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_OPTIONS) {
                bootLog(bootLogFile, "OPTIONS BUTTON PRESSED");
                // Method 2: create and open the app's writable internal folder.
                storageScanned = prepareInternalStorage(picker);
                pickerChanged = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_SQUARE) {
                // USB browsing remains available as an alternate source.
                if (isDirectory("/mnt/usb0")) picker.path = "/mnt/usb0";
                else if (isDirectory("/mnt/usb1")) picker.path = "/mnt/usb1";
                else {
                    picker.message = "NO USB STORAGE MOUNTED";
                    pickerChanged = true;
                }
                if (isDirectory(picker.path) &&
                    (picker.path == "/mnt/usb0" || picker.path == "/mnt/usb1")) {
                    refreshFolders(picker);
                    storageScanned = true;
                    pickerChanged = true;
                }
            }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                if (storageScanned) pickerEnter(picker);
                else picker.message = "PRESS OPTIONS TO CREATE INTERNAL FOLDER";
                pickerChanged = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                if (storageScanned) pickerUp(picker);
                else picker.message = "PRESS OPTIONS TO CREATE INTERNAL FOLDER";
                pickerChanged = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                if (!storageScanned) {
                    picker.message = "PRESS OPTIONS TO PREPARE INTERNAL FOLDER FIRST";
                    pickerChanged = true;
                } else {
                    coreError[0] = '\0';
                    if (ki1_core_init_from_directory(picker.path.c_str(),
                                                     coreError, sizeof(coreError))) {
                        gameReady = true;
                        ki1_core_run_frame(0, 0);
                        ki1_core_copy_rgb(gameFrame);
                        printf("Killer Instinct core initialized from %s\n", picker.path.c_str());
                    } else {
                        picker.message = coreError[0] ? coreError : "GAME FILES NOT FOUND";
                        pickerChanged = true;
                        printf("Folder rejected: %s\n", picker.message.c_str());
                    }
                }
            }
        }

        if (gameReady) {
            frameAccumulator += 58.9634 / 60.0;
            if (frameAccumulator >= 1.0) {
                frameAccumulator -= 1.0;
                ki1_core_run_frame(mapArcadeButtons(currentButtons), 0);
                ki1_core_copy_rgb(gameFrame);
            }
            drawArcadeScene(scene, gameFrame, glow);

            // The core generates samples, but PS4 audio output is not wired yet.
            int16_t discardedAudio[2048];
            while (ki1_core_fetch_audio(discardedAudio, 2048) > 0) {}
        } else if (pickerChanged) {
            drawFolderPicker(scene, picker);
        }

        // Keep the exact submit/wait/swap order used by the known-good build.
        scene->SubmitFlip(frameId);
        scene->FrameWait(frameId);
        scene->FrameBufferSwap();
        ++frameId;

        // Mirror changed picker contents to the new back buffer.
        if (!gameReady && pickerChanged)
            drawFolderPicker(scene, picker);
    }

    return 0;
}
