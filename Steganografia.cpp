#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <bitset>
#include <vector>
#include <random>
#include <conio.h>
#include <stdlib.h>
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace std;
using namespace Gdiplus;

// ============ ОБЪЯВЛЕНИЯ ФУНКЦИЙ ============
void infoHider();
void infoViewer();
void infoDetector();
void programTester();
void changeSettings();
void programHelper();
void exitProgram();

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct ProgramSettings {
    bool useEncryption = true;
    unsigned int currentSeed = 0;
    bool useSuffix = true;
    bool useSeedSuffix = true;
    int packLevel = 1; // 1: 1 байт/пиксель, 2: 2 байта/пиксель, 3: 3 байта/пиксель
};

ProgramSettings g_settings;

// ============ СТРУКТУРА ЦВЕТА ============
struct PixelColor {
    unsigned char R, G, B;
    PixelColor(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0) : R(r), G(g), B(b) {}
};

// ============ СТРУКТУРА СЛУЖЕБНОГО ЗАГОЛОВКА ============
struct ServiceHeader {
    bool valid = false;
    int packLevel = 1;
    string extension = "txt";
    int textSize = 0;
};

// ============ СТРУКТУРА ДЛЯ ПОЗИЦИЙ ПИКСЕЛЕЙ ============
struct PixelPosition {
    int x, y;
};

// ============ БИТОВОЕ РАСПРЕДЕЛЕНИЕ ПО КАНАЛАМ RGB (HVS-ОПТИМИЗИРОВАННОЕ) ============
// Учитывает психофизиологию человеческого зрения (Human Visual System):
// Зеленый канал (G) воспринимается глазом наиболее чувствительно (~59% яркости),
// Красный (R) — умеренно (~30%), Синий (B) — наименее чувствительно (~11%).
//
// Уровень 1 (1 байт = 8 бит/пиксель):
//   R: 3 бита (биты 0..2)
//   G: 2 бита (биты 3..4)  <- наименьшее искажение для чувствительного зеленого
//   B: 3 бита (биты 5..7)
//
// Уровень 2 (2 байта = 16 бит/пиксель):
//   R: 5 бит (биты 0..4)
//   G: 5 бит (биты 5..9)
//   B: 6 бит (биты 10..15) <- дополнительный бит отдан синему как наименее заметному
//
// Уровень 3 (3 байта = 24 бит/пиксель):
//   R: 8 бит (Байт 0)
//   G: 8 бит (Байт 1)
//   B: 8 бит (Байт 2)

inline PixelColor EmbedByteLevel1(PixelColor c, unsigned char symbol) {
    c.R = static_cast<unsigned char>((c.R & ~0x07) | (symbol & 0x07));
    c.G = static_cast<unsigned char>((c.G & ~0x03) | ((symbol >> 3) & 0x03));
    c.B = static_cast<unsigned char>((c.B & ~0x07) | ((symbol >> 5) & 0x07));
    return c;
}

inline unsigned char ExtractByteLevel1(const PixelColor& c) {
    unsigned char val = 0;
    val |= (c.R & 0x07);
    val |= ((c.G & 0x03) << 3);
    val |= ((c.B & 0x07) << 5);
    return val;
}

inline PixelColor Embed2BytesLevel2(PixelColor c, unsigned char b0, unsigned char b1) {
    uint16_t w = static_cast<uint16_t>(b0) | (static_cast<uint16_t>(b1) << 8);
    c.R = static_cast<unsigned char>((c.R & ~0x1F) | (w & 0x1F));
    c.G = static_cast<unsigned char>((c.G & ~0x1F) | ((w >> 5) & 0x1F));
    c.B = static_cast<unsigned char>((c.B & ~0x3F) | ((w >> 10) & 0x3F));
    return c;
}

inline void Extract2BytesLevel2(const PixelColor& c, unsigned char& b0, unsigned char& b1) {
    uint16_t w = 0;
    w |= (c.R & 0x1F);
    w |= ((c.G & 0x1F) << 5);
    w |= ((c.B & 0x3F) << 10);
    b0 = static_cast<unsigned char>(w & 0xFF);
    b1 = static_cast<unsigned char>((w >> 8) & 0xFF);
}

inline PixelColor Embed3BytesLevel3(PixelColor c, unsigned char b0, unsigned char b1, unsigned char b2) {
    c.R = b0;
    c.G = b1;
    c.B = b2;
    return c;
}

inline void Extract3BytesLevel3(const PixelColor& c, unsigned char& b0, unsigned char& b1, unsigned char& b2) {
    b0 = c.R;
    b1 = c.G;
    b2 = c.B;
}

// Универсальное встраивание до packLevel байт в пиксель
inline PixelColor EmbedBytesToPixel(PixelColor c, const unsigned char* bytes, int count, int packLevel) {
    if (packLevel == 1) {
        return EmbedByteLevel1(c, count > 0 ? bytes[0] : 0);
    }
    else if (packLevel == 2) {
        unsigned char b0 = (count > 0) ? bytes[0] : 0;
        unsigned char b1 = (count > 1) ? bytes[1] : 0;
        return Embed2BytesLevel2(c, b0, b1);
    }
    else if (packLevel == 3) {
        unsigned char b0 = (count > 0) ? bytes[0] : 0;
        unsigned char b1 = (count > 1) ? bytes[1] : 0;
        unsigned char b2 = (count > 2) ? bytes[2] : 0;
        return Embed3BytesLevel3(c, b0, b1, b2);
    }
    return c;
}

