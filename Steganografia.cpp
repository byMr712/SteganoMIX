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
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ============ ОБЪЯВЛЕНИЯ ФУНКЦИЙ ============
void infoHider();
void infoViewer();
void infoDetector();
void programTester();
void changeSettings();
void programHelper();
void exitProgram();

// ============ БИБЛИОТЕКИ / ЗАГОЛОВОЧНЫЕ ФАЙЛЫ ============
using namespace std;
using namespace Gdiplus;

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct ProgramSettings {
    bool useEncryption = true;
    unsigned int currentSeed = 0;
    bool useSuffix = true;
    bool useSeedSuffix = true;
    int packLevel = 3;
};

ProgramSettings g_settings;

// ============ ОСНОВНАЯ ПРОГРАММА ============
int main() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int choice;
    do {
        system("cls");
        cout << "=== SteganoMIX v1.03 ===";
        cout << "\nHide and view your information in a BMP file\n";
        cout << "\n=== Main menu ===\n";
        cout << "1. Hide information\n";
        cout << "2. View information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Tests this program\n";
        cout << "5. Settings\n";
        cout << "6. Help\n";
        cout << "0. Exit\n\n";
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

// ============ СТРУКТУРА ЦВЕТА ============
struct PixelColor {
    uint8_t R, G, B;
    PixelColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : R(r), G(g), B(b) {}
};

// ============ БАЗОВЫЕ ФУНКЦИИ ============
bitset<8> ByteToBit(unsigned char src) {
    return bitset<8>(src);
}

unsigned char BitToByte(const bitset<8>& scr) {
    return static_cast<unsigned char>(scr.to_ulong());
}

PixelColor EmbedSymbolToColor(const PixelColor& curColor, unsigned char symbol) {
    bitset<8> symbolBits = ByteToBit(symbol);
    PixelColor result = curColor;

    bitset<8> tempR = ByteToBit(curColor.R);
    tempR[0] = symbolBits[0];
    tempR[1] = symbolBits[1];
    result.R = BitToByte(tempR);

    bitset<8> tempG = ByteToBit(curColor.G);
    tempG[0] = symbolBits[2];
    tempG[1] = symbolBits[3];
    tempG[2] = symbolBits[4];
    result.G = BitToByte(tempG);

    bitset<8> tempB = ByteToBit(curColor.B);
    tempB[0] = symbolBits[5];
    tempB[1] = symbolBits[6];
    tempB[2] = symbolBits[7];
    result.B = BitToByte(tempB);

    return result;
}

bool isEncryption(const PixelColor& pixelColor) {
    unsigned char extracted = 0;
    extracted |= (pixelColor.R & 0x03);
    extracted |= ((pixelColor.G & 0x07) << 2);
    extracted |= ((pixelColor.B & 0x07) << 5);
    return (extracted == '/');
}

// ============ СТЕПЕНЬ УПАКОВКИ ============
int GetBitsPerChannel(int packLevel) {
    if (packLevel >= 8) return 8;
    return packLevel;
}

int GetBitsPerPixel(int packLevel) {
    if (packLevel >= 8) return 24;
    return packLevel * 3;
}

int GetMaxCapacity(int width, int height, int packLevel) {
    int totalPixels = width * height;
    int bitsPerPixel = GetBitsPerPixel(packLevel);
    int availablePixels = totalPixels - 4;

    // ============ ПРОВЕРКА ============
    if (availablePixels <= 0) {
        return 0;  // Нельзя ничего записать
    }

    return (availablePixels * bitsPerPixel) / 8;
}

unsigned char GetBitMask(int packLevel) {
    if (packLevel >= 8) return 0xFF;
    return (1 << packLevel) - 1;
}

PixelColor EmbedSymbolToColorLevel(const PixelColor& curColor, unsigned char symbol, int packLevel) {
    if (packLevel <= 0 || packLevel > 9) packLevel = 3;

    PixelColor result = curColor;
    int bitsPerChannel = GetBitsPerChannel(packLevel);
    unsigned char mask = GetBitMask(bitsPerChannel);

    result.R = (curColor.R & ~mask) | (symbol & mask);
    result.G = (curColor.G & ~mask) | ((symbol >> bitsPerChannel) & mask);
    result.B = (curColor.B & ~mask) | ((symbol >> (bitsPerChannel * 2)) & mask);

    return result;
}

unsigned char ExtractSymbolFromColorLevel(const PixelColor& color, int packLevel) {
    if (packLevel <= 0 || packLevel > 9) packLevel = 3;

    int bitsPerChannel = GetBitsPerChannel(packLevel);
    unsigned char mask = GetBitMask(bitsPerChannel);

    unsigned char extracted = 0;
    extracted |= (color.R & mask);
    extracted |= ((color.G & mask) << bitsPerChannel);
    extracted |= ((color.B & mask) << (bitsPerChannel * 2));

    return extracted;
}

unsigned char ExtractSymbolFromColor(const PixelColor& color) {
    unsigned char extracted = 0;
    extracted |= (color.R & 0x03);
    extracted |= ((color.G & 0x07) << 2);
    extracted |= ((color.B & 0x07) << 5);
    return extracted;
}

// ============ РАБОТА С BMP ============
PixelColor GetPixelColor(Bitmap* bPic, int x, int y) {
    Color color;
    bPic->GetPixel(x, y, &color);
    return PixelColor(
        static_cast<uint8_t>(color.GetR()),
        static_cast<uint8_t>(color.GetG()),
        static_cast<uint8_t>(color.GetB())
    );
}

void SetPixelColor(Bitmap* bPic, int x, int y, const PixelColor& color) {
    Color gdiColor(color.R, color.G, color.B);
    bPic->SetPixel(x, y, gdiColor);
}

bool isEncryptionInBMP(Bitmap* bPic) {
    PixelColor color = GetPixelColor(bPic, 0, 0);
    return isEncryption(color);
}

bool isEncryptionInBMPLevel(Bitmap* bPic, int packLevel) {
    PixelColor color = GetPixelColor(bPic, 0, 0);
    return (ExtractSymbolFromColorLevel(color, packLevel) == '/');
}

// ============ РАБОТА С РАЗМЕРОМ ============
void WriteCountText(int count, Bitmap* src) {
    char countStr[16] = { 0 };
    sprintf_s(countStr, sizeof(countStr), "%04d", count);  // ← 4 цифры

    for (int i = 0; i < 4; i++) {
        unsigned char symbol = static_cast<unsigned char>(countStr[i]);
        PixelColor curColor = GetPixelColor(src, 0, i + 1);
        PixelColor newColor = EmbedSymbolToColor(curColor, symbol);
        SetPixelColor(src, 0, i + 1, newColor);
    }

}

void WriteCountTextLevel(int count, Bitmap* src, int packLevel) {
    char countStr[16] = { 0 };
    sprintf_s(countStr, sizeof(countStr), "%04d", count);  // ← 4 цифры ("3543")

    for (int i = 0; i < 4; i++) {  // ← 4 пикселя
        unsigned char symbol = static_cast<unsigned char>(countStr[i]);
        PixelColor curColor = GetPixelColor(src, 0, i + 1);
        PixelColor newColor = EmbedSymbolToColorLevel(curColor, symbol, packLevel);
        SetPixelColor(src, 0, i + 1, newColor);
    }
}

int ReadCountText(Bitmap* src) {
    int result = 0;

    for (int i = 0; i < 4; i++) {
        PixelColor color = GetPixelColor(src, 0, i + 1);
        unsigned char ch = ExtractSymbolFromColor(color);

        if (ch >= '0' && ch <= '9') {
            result = result * 10 + (ch - '0');
        }
        else {
            result = result * 10 + 0;
        }
    }

    return result;
}

int ReadCountTextLevel(Bitmap* src, int packLevel) {
    int result = 0;

    for (int i = 0; i < 4; i++) {  // ← 4 пикселя
        PixelColor color = GetPixelColor(src, 0, i + 1);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

        if (ch >= '0' && ch <= '9') {
            result = result * 10 + (ch - '0');
        }
        else {
            result = result * 10 + 0;
        }
    }

    return result;
}

// ============ ГЕНЕРАЦИЯ SEED ============
unsigned int GenerateSeed(int width, int height) {
    unsigned int seed = (width * 31) ^ (height * 17);
    seed ^= static_cast<unsigned int>(time(nullptr));
    seed ^= static_cast<unsigned int>(GetTickCount64());
    seed = (seed << 13) ^ seed;
    seed = (seed >> 7) ^ seed;
    seed = (seed << 17) ^ seed;
    return seed;
}

// ============ ГЕНЕРАЦИЯ ПОЗИЦИЙ ============
struct PixelPosition {
    int x, y;
};

#include <random>  // Добавь в начале файла

vector<PixelPosition> GenerateRandomPositions(int width, int height, int count, unsigned int seed) {
    vector<PixelPosition> positions;
    positions.reserve(count);

    std::mt19937 rng(seed);

    vector<PixelPosition> allPositions;
    for (int i = 4; i < width; i++) {
        for (int j = 0; j < height; j++) {
            allPositions.push_back({ i, j });
        }
    }

    for (int i = static_cast<int>(allPositions.size()) - 1; i > 0; i--) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        swap(allPositions[i], allPositions[j]);
    }

    for (int i = 0; i < count && i < static_cast<int>(allPositions.size()); i++) {
        positions.push_back(allPositions[i]);
    }

    return positions;
}