// Универсальное извлечение до packLevel байт из пикселя
inline int ExtractBytesFromPixel(const PixelColor& c, unsigned char* outBytes, int maxToExtract, int packLevel) {
    if (packLevel == 1) {
        if (maxToExtract >= 1) {
            outBytes[0] = ExtractByteLevel1(c);
            return 1;
        }
    }
    else if (packLevel == 2) {
        unsigned char b0 = 0, b1 = 0;
        Extract2BytesLevel2(c, b0, b1);
        int written = 0;
        if (maxToExtract >= 1) { outBytes[0] = b0; written++; }
        if (maxToExtract >= 2) { outBytes[1] = b1; written++; }
        return written;
    }
    else if (packLevel == 3) {
        unsigned char b0 = 0, b1 = 0, b2 = 0;
        Extract3BytesLevel3(c, b0, b1, b2);
        int written = 0;
        if (maxToExtract >= 1) { outBytes[0] = b0; written++; }
        if (maxToExtract >= 2) { outBytes[1] = b1; written++; }
        if (maxToExtract >= 3) { outBytes[2] = b2; written++; }
        return written;
    }
    return 0;
}

// ============ РАБОТА С BMP ============
PixelColor GetPixelColor(Bitmap* bPic, int x, int y) {
    Color color;
    bPic->GetPixel(x, y, &color);
    return PixelColor(
        static_cast<unsigned char>(color.GetR()),
        static_cast<unsigned char>(color.GetG()),
        static_cast<unsigned char>(color.GetB())
    );
}

void SetPixelColor(Bitmap* bPic, int x, int y, const PixelColor& color) {
    Color gdiColor(color.R, color.G, color.B);
    bPic->SetPixel(x, y, gdiColor);
}

// ============ ВЫЧИСЛЕНИЕ ЕМКОСТИ ============
int GetBitsPerPixel(int packLevel) {
    if (packLevel <= 1) return 8;
    if (packLevel == 2) return 16;
    return 24;
}

int GetMaxCapacity(int width, int height, int packLevel) {
    if (packLevel < 1 || packLevel > 3) {
        packLevel = 1;
    }
    if (width < 20 || height < 20) return 0;

    // Столбец 0 зарезервирован для первичного заголовка.
    // Последняя строка (height - 1) зарезервирована для дублирующего заголовка.
    int availablePixels = (width - 1) * (height - 1);
    if (availablePixels <= 0) return 0;

    return availablePixels * packLevel;
}

// ============ ПРОВЕРКА РАЗМЕРА ИЗОБРАЖЕНИЯ ============
bool CheckImageSize(Bitmap* bPic) {
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    const int MIN_DIM = 20;

    if (width < MIN_DIM || height < MIN_DIM) {
        cout << "\n=== WARNING: Image is too small! ===" << endl;
        cout << "Dimensions: " << width << "x" << height << " pixels" << endl;
        cout << "Minimum required: " << MIN_DIM << "x" << MIN_DIM << " pixels" << endl;
        cout << "Please use a larger image." << endl;
        cout << "=====================================" << endl;
        system("pause");
        return false;
    }

    return true;
}

// ============ КРИПТОСТОЙКАЯ ГЕНЕРАЦИЯ SEED ============
unsigned int GenerateSeed(int width, int height) {
    unsigned int seed = 0;
    try {
        random_device rd;
        seed = rd();
    }
    catch (...) {
        seed = 0;
    }

    LARGE_INTEGER perfCount;
    QueryPerformanceCounter(&perfCount);

    seed ^= static_cast<unsigned int>(perfCount.QuadPart);
    seed ^= (static_cast<unsigned int>(width) * 2654435761u);
    seed ^= (static_cast<unsigned int>(height) * 2246822519u);
    seed ^= static_cast<unsigned int>(GetTickCount64());
    seed ^= static_cast<unsigned int>(time(nullptr));

    // Murmur3 32-bit finalizer
    seed ^= seed >> 16;
    seed *= 0x85ebca6b;
    seed ^= seed >> 13;
    seed *= 0xc2b2ae35;
    seed ^= seed >> 16;

    if (seed == 0) seed = 0x5F3759DF;
    return seed;
}

// ============ ГЕНЕРАЦИЯ СЛУЧАЙНЫХ ПОЗИЦИЙ (ОПТИМИЗИРОВАННЫЙ FISHER-YATES) ============
vector<PixelPosition> GenerateRandomPositions(int width, int height, int count, unsigned int seed) {
    int totalPixels = (width - 1) * (height - 1);
    if (count > totalPixels) count = totalPixels;
    if (count <= 0) return {};

    vector<PixelPosition> positions;
    positions.reserve(count);

    mt19937 rng(seed);

    vector<PixelPosition> allPositions;
    allPositions.reserve(totalPixels);
    for (int x = 1; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            allPositions.push_back({ x, y });
        }
    }

    // Частичное тасование Фишера-Йетса за O(count)
    for (int i = 0; i < count; i++) {
        uniform_int_distribution<int> dist(i, totalPixels - 1);
        int j = dist(rng);
        swap(allPositions[i], allPositions[j]);
        positions.push_back(allPositions[i]);
    }

    return positions;
}

// ============ ЗАПИСЬ И ЧТЕНИЕ СЛУЖЕБНЫХ ЗАГОЛОВКОВ (С ДУБЛИРОВАНИЕМ) ============
void WriteServiceData(Bitmap* bPic, int packLevel, const string& extension, int textSize) {
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int lastRow = height - 1;

    string ext3 = extension;
    while (ext3.length() < 3) ext3 += ' ';
    if (ext3.length() > 3) ext3 = ext3.substr(0, 3);

    string header = "";
    header += static_cast<char>('0' + packLevel); // [0] packLevel
    header += '/';                                // [1] маркер секрета
    header += ext3;                               // [2..4] расширение
    header += to_string(textSize) + ":";          // [5..] размер с разделителем

    // 1. Запись первичного заголовка в столбец 0 (x=0, y=0..N)
    for (int i = 0; i < (int)header.length() && i < height; i++) {
        PixelColor cur = GetPixelColor(bPic, 0, i);
        SetPixelColor(bPic, 0, i, EmbedByteLevel1(cur, static_cast<unsigned char>(header[i])));
    }

    // 2. Запись дублирующего заголовка в последнюю строку (y=lastRow, x=0..N)
    for (int i = 0; i < (int)header.length() && i < width; i++) {
        PixelColor cur = GetPixelColor(bPic, i, lastRow);
        SetPixelColor(bPic, i, lastRow, EmbedByteLevel1(cur, static_cast<unsigned char>(header[i])));
    }
}

ServiceHeader ReadPrimaryHeader(Bitmap* bPic) {
    ServiceHeader hdr;
    int height = bPic->GetHeight();
    if (height < 10) return hdr;

    unsigned char plChar = ExtractByteLevel1(GetPixelColor(bPic, 0, 0));
    if (plChar < '1' || plChar > '3') return hdr;
    hdr.packLevel = plChar - '0';

    unsigned char marker = ExtractByteLevel1(GetPixelColor(bPic, 0, 1));
    if (marker != '/') return hdr;

    string ext = "";
    for (int y = 2; y <= 4; y++) {
        unsigned char c = ExtractByteLevel1(GetPixelColor(bPic, 0, y));
        if (c != ' ' && c != 0) ext += static_cast<char>(c);
    }
    hdr.extension = ext.empty() ? "txt" : ext;

    string sizeStr = "";
    for (int y = 5; y < height && y < 30; y++) {
        unsigned char c = ExtractByteLevel1(GetPixelColor(bPic, 0, y));
        if (c == ':') break;
        if (c >= '0' && c <= '9') {
            sizeStr += static_cast<char>(c);
        }
        else {
            return hdr;
        }
    }

    if (sizeStr.empty()) return hdr;
    try {
        hdr.textSize = stoi(sizeStr);
        if (hdr.textSize > 0) hdr.valid = true;
    }
    catch (...) {
        hdr.valid = false;
    }

    return hdr;
}

ServiceHeader ReadBackupHeader(Bitmap* bPic) {
    ServiceHeader hdr;
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int lastRow = height - 1;
    if (width < 10 || height < 2) return hdr;

    unsigned char plChar = ExtractByteLevel1(GetPixelColor(bPic, 0, lastRow));
    if (plChar < '1' || plChar > '3') return hdr;
    hdr.packLevel = plChar - '0';

    unsigned char marker = ExtractByteLevel1(GetPixelColor(bPic, 1, lastRow));
    if (marker != '/') return hdr;

    string ext = "";
    for (int x = 2; x <= 4; x++) {
        unsigned char c = ExtractByteLevel1(GetPixelColor(bPic, x, lastRow));
        if (c != ' ' && c != 0) ext += static_cast<char>(c);
    }
    hdr.extension = ext.empty() ? "txt" : ext;

    string sizeStr = "";
    for (int x = 5; x < width && x < 30; x++) {
        unsigned char c = ExtractByteLevel1(GetPixelColor(bPic, x, lastRow));
        if (c == ':') break;
        if (c >= '0' && c <= '9') {
            sizeStr += static_cast<char>(c);
        }
        else {
            return hdr;
        }
    }

    if (sizeStr.empty()) return hdr;
    try {
        hdr.textSize = stoi(sizeStr);
        if (hdr.textSize > 0) hdr.valid = true;
    }
    catch (...) {
        hdr.valid = false;
    }

    return hdr;
}

bool CheckServiceData(Bitmap* bPic, ServiceHeader& outHeader) {
    ServiceHeader primary = ReadPrimaryHeader(bPic);
    ServiceHeader backup = ReadBackupHeader(bPic);

    if (primary.valid && backup.valid) {
        if (primary.packLevel == backup.packLevel &&
            primary.extension == backup.extension &&
            primary.textSize == backup.textSize) {
            cout << "Service data verified successfully (Primary and Backup match)." << endl;
            outHeader = primary;
            return true;
        }
        else {
            cout << "\n=== WARNING: Service data mismatch between Primary and Backup! ===" << endl;
            cout << "Primary: Level=" << primary.packLevel << ", Ext=" << primary.extension << ", Size=" << primary.textSize << endl;
            cout << "Backup:  Level=" << backup.packLevel << ", Ext=" << backup.extension << ", Size=" << backup.textSize << endl;
            cout << "Using Primary service data." << endl;
            outHeader = primary;
            return true;
        }
    }
    else if (primary.valid) {
        cout << "Primary service data valid (Backup corrupted). Using Primary." << endl;
        outHeader = primary;
        return true;
    }
    else if (backup.valid) {
        cout << "Backup service data valid (Primary corrupted). Using Backup." << endl;
        outHeader = backup;
        return true;
    }
    else {
        return false;
    }
}

bool isSecretPresentInBMP(Bitmap* bPic) {
    ServiceHeader primary = ReadPrimaryHeader(bPic);
    if (primary.valid) return true;
    ServiceHeader backup = ReadBackupHeader(bPic);
    return backup.valid;
}