// ============ ЗАПИСЬ КОЛИЧЕСТВА СИМВОЛОВ ДО РАЗДЕЛИТЕЛЯ ============
void WriteCountTextDynamic(int count, Bitmap* src, int packLevel) {
    string countStr = to_string(count) + ":";

    for (size_t i = 0; i < countStr.length(); i++) {
        unsigned char symbol = static_cast<unsigned char>(countStr[i]);
        PixelColor curColor = GetPixelColor(src, 0, static_cast<int>(i) + 1);
        PixelColor newColor = EmbedSymbolToColorLevel(curColor, symbol, packLevel);
        SetPixelColor(src, 0, static_cast<int>(i) + 1, newColor);
    }
}

// ============ ЧТЕНИЕ КОЛИЧЕСТВА СИМВОЛОВ ДО РАЗДЕЛИТЕЛЯ ============
int ReadCountTextDynamic(Bitmap* src, int packLevel) {
    string countStr = "";
    int i = 1;  // начинаем с пикселя (0,1)

    while (true) {
        PixelColor color = GetPixelColor(src, 0, i);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

        if (ch == ':') {
            break;  // Остановка на разделителе
        }

        if (ch >= '0' && ch <= '9') {
            countStr += static_cast<char>(ch);
        }
        else {
            // Если встретим не цифру и не ':' — возвращаем 0
            return 0;
        }

        i++;
    }

    if (countStr.empty()) {
        return 0;
    }

    return stoi(countStr);
}