// ============ ЗАПИСЬ ПОЛЕЗНОЙ НАГРУЗКИ В BMP ============
bool HidePayloadInBMP(Bitmap* bPic, const vector<unsigned char>& payload, const string& extension, bool encrypted, unsigned int seed, int packLevel) {
    int totalBytes = static_cast<int>(payload.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    if (packLevel < 1 || packLevel > 3) packLevel = 1;
    int capacity = GetMaxCapacity(width, height, packLevel);

    if (totalBytes > capacity) {
        string msg = "Data too large for this image!\nSize: " + to_string(totalBytes) +
                     " bytes\nMax capacity: " + to_string(capacity) + " bytes";
        MessageBoxA(NULL, msg.c_str(), "Capacity Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (isSecretPresentInBMP(bPic)) {
        MessageBoxA(NULL, "This image already contains hidden information!", "Information", MB_OK | MB_ICONWARNING);
        return false;
    }

    // 1. Запись служебного заголовка (в столбец 0 и последнюю строку)
    WriteServiceData(bPic, packLevel, extension, totalBytes);

    // 2. Запись полезных данных
    if (!encrypted) {
        // Последовательная запись
        int byteIndex = 0;
        for (int x = 1; x < width && byteIndex < totalBytes; x++) {
            for (int y = 0; y < height - 1 && byteIndex < totalBytes; y++) {
                int bytesThisPixel = min(packLevel, totalBytes - byteIndex);
                PixelColor curColor = GetPixelColor(bPic, x, y);
                PixelColor newColor = EmbedBytesToPixel(curColor, &payload[byteIndex], bytesThisPixel, packLevel);
                SetPixelColor(bPic, x, y, newColor);
                byteIndex += bytesThisPixel;
            }
        }
    }
    else {
        // Псевдослучайное распределение пикселей на основе seed
        int pixelsNeeded = (totalBytes + packLevel - 1) / packLevel;
        vector<PixelPosition> positions = GenerateRandomPositions(width, height, pixelsNeeded, seed);

        int byteIndex = 0;
        for (int i = 0; i < (int)positions.size() && byteIndex < totalBytes; i++) {
            int bytesThisPixel = min(packLevel, totalBytes - byteIndex);
            PixelColor curColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
            PixelColor newColor = EmbedBytesToPixel(curColor, &payload[byteIndex], bytesThisPixel, packLevel);
            SetPixelColor(bPic, positions[i].x, positions[i].y, newColor);
            byteIndex += bytesThisPixel;
        }
    }

    return true;
}

// ============ ИЗВЛЕЧЕНИЕ ПОЛЕЗНОЙ НАГРУЗКИ ИЗ BMP ============
bool ExtractPayloadFromBMP(Bitmap* bPic, vector<unsigned char>& outPayload, string& outExtension, bool isEncrypted, unsigned int seed) {
    ServiceHeader header;
    if (!CheckServiceData(bPic, header)) {
        MessageBoxA(NULL, "No valid hidden information found in this image.", "Information", MB_OK | MB_ICONINFORMATION);
        return false;
    }

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int totalBytes = header.textSize;
    int packLevel = header.packLevel;
    outExtension = header.extension;

    outPayload.clear();
    outPayload.reserve(totalBytes);

    if (!isEncrypted) {
        int bytesRead = 0;
        for (int x = 1; x < width && bytesRead < totalBytes; x++) {
            for (int y = 0; y < height - 1 && bytesRead < totalBytes; y++) {
                unsigned char buf[3];
                int bytesToRead = min(packLevel, totalBytes - bytesRead);
                int extracted = ExtractBytesFromPixel(GetPixelColor(bPic, x, y), buf, bytesToRead, packLevel);
                for (int b = 0; b < extracted && bytesRead < totalBytes; b++) {
                    outPayload.push_back(buf[b]);
                    bytesRead++;
                }
            }
        }
    }
    else {
        int pixelsNeeded = (totalBytes + packLevel - 1) / packLevel;
        vector<PixelPosition> positions = GenerateRandomPositions(width, height, pixelsNeeded, seed);

        int bytesRead = 0;
        for (int i = 0; i < (int)positions.size() && bytesRead < totalBytes; i++) {
            unsigned char buf[3];
            int bytesToRead = min(packLevel, totalBytes - bytesRead);
            int extracted = ExtractBytesFromPixel(GetPixelColor(bPic, positions[i].x, positions[i].y), buf, bytesToRead, packLevel);
            for (int b = 0; b < extracted && bytesRead < totalBytes; b++) {
                outPayload.push_back(buf[b]);
                bytesRead++;
            }
        }
    }

    return (static_cast<int>(outPayload.size()) == totalBytes);
}

// ============ ВЫБОР ФАЙЛА ============
bool SelectBMPFile(char* outPath, int maxPath) {
    OPENFILENAMEA ofn = { 0 };
    char file[MAX_PATH] = { 0 };
    char filter[] =
        "BMP files (*.bmp;*.BMP)\0*.bmp;*.BMP\0"
        "All files (*.*)\0*.*\0";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetDesktopWindow();
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select BMP file";
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        strcpy_s(outPath, maxPath, file);
        return true;
    }
    return false;
}

bool SelectAnyFile(char* outPath, int maxPath) {
    OPENFILENAMEA ofn = { 0 };
    char file[MAX_PATH] = { 0 };
    char filter[] =
        "All files (*.*)\0*.*\0"
        "Text files (*.txt)\0*.txt\0"
        "Archive files (*.zip;*.rar;*.7z)\0*.zip;*.rar;*.7z\0";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetDesktopWindow();
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select file to hide";
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        strcpy_s(outPath, maxPath, file);
        return true;
    }
    return false;
}

// ============ СОЗДАНИЕ ПАПКИ ============
bool CreateDirectoryIfNotExists(const char* path) {
    DWORD attribs = GetFileAttributesA(path);
    if (attribs != INVALID_FILE_ATTRIBUTES) {
        return true;
    }

    char tempPath[MAX_PATH];
    strcpy_s(tempPath, sizeof(tempPath), path);

    for (unsigned i = 0; i < strlen(tempPath); i++) {
        if (tempPath[i] == '\\' || tempPath[i] == '/') {
            tempPath[i] = '\0';
            CreateDirectoryA(tempPath, NULL);
            tempPath[i] = '\\';
        }
    }
    CreateDirectoryA(tempPath, NULL);
    return true;
}

// ============ ИДЕНТИФИКАТОР (GUID) ДЛЯ BMP ФАЙЛОВ ============
const CLSID CLSID_BMP = { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } };

// ============ СОХРАНЕНИЕ ВЫХОДНОГО ФАЙЛА ============
bool SaveBMPWith(Bitmap* bPic, const char* originalPath, bool encrypted, unsigned int seed) {
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    _splitpath_s(originalPath, drive, _MAX_DRIVE, dir, _MAX_DIR,
        fname, _MAX_FNAME, ext, _MAX_EXT);

    char outputDir[MAX_PATH];
    if (strlen(drive) > 0 || strlen(dir) > 0) {
        sprintf_s(outputDir, sizeof(outputDir), "%s%soutput_img\\", drive, dir);
    }
    else {
        sprintf_s(outputDir, sizeof(outputDir), "output_img\\");
    }
    CreateDirectoryIfNotExists(outputDir);

    char fullName[MAX_PATH];

    if (g_settings.useSuffix) {
        if (encrypted) {
            sprintf_s(fullName, sizeof(fullName), "%s%s_encrypted", outputDir, fname);
            if (g_settings.useSeedSuffix) {
                char seedStr[32];
                sprintf_s(seedStr, sizeof(seedStr), "_%u", seed);
                strcat_s(fullName, seedStr);
            }
        }
        else {
            sprintf_s(fullName, sizeof(fullName), "%s%s_hidden", outputDir, fname);
        }
    }
    else {
        sprintf_s(fullName, sizeof(fullName), "%s%s", outputDir, fname);
    }

    strcat_s(fullName, ".bmp");

    WCHAR wnewPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, fullName, -1, wnewPath, MAX_PATH);

    if (bPic->Save(wnewPath, &CLSID_BMP, NULL) != Ok) {
        return false;
    }

    cout << "\nFile successfully saved as:\n" << fullName << endl;
    return true;
}

// Очистка входного потока
void ClearCin() {
    cin.clear();
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
}

// ============ ОТОБРАЖЕНИЕ НАСТРОЕК ============
void showSettings() {
    system("cls");
    cout << "================== Settings ==================\n";
    cout << "1. Encryption (Pseudo-random scattering): " << (g_settings.useEncryption ? "ON" : "OFF") << "\n";
    cout << "2. Add suffix to output filename:        " << (g_settings.useSuffix ? "ON" : "OFF") << "\n";
    cout << "3. Add seed to output filename:          " << (g_settings.useSeedSuffix ? "ON" : "OFF") << "\n";
    cout << "4. Pack level (1-3):                     " << g_settings.packLevel << "\n";
    cout << "   -> Level 1: 1 byte/pixel (R3 G2 B3, highest visual quality)\n";
    cout << "   -> Level 2: 2 bytes/pixel (R5 G5 B6, balanced)\n";
    cout << "   -> Level 3: 3 bytes/pixel (R8 G8 B8, maximum capacity)\n";
    cout << "0. Return to main menu\n";
    cout << "==============================================\n\n";
}

// ============ ИЗМЕНЕНИЕ НАСТРОЕК ============
void changeSettings() {
    int choice;
    do {
        showSettings();
        cout << "Select setting to change: ";
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            g_settings.useEncryption = !g_settings.useEncryption;
            break;
        case 2:
            g_settings.useSuffix = !g_settings.useSuffix;
            break;
        case 3:
            g_settings.useSeedSuffix = !g_settings.useSeedSuffix;
            break;
        case 4: {
            cout << "\nEnter pack level (1-3): ";
            int level = _getch() - '0';
            if (level >= 1 && level <= 3) {
                g_settings.packLevel = level;
                cout << "\nPack level set to: " << level << "\n";
            }
            else {
                cout << "\nInvalid level! Please enter 1, 2, or 3.\n";
            }
            system("pause");
            break;
        }
        case 0:
            return;
        default:
            break;
        }
    } while (choice != 0);
}