// ============ ЗАПИСЬ ТЕКСТА ============
void HideTextInBMP(Bitmap* bPic, const string& text) {
    vector<unsigned char> bList;
    for (size_t i = 0; i < text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    if (CountText > (width * height) - 4) {
        MessageBox(NULL, L"No free space in the image", L"Information", MB_OK);
        return;
    }

    if (isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColor(pixel00, '/'));

    WriteCountText(CountText, bPic);

    int index = 0;
    for (int i = 4; i < width && index < CountText; i++) {
        for (int j = 0; j < height && index < CountText; j++) {
            PixelColor pixelColor = GetPixelColor(bPic, i, j);
            PixelColor newColor = EmbedSymbolToColor(pixelColor, bList[index]);
            SetPixelColor(bPic, i, j, newColor);
            index++;
        }
    }
}

void HideTextInBMP_Encrypted(Bitmap* bPic, const string& text, unsigned int seed, const char* extension) {
    // ============ ДОБАВЛЯЕМ ТИП ФАЙЛА В КОНЕЦ ТЕКСТА ============
    string fullText = text + '\0' + extension;

    vector<unsigned char> bList;
    for (size_t i = 0; i < fullText.length(); i++) {
        bList.push_back(static_cast<unsigned char>(fullText[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int packLevel = g_settings.packLevel;

    int availablePixels = (width * height) - 1;
    int bitsPerPixel = GetBitsPerPixel(packLevel);
    int capacity = (availablePixels * bitsPerPixel) / 8;
    capacity -= 11;  // под размер

    if (CountText > capacity) {
        string msg = "Text too large!\nText size: " + to_string(CountText) +
            " bytes\nCapacity: " + to_string(capacity) + " bytes\nPack level: " + to_string(packLevel);
        wstring wmsg(msg.begin(), msg.end());
        MessageBox(NULL, wmsg.c_str(), L"Information", MB_OK);
        return;
    }

    if (isEncryptionInBMPLevel(bPic, packLevel)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    // 1. Признак
    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColorLevel(pixel00, '/', packLevel));

    // 2. Размер
    WriteCountTextDynamic(CountText, bPic, packLevel);

    // 3. Текст + тип файла (вместе!)
    vector<PixelPosition> positions = GenerateRandomPositions(width, height, CountText, seed);
    for (int i = 0; i < CountText && i < (int)positions.size(); i++) {
        PixelColor pixelColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
        PixelColor newColor = EmbedSymbolToColorLevel(pixelColor, bList[i], packLevel);
        SetPixelColor(bPic, positions[i].x, positions[i].y, newColor);
    }

    // 4. ТИП ФАЙЛА УЖЕ В ТЕКСТЕ! НИЧЕГО ДОПОЛНИТЕЛЬНО НЕ ПИШЕМ!
}

// ============ ЧТЕНИЕ ТЕКСТА ============
string ReadTextFromBMP_Encrypted(Bitmap* bPic, unsigned int seed, const char* filePath) {
    int packLevel = g_settings.packLevel;

    if (!isEncryptionInBMPLevel(bPic, packLevel)) {
        MessageBox(NULL, L"No hidden information", L"Information", MB_OK);
        return "";
    }

    int countSymbol = ReadCountTextDynamic(bPic, packLevel);

    if (countSymbol <= 0) {
        return "";
    }

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    // 1. Читаем ВЕСЬ текст (с типом файла внутри)
    vector<PixelPosition> positions = GenerateRandomPositions(width, height, countSymbol, seed);
    vector<unsigned char> fullData;
    fullData.reserve(countSymbol);

    for (int i = 0; i < countSymbol && i < (int)positions.size(); i++) {
        PixelColor pixelColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
        fullData.push_back(ExtractSymbolFromColorLevel(pixelColor, packLevel));
    }

    // 2. Находим разделитель '\0'
    string fullString(fullData.begin(), fullData.end());
    size_t sepPos = fullString.find('\0');

    if (sepPos == string::npos) {
        // Нет разделителя — значит тип файла не сохранялся
        cout << "No file type found (default: .txt)\n";
        return fullString;
    }

    // 3. Отделяем текст от типа файла
    string hiddenText = fullString.substr(0, sepPos);
    string extension = fullString.substr(sepPos + 1);

    if (!extension.empty()) {
        cout << "File type detected: ." << extension << endl;
    }
    else {
        cout << "No file type found (default: .txt)\n";
    }

    return hiddenText;
}

string ReadTextFromBMP(Bitmap* bPic) {
    if (!isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"Нет зашифрованной информации", L"Информация", MB_OK);
        return "";
    }

    int countSymbol = ReadCountText(bPic);
    if (countSymbol <= 0) {
        return "";
    }

    vector<unsigned char> message;
    message.reserve(countSymbol);

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    for (int i = 4; i < width && message.size() < countSymbol; i++) {
        for (int j = 0; j < height && message.size() < countSymbol; j++) {
            PixelColor pixelColor = GetPixelColor(bPic, i, j);
            message.push_back(ExtractSymbolFromColor(pixelColor));
        }
    }

    return string(message.begin(), message.end());
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
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

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

    for (size_t i = 0; i < strlen(tempPath); i++) {
        if (tempPath[i] == '\\' || tempPath[i] == '/') {
            tempPath[i] = '\0';
            CreateDirectoryA(tempPath, NULL);
            tempPath[i] = '\\';
        }
    }
    CreateDirectoryA(tempPath, NULL);
    return true;
}

// ============ СОХРАНЕНИЕ BMP ============
const CLSID CLSID_BMP = { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } };

bool SaveBMPWith(Bitmap* bPic, const char* originalPath, bool encrypted, unsigned int seed) {
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    _splitpath_s(originalPath, drive, _MAX_DRIVE, dir, _MAX_DIR,
        fname, _MAX_FNAME, ext, _MAX_EXT);

    char outputDir[MAX_PATH];
    sprintf_s(outputDir, sizeof(outputDir), "%s%soutput_img\\", drive, dir);
    CreateDirectoryIfNotExists(outputDir);

    char fullName[MAX_PATH];

    if (g_settings.useSuffix) {
        if (encrypted) {
            sprintf_s(fullName, sizeof(fullName), "%s%s_encrypted", outputDir, fname);
            if (g_settings.useSeedSuffix) {
                char seedStr[16];
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

    strcat_s(fullName, ext);

    WCHAR wnewPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, fullName, -1, wnewPath, MAX_PATH);

    if (bPic->Save(wnewPath, &CLSID_BMP, NULL) != Ok) {
        return false;
    }

    cout << "Saved as: " << fullName << endl;
    return true;
}

// ============ НАСТРОЙКИ ============
void showSettings() {
    system("cls");
    cout << "=== Settings ===\n";
    cout << "1. Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << "\n";
    cout << "2. Add suffix to filename: " << (g_settings.useSuffix ? "ON" : "OFF") << "\n";
    cout << "3. Add seed to filename: " << (g_settings.useSeedSuffix ? "ON" : "OFF") << "\n";
    cout << "4. Pack level (1-9): " << g_settings.packLevel << "\n";
    cout << "   Bits per pixel: " << GetBitsPerPixel(g_settings.packLevel) << "\n";
    cout << "0. Return\n\n";
}

void changeSettings() {
    int choice;
    do {
        showSettings();
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
            cout << "\nEnter pack level (1-9): ";
            int level;
            cin >> level;
            if (level >= 1 && level <= 9) {
                g_settings.packLevel = level;
                cout << "Pack level set to: " << level << "\n";
                cout << "Bits per pixel: " << GetBitsPerPixel(level) << "\n";
            }
            else {
                cout << "Invalid level! Use 1-9.\n";
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

// ============ infoHider ============
void infoHider() {
    system("cls");
    cout << "=== Welcome to InfoHider ===\n";
    cout << "1. LSB Algorithm (Least Significant Bit)\n";
    cout << "2. FF Algorithm (For Future)\n";
    cout << "3. Settings\n";
    cout << "0. Return to menu\n\n";

    int alg = _getch() - '0';

    if (alg == 0) {
        return;
    }

    if (alg == 3) {
        changeSettings();
        return;
    }

    if (alg == 1) {
        system("cls");
        cout << "=== LSB Algorithm ===\n\n";

        char path[MAX_PATH];
        cout << "Select a BMP file to hide information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected. Operation cancelled.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        if (isEncryptionInBMP(bPic)) {
            cout << "\nThis file already contains hidden information!\n";
            cout << "Use 'View information' to read the hidden text.\n";
            delete bPic;
            system("pause");
            return;
        }

        int width = bPic->GetWidth();
        int height = bPic->GetHeight();
        int packLevel = g_settings.packLevel;
        int capacity = GetMaxCapacity(width, height, packLevel) - 4;

        cout << "File successfully loaded: " << path << endl;
        cout << "Size: " << width << "x" << height << endl;
        cout << "Max capacity: " << capacity << " bytes" << endl;
        cout << "Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << endl;
        cout << "Pack level: " << packLevel << " (" << GetBitsPerPixel(packLevel) << " bits per pixel)\n";

        cout << "\nInput your text: ";
        string text;
        getline(cin, text);

        if (text.empty()) {
            cout << "Text cannot be empty!\n";
            delete bPic;
            system("pause");
            return;
        }

        // ============ ВЫВОДИМ РАЗМЕР ТЕКСТА ============
        int textBytes = static_cast<int>(text.length());
        cout << "\nText length: " << text.length() << " symbols (" << textBytes << " bytes)" << endl;

        bool encrypted = g_settings.useEncryption;
        unsigned int seed = 0;

        if (encrypted) {
            // ============ ПРОВЕРКА ВМЕСТИМОСТИ ДО ГЕНЕРАЦИИ SEED ============
            if (textBytes > capacity) {
                cout << "\nERROR: Text too large for this image!\n";
                cout << "Text size: " << textBytes << " bytes\n";
                cout << "Capacity: " << capacity << " bytes\n";
                cout << "Pack level: " << packLevel << " (" << GetBitsPerPixel(packLevel) << " bits per pixel)\n";
                cout << "Try increasing pack level or use a larger image.\n";
                delete bPic;
                system("pause");
                return;
            }

            seed = GenerateSeed(width, height);
            g_settings.currentSeed = seed;
            cout << "Seed generated: " << seed << endl;
            cout << "SAVE THIS SEED to decrypt later!\n";

            string extension = "";
            const char* p = strrchr(path, '.');
            if (p != nullptr) {
                extension = string(p + 1);
            }
            if (extension.empty()) {
                extension = "txt";
            }

            cout << "File type: ." << extension << " (will be hidden in image)\n";
            HideTextInBMP_Encrypted(bPic, text, seed, extension.c_str());
        }
        else {
            // ============ ПРОВЕРКА ВМЕСТИМОСТИ ДЛЯ ОБЫЧНОГО РЕЖИМА ============
            int normalCapacity = (width * height) - 4;
            if (textBytes > normalCapacity) {
                cout << "\nERROR: Text too large for this image!\n";
                cout << "Text size: " << textBytes << " bytes\n";
                cout << "Capacity: " << normalCapacity << " bytes\n";
                delete bPic;
                system("pause");
                return;
            }
            HideTextInBMP(bPic, text);
        }

        if (!SaveBMPWith(bPic, path, encrypted, seed)) {
            cout << "\nError saving file!\n";
        }
        else {
            cout << "\nInformation successfully hidden and saved!\n";
        }

        delete bPic;
        system("pause");
        return;
    }

    if (alg == 2) {
        system("cls");
        cout << "=== FF Algorithm ===\n";
        cout << "Error 404\n";
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

void infoViewer() {
    system("cls");
    cout << "=== Welcome to InfoViewer ===\n";
    cout << "1. Read hidden text\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';

    if (choice == 0) {
        return;
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to read hidden information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        cout << "\nEnter seed (or press Enter for unencrypted text): ";
        string seedInput;
        getline(cin, seedInput);

        string hiddenText;

        if (!seedInput.empty()) {
            // ============ СОХРАНЯЕМ ОРИГИНАЛЬНУЮ СТРОКУ ============
            string originalSeed = seedInput;

            // Убираем ведущие нули только для преобразования
            unsigned int seed = 0;
            try {
                seed = static_cast<unsigned int>(stoul(seedInput));
            }
            catch (...) {
                seed = 0;
            }

            cout << "Using seed: " << originalSeed << " (encrypted mode)\n";

            // Передаём seed в функцию — она должна использовать его для позиций
            hiddenText = ReadTextFromBMP_Encrypted(bPic, seed, path);
        }

        if (!hiddenText.empty()) {
            cout << "\n=== Hidden text ===\n";
            cout << hiddenText << endl;
            cout << "==================\n";
            cout << "Text length: " << hiddenText.length() << " symbols\n";
            cout << "==================\n";
        }
        else {
            cout << "\nNo text found.\n";
        }

        delete bPic;
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

void infoDetector() {
    system("cls");
    cout << "=== Welcome to InfoDetector ===\n";
    cout << "1. Check BMP file\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';

    if (choice == 0) {
        return;
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to check...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        int packLevel = g_settings.packLevel;

        if (isEncryptionInBMPLevel(bPic, packLevel)) {
            cout << "Hidden information detected!\n";
            int count = ReadCountTextLevel(bPic, packLevel);
            if (count > 0) {
                cout << "   Text size: " << count << " symbols\n";
            }
        }
        else {
            cout << "No hidden information found.\n";
        }

        delete bPic;
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

void programTester() {
    int choice;
    do {
        system("cls");
        cout << "=== Welcome to ProgramTester ===\n";
        cout << "1. Auto all tests\n";
        cout << "2. Manual select tests\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';
        switch (choice) {
        case 0: return;
        default: break;
        }
    } while (choice != 0);
}

void programHelper() {
    int choice;
    do {
        system("cls");
        cout << "=== Welcome to ProgramHelper ===\n";
        cout << "1. About hide information\n";
        cout << "2. About view information\n";
        cout << "3. About detect secret in BMP\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';
        switch (choice) {
        case 0: return;
        default: break;
        }
    } while (choice != 0);
}

void exitProgram() {
    exit(0);
}