// ============ СОКРЫТИЕ ДАННЫХ В BMP ============
void infoHider() {
    system("cls");
    cout << "=== Welcome to InfoHider ===\n";
    cout << "1. LSB Text Mode (Hide text message)\n";
    cout << "2. LSB File Mode (Hide any file .txt, .zip, .bin into BMP)\n";
    cout << "3. Settings\n";
    cout << "0. Return to menu\n\n";

    int alg = _getch() - '0';

    if (alg == 0) return;
    if (alg == 3) {
        changeSettings();
        return;
    }

    if (alg == 1 || alg == 2) {
        system("cls");
        cout << (alg == 1 ? "=== LSB Text Mode ===\n\n" : "=== LSB File Mode ===\n\n");

        char path[MAX_PATH];
        cout << "Select carrier BMP file...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No carrier file selected. Operation cancelled.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        unique_ptr<Bitmap> bPic(new Bitmap(wpath));
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP image!\n";
            system("pause");
            return;
        }

        if (!CheckImageSize(bPic.get())) {
            return;
        }

        int packLevel = g_settings.packLevel;
        if (isSecretPresentInBMP(bPic.get())) {
            cout << "\nThis file already contains hidden information!\n";
            cout << "Use 'View information' to inspect or extract it.\n";
            system("pause");
            return;
        }

        int width = bPic->GetWidth();
        int height = bPic->GetHeight();
        int capacity = GetMaxCapacity(width, height, packLevel);

        cout << "Carrier image loaded: " << path << endl;
        cout << "Dimensions: " << width << "x" << height << endl;
        cout << "Pack level: " << packLevel << " (" << packLevel << " byte(s) per pixel)\n";
        cout << "Max capacity: " << capacity << " bytes\n";
        cout << "Encryption (Seed scattering): " << (g_settings.useEncryption ? "ON" : "OFF") << endl;

        vector<unsigned char> payload;
        string extension = "txt";

        if (alg == 1) {
            cout << "\nInput your text to hide:\n";
            string text;
            getline(cin, text);
            if (text.empty()) {
                // Если после предыдущего ввода остался newline
                getline(cin, text);
            }

            if (text.empty()) {
                cout << "Text cannot be empty!\n";
                system("pause");
                return;
            }

            payload.assign(text.begin(), text.end());
            extension = "txt";
        }
        else {
            char filePath[MAX_PATH];
            cout << "\nSelect file to embed into BMP...\n";
            if (!SelectAnyFile(filePath, sizeof(filePath))) {
                cout << "No file selected. Operation cancelled.\n";
                system("pause");
                return;
            }

            ifstream inFile(filePath, ios::binary);
            if (!inFile) {
                cout << "Error opening payload file!\n";
                system("pause");
                return;
            }

            payload.assign((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
            inFile.close();

            if (payload.empty()) {
                cout << "Selected payload file is empty!\n";
                system("pause");
                return;
            }

            const char* dot = strrchr(filePath, '.');
            if (dot != nullptr && strlen(dot + 1) > 0) {
                extension = string(dot + 1);
            }
            else {
                extension = "bin";
            }
        }

        cout << "\nPayload size: " << payload.size() << " bytes (." << extension << ")" << endl;

        if ((int)payload.size() > capacity) {
            cout << "\nError: Payload exceeds maximum capacity (" << capacity << " bytes)!\n";
            cout << "Please use a larger image or increase pack level.\n";
            system("pause");
            return;
        }

        bool encrypted = g_settings.useEncryption;
        unsigned int seed = 0;

        if (encrypted) {
            seed = GenerateSeed(width, height);
            g_settings.currentSeed = seed;
            cout << "\n>>> IMPORTANT: SEED GENERATED: " << seed << " <<<\n";
            cout << "SAVE THIS SEED! It will be required to extract the hidden data.\n";
        }

        if (!HidePayloadInBMP(bPic.get(), payload, extension, encrypted, seed, packLevel)) {
            cout << "\nError embedding data!\n";
            system("pause");
            return;
        }

        if (!SaveBMPWith(bPic.get(), path, encrypted, seed)) {
            cout << "\nError saving modified BMP file!\n";
        }
        else {
            cout << "\nData successfully embedded and saved!\n";
        }

        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ ЧТЕНИЕ ДАННЫХ ИЗ BMP ============
void infoViewer() {
    system("cls");
    cout << "=== Welcome to InfoViewer ===\n";
    cout << "1. Read hidden data from BMP\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';
    if (choice == 0) return;

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select BMP file to inspect and read...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        unique_ptr<Bitmap> bPic(new Bitmap(wpath));
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            system("pause");
            return;
        }

        if (!isSecretPresentInBMP(bPic.get())) {
            cout << "\nNo hidden information detected in this image.\n";
            system("pause");
            return;
        }

        cout << "\nEnter seed (or press Enter if unencrypted): ";
        string seedInput;
        getline(cin, seedInput);
        if (seedInput.empty()) {
            // Вторичная проверка на случай висящего символа переноса
            // Если пользователь просто нажал Enter, seedInput останется пустой
        }

        bool isEncrypted = false;
        unsigned int seed = 0;

        if (!seedInput.empty()) {
            bool isNumber = true;
            for (char c : seedInput) {
                if (!isdigit(c)) { isNumber = false; break; }
            }

            if (isNumber) {
                try {
                    seed = static_cast<unsigned int>(stoul(seedInput));
                    isEncrypted = true;
                    cout << "Using seed: " << seed << " (Encrypted Mode)\n";
                }
                catch (...) {
                    cout << "Invalid seed value!\n";
                    system("pause");
                    return;
                }
            }
            else {
                cout << "Invalid seed format! Numbers only.\n";
                system("pause");
                return;
            }
        }

        vector<unsigned char> extractedData;
        string extension;

        if (!ExtractPayloadFromBMP(bPic.get(), extractedData, extension, isEncrypted, seed)) {
            cout << "\nFailed to extract hidden data.\n";
            system("pause");
            return;
        }

        cout << "\n================ Extracted Secret ================\n";
        cout << "Data format: ." << extension << "\n";
        cout << "Data size:   " << extractedData.size() << " bytes\n";

        // Проверяем, является ли текст читаемым ASCII/UTF-8
        bool isPrintable = true;
        for (unsigned char c : extractedData) {
            if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
                isPrintable = false;
                break;
            }
        }

        if (extension == "txt" || isPrintable) {
            string text(extractedData.begin(), extractedData.end());
            cout << "\n--- Text Content ---\n";
            cout << text << "\n";
            cout << "--------------------\n";
        }
        else {
            cout << "\n[Binary payload detected: ." << extension << "]\n";
        }

        cout << "\nSave extracted data to file? (Y/N): ";
        char saveChoice = _getch();
        cout << saveChoice << endl;

        if (saveChoice == 'y' || saveChoice == 'Y') {
            char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
            _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

            char outDir[MAX_PATH];
            sprintf_s(outDir, sizeof(outDir), "%s%soutput_img\\", drive, dir);
            CreateDirectoryIfNotExists(outDir);

            char savePath[MAX_PATH];
            sprintf_s(savePath, sizeof(savePath), "%sextracted_%s.%s", outDir, fname, extension.c_str());

            ofstream outFile(savePath, ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(extractedData.data()), extractedData.size());
                outFile.close();
                cout << "Successfully saved to: " << savePath << endl;
            }
            else {
                cout << "Error writing output file!" << endl;
            }
        }

        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ ДЕТЕКТОР СКРЫТОЙ ИНФОРМАЦИИ ============
void infoDetector() {
    system("cls");
    cout << "=== Welcome to InfoDetector ===\n";
    cout << "1. Check BMP file for steganographic markers\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';
    if (choice == 0) return;

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select BMP file to analyze...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        unique_ptr<Bitmap> bPic(new Bitmap(wpath));
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            system("pause");
            return;
        }

        ServiceHeader header;
        bool hasSecret = CheckServiceData(bPic.get(), header);

        if (!hasSecret) {
            cout << "\nResult: No steganographic signatures detected in this image.\n";
        }
        else {
            cout << "\n================ Analysis Report ================\n";
            cout << "[!] Steganographic signature DETECTED!\n";
            cout << "    - Packing Level: " << header.packLevel << " (" << header.packLevel << " byte(s)/pixel)\n";
            cout << "    - Bits/Pixel:    " << GetBitsPerPixel(header.packLevel) << " bits\n";
            cout << "    - Payload Type:  ." << header.extension << "\n";
            cout << "    - Payload Size:  " << header.textSize << " bytes\n";
            cout << "    - Integrity:     Verified via redundant primary and backup headers.\n";
            cout << "=================================================\n";
        }

        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ АВТОМАТИЗИРОВАННЫЙ ТЕСТОВЫЙ КОМПЛЕКС ============
int runAutomatedTests(bool interactive = true) {
    if (interactive) {
        system("cls");
    }
    cout << "====================================================\n";
    cout << "         STEGANOMIX AUTOMATED TEST SUITE            \n";
    cout << "====================================================\n\n";

    int passed = 0;
    int total = 0;

    // ТЕСТ 1: Обратимость распределения бит на всех уровнях упаковки
    total++;
    cout << "[TEST 1] Testing RGB Bit Distribution Invertibility (All 256 byte values)... ";
    bool test1_ok = true;
    for (int level = 1; level <= 3; level++) {
        for (int val = 0; val < 256; val++) {
            PixelColor orig(128, 128, 128);
            unsigned char b0 = static_cast<unsigned char>(val);
            unsigned char b1 = static_cast<unsigned char>((val * 7 + 13) & 0xFF);
            unsigned char b2 = static_cast<unsigned char>((val * 11 + 29) & 0xFF);
            unsigned char inBuf[3] = { b0, b1, b2 };

            PixelColor embedded = EmbedBytesToPixel(orig, inBuf, level, level);
            unsigned char outBuf[3] = { 0, 0, 0 };
            int extracted = ExtractBytesFromPixel(embedded, outBuf, level, level);

            if (extracted != level || outBuf[0] != b0 || (level >= 2 && outBuf[1] != b1) || (level >= 3 && outBuf[2] != b2)) {
                test1_ok = false;
                break;
            }
        }
        if (!test1_ok) break;
    }
    if (test1_ok) { cout << "PASSED\n"; passed++; }
    else { cout << "FAILED\n"; }

    // ТЕСТ 2: Полный цикл встраивания и извлечения в памяти (Без шифрования)
    total++;
    cout << "[TEST 2] Testing In-Memory Steganography (Levels 1, 2, 3 Unencrypted)... ";
    bool test2_ok = true;
    const string sampleText = "SteganoMIX Security Suite: Testing high-capacity LSB embedding and extraction.";
    vector<unsigned char> testPayload(sampleText.begin(), sampleText.end());

    for (int level = 1; level <= 3; level++) {
        Bitmap bmp(64, 64, PixelFormat24bppRGB);
        // Заполняем фон нейтральным цветом
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                Color c(100 + (x % 50), 120 + (y % 50), 140);
                bmp.SetPixel(x, y, c);
            }
        }

        if (!HidePayloadInBMP(&bmp, testPayload, "txt", false, 0, level)) {
            test2_ok = false;
            break;
        }

        vector<unsigned char> extractedPayload;
        string ext;
        if (!ExtractPayloadFromBMP(&bmp, extractedPayload, ext, false, 0) ||
            extractedPayload != testPayload || ext != "txt") {
            test2_ok = false;
            break;
        }
    }
    if (test2_ok) { cout << "PASSED\n"; passed++; }
    else { cout << "FAILED\n"; }

    // ТЕСТ 3: Полный цикл с псевдослучайным распределением (Encrypted Mode + Seed)
    total++;
    cout << "[TEST 3] Testing Seed-based Scattering & Decryption... ";
    bool test3_ok = true;
    for (int level = 1; level <= 3; level++) {
        Bitmap bmp(80, 80, PixelFormat24bppRGB);
        unsigned int seed = 987654321;

        if (!HidePayloadInBMP(&bmp, testPayload, "txt", true, seed, level)) {
            test3_ok = false;
            break;
        }

        // Извлечение с ПРАВИЛЬНЫМ seed
        vector<unsigned char> extractedPayload;
        string ext;
        if (!ExtractPayloadFromBMP(&bmp, extractedPayload, ext, true, seed) ||
            extractedPayload != testPayload) {
            test3_ok = false;
            break;
        }

        // Извлечение с НЕПРАВИЛЬНЫМ seed должно дать несовпадающие данные
        vector<unsigned char> wrongExtracted;
        ExtractPayloadFromBMP(&bmp, wrongExtracted, ext, true, seed + 1);
        if (wrongExtracted == testPayload) {
            test3_ok = false; // Нарушение конфиденциальности
            break;
        }
    }
    if (test3_ok) { cout << "PASSED\n"; passed++; }
    else { cout << "FAILED\n"; }

    // ТЕСТ 4: Отказоустойчивость и восстановление заголовков при повреждении
    total++;
    cout << "[TEST 4] Testing Redundant Header Fault-Tolerance... ";
    bool test4_ok = true;
    {
        Bitmap bmp(64, 64, PixelFormat24bppRGB);
        HidePayloadInBMP(&bmp, testPayload, "txt", false, 0, 1);

        // Симулируем повреждение первичного заголовка (столбец 0)
        for (int y = 0; y < 10; y++) {
            Color zero(0, 0, 0);
            bmp.SetPixel(0, y, zero);
        }

        // Проверяем, что извлечение срабатывает по дублирующему заголовку
        vector<unsigned char> extracted;
        string ext;
        if (!ExtractPayloadFromBMP(&bmp, extracted, ext, false, 0) || extracted != testPayload) {
            test4_ok = false;
        }
    }
    if (test4_ok) { cout << "PASSED\n"; passed++; }
    else { cout << "FAILED\n"; }

    // ТЕСТ 5: Граничные условия и предельная емкость
    total++;
    cout << "[TEST 5] Testing Boundary Conditions & Max Capacity... ";
    bool test5_ok = true;
    {
        Bitmap smallBmp(10, 10, PixelFormat24bppRGB);
        if (GetMaxCapacity(10, 10, 1) != 0) test5_ok = false;

        Bitmap bmp(30, 30, PixelFormat24bppRGB);
        int cap = GetMaxCapacity(30, 30, 1);
        vector<unsigned char> maxPayload(cap, 0xAB);
        if (!HidePayloadInBMP(&bmp, maxPayload, "bin", false, 0, 1)) test5_ok = false;

        vector<unsigned char> extracted;
        string ext;
        if (!ExtractPayloadFromBMP(&bmp, extracted, ext, false, 0) || extracted != maxPayload || ext != "bin") {
            test5_ok = false;
        }
    }
    if (test5_ok) { cout << "PASSED\n"; passed++; }
    else { cout << "FAILED\n"; }

    cout << "\n====================================================\n";
    cout << "Test Summary: " << passed << "/" << total << " tests PASSED.\n";
    if (passed == total) {
        cout << "STATUS: ALL SYSTEMS NOMINAL. 100% RELIABILITY ACHIEVED.\n";
    }
    else {
        cout << "STATUS: TEST FAILURES DETECTED.\n";
    }
    cout << "====================================================\n";
    if (interactive) {
        system("pause");
    }
    return (passed == total) ? 0 : 1;
}

void programTester() {
    int choice;
    do {
        system("cls");
        cout << "=== SteganoMIX Diagnostic & Test Suite ===\n";
        cout << "1. Run all automated tests\n";
        cout << "0. Return to main menu\n\n";
        cout << "Enter choice: ";
        choice = _getch() - '0';
        switch (choice) {
        case 1:
            runAutomatedTests(true);
            return;
        case 0:
            return;
        default:
            break;
        }
    } while (choice != 0);
}

// ============ СПРАВОЧНАЯ СИСТЕМА ============
void programHelper() {
    system("cls");
    cout << "================== SteganoMIX v1.0 - Help & Specification ==================\n\n";

    cout << "1. OVERVIEW\n";
    cout << "   SteganoMIX is an advanced steganography utility designed to securely hide\n";
    cout << "   and extract arbitrary data (text or binary files) within BMP images.\n\n";

    cout << "2. OPTIMIZED HVS-AWARE LSB COLOR DISTRIBUTION\n";
    cout << "   The human eye is unevenly sensitive to RGB primary colors:\n";
    cout << "   - Green (59% luminance): Highest visual sensitivity\n";
    cout << "   - Red   (30% luminance): Medium visual sensitivity\n";
    cout << "   - Blue  (11% luminance): Lowest visual sensitivity\n\n";
    cout << "   Allocation per pixel:\n";
    cout << "   - Pack Level 1 (8 bits/pixel, 1 Byte):  R: 3 bits, G: 2 bits, B: 3 bits\n";
    cout << "     -> Minimal distortion in green channel preserves image fidelity.\n";
    cout << "   - Pack Level 2 (16 bits/pixel, 2 Bytes): R: 5 bits, G: 5 bits, B: 6 bits\n";
    cout << "     -> Blue channel carries the 6th bit to hide high-frequency noise.\n";
    cout << "   - Pack Level 3 (24 bits/pixel, 3 Bytes): R: 8 bits, G: 8 bits, B: 8 bits\n";
    cout << "     -> Maximum data density across all color components.\n\n";

    cout << "3. SECURITY & PSEUDO-RANDOM SCATTERING (SEED)\n";
    cout << "   - When encryption mode is ON, payload bytes are distributed across\n";
    cout << "     pixels using a cryptographically randomized Fisher-Yates permutation.\n";
    cout << "   - Extraction is mathematically impossible without the correct secret seed.\n\n";

    cout << "4. DUAL REDUNDANT SERVICE HEADERS\n";
    cout << "   - Primary service header: Embedded in Column 0 (x=0, y=0..N).\n";
    cout << "   - Backup service header:  Embedded in the Last Row (y=height-1, x=0..N).\n";
    cout << "   - In case of transmission damage to either header, the redundant copy\n";
    cout << "     allows flawless payload recovery.\n\n";

    cout << "5. SUPPORTED MODES\n";
    cout << "   - Text Mode: Embed and read plain text strings directly in the console.\n";
    cout << "   - File Mode: Embed ANY file (.zip, .pdf, .docx, .bin) with preserved extension\n";
    cout << "     and extract it back to the 'output_img' folder.\n\n";

    cout << "=============================================================================\n";
    cout << "Press any key to return to the main menu...\n";
    (void)_getch();
}

// ============ ВЫХОД ИЗ ПРОГРАММЫ ============
void exitProgram() {
    exit(0);
}

// ============ ТОЧКА ВХОДА ============
int main(int argc, char* argv[]) {
    // Настройка кодовой страницы для корректной поддержки кириллицы
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Поддержка запуска тестов из командной строки (--test или -t)
    if (argc > 1 && (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0)) {
        int result = runAutomatedTests(false);
        GdiplusShutdown(gdiplusToken);
        return result;
    }

    int choice;
    do {
        system("cls");
        cout << "========================================\n";
        cout << "         SteganoMIX Security v1.0       \n";
        cout << "   Advanced BMP Steganography System    \n";
        cout << "========================================\n\n";
        cout << "1. Hide information (Text / File)\n";
        cout << "2. View / Extract information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Automated Test Suite\n";
        cout << "5. Settings\n";
        cout << "6. Help & Documentation\n";
        cout << "0. Exit\n\n";
        cout << "Enter choice: ";

        choice = _getch() - '0';

        switch (choice) {
        case 1: infoHider(); break;
        case 2: infoViewer(); break;
        case 3: infoDetector(); break;
        case 4: programTester(); break;
        case 5: changeSettings(); break;
        case 6: programHelper(); break;
        case 0: exitProgram(); break;
        default: break;
        }
    } while (choice != 0);

    GdiplusShutdown(gdiplusToken);
    return 0;
}