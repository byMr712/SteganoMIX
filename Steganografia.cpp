#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <bitset>
#include <vector>
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

// ============ ОСНОВНАЯ ПРОГРАММА ============
int main() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int choice;
    do {
        system("cls");
        cout << "=== SteganoMIX v1.02 ===";
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

// ============ ОСНОВНЫЕ ФУНКЦИИ СТЕГАНОГРАФИИ ============
// Структура цвета
struct PixelColor {
    uint8_t R, G, B;
    PixelColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : R(r), G(g), B(b) {}
};

// Преобразование байта в биты (ОДНА ВЕРСИЯ!)
    bitset<8> ByteToBit(unsigned char src) {
    return bitset<8>(src);
}

// Преобразование битов в байт (ОДНА ВЕРСИЯ!)
unsigned char BitToByte(const bitset<8>& scr) {
    return static_cast<unsigned char>(scr.to_ulong());
}

// Встраивание символа в цвет пикселя
PixelColor EmbedSymbolToColor(const PixelColor& curColor, unsigned char symbol) {
    bitset<8> symbolBits = ByteToBit(symbol);
    PixelColor result = curColor;

    // R: младшие 2 бита
    bitset<8> tempR = ByteToBit(curColor.R);
    tempR[0] = symbolBits[0];
    tempR[1] = symbolBits[1];
    result.R = BitToByte(tempR);

    // G: младшие 3 бита
    bitset<8> tempG = ByteToBit(curColor.G);
    tempG[0] = symbolBits[2];
    tempG[1] = symbolBits[3];
    tempG[2] = symbolBits[4];
    result.G = BitToByte(tempG);

    // B: младшие 3 бита
    bitset<8> tempB = ByteToBit(curColor.B);
    tempB[0] = symbolBits[5];
    tempB[1] = symbolBits[6];
    tempB[2] = symbolBits[7];
    result.B = BitToByte(tempB);

    return result;
}

// Проверка признака шифрования
bool isEncryption(const PixelColor& pixelColor) {
    unsigned char extracted = 0;
    extracted |= (pixelColor.R & 0x03);        // биты 0-1
    extracted |= ((pixelColor.G & 0x07) << 2); // биты 2-4
    extracted |= ((pixelColor.B & 0x07) << 5); // биты 5-7
    return (extracted == '/');
}

// ============ РАБОТА С BMP ============
// Получить PixelColor из BMP
PixelColor GetPixelColor(Bitmap* bPic, int x, int y) {
    Color color;
    bPic->GetPixel(x, y, &color);
    return PixelColor(
        static_cast<uint8_t>(color.GetR()),
        static_cast<uint8_t>(color.GetG()),
        static_cast<uint8_t>(color.GetB())
    );
}

// Установить PixelColor в BMP
void SetPixelColor(Bitmap* bPic, int x, int y, const PixelColor& color) {
    Color gdiColor(color.R, color.G, color.B);
    bPic->SetPixel(x, y, gdiColor);
}

// Встраивание признака в BMP
void EmbedTextIntoBMP(Bitmap* bPic) {
    PixelColor curColor = GetPixelColor(bPic, 0, 0);
    PixelColor newColor = EmbedSymbolToColor(curColor, '/');
    SetPixelColor(bPic, 0, 0, newColor);
}

// Проверка признака в BMP
bool isEncryptionInBMP(Bitmap* bPic) {
    PixelColor color = GetPixelColor(bPic, 0, 0);
    return isEncryption(color);
}

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

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct ProgramSettings {
    //Общие настройки
    bool useEncryption = true;
    unsigned int currentSeed = 0;

    // Суффиксы (упрощённые)
    bool useSuffix = true;      // Включает/выключает добавление суффикса вообще
    bool useSeedSuffix = true;   // Добавлять ли seed в имя файла (только при шифровании)
};



// ============ НАСТРОЙКИ ============vs
ProgramSettings g_settings;
void showSettings() {
    system("cls");
    cout << "=== Settings ===\n";
    cout << "1. Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << "\n";
    cout << "2. Add suffix to filename: " << (g_settings.useSuffix ? "ON" : "OFF") << "\n";
    cout << "3. Add seed to filename: " << (g_settings.useSeedSuffix ? "ON" : "OFF") << "\n";
    cout << "0. Return\n\n";
    cout << "Note: \n";
    cout << "  - If Encryption ON: + suffix '_encrypted'\n";
    cout << "  - If Encryption OFF: suffix '_hidden'\n";
    cout << "  - If Add suffix to filename ON: enable all suffix\n";
    cout << "  - If Add suffix to filename OFF: disable all suffix\n";
    cout << "  - If Add seed to filename ON: enable suffix '_you_seed'\n";
    cout << "  - If Add seed to filename OFF: disable suffix '_you_seed'\n";
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
        case 0:
            return;
        default:
            break;
        }
    } while (choice != 0);
}

// ============ ИЗВЛЕЧЕНИЕ СИМВОЛА ИЗ ЦВЕТА ============
unsigned char ExtractSymbolFromColor(const PixelColor& color) {
    unsigned char extracted = 0;
    extracted |= (color.R & 0x03);        // биты 0-1 (из R)
    extracted |= ((color.G & 0x07) << 2); // биты 2-4 (из G)
    extracted |= ((color.B & 0x07) << 5); // биты 5-7 (из B)
    return extracted;
}

// ============ ЗАПИСЬ РАЗМЕРА ТЕКСТ ============
void WriteCountText(int count, Bitmap* src) {
    char countStr[4] = { 0 };
    sprintf_s(countStr, sizeof(countStr), "%03d", count);  // 3 цифры с ведущими нулями

    for (int i = 0; i < 3; i++) {
        unsigned char symbol = static_cast<unsigned char>(countStr[i]);
        PixelColor curColor = GetPixelColor(src, 0, i + 1);
        PixelColor newColor = EmbedSymbolToColor(curColor, symbol);
        SetPixelColor(src, 0, i + 1, newColor);
    }
}

// ============ ЧТЕНИЕ РАЗМЕРА ТЕКСТА ============
int ReadCountText(Bitmap* src) {
    char countStr[4] = { 0 };

    for (int i = 0; i < 3; i++) {
        PixelColor color = GetPixelColor(src, 0, i + 1);
        unsigned char ch = ExtractSymbolFromColor(color);
        // Проверяем, что это цифра
        if (ch >= '0' && ch <= '9') {
            countStr[i] = static_cast<char>(ch);
        }
        else {
            countStr[i] = '0';  // Если не цифра - заменяем на '0'
        }
    }

    return atoi(countStr);
}

// ============ ГЕНЕРАЦИЯ SEED (для шифрования) ============
unsigned int GenerateSeed(int width, int height) {
    // 1. Базовый seed из ширины и высоты самого BMP
    unsigned int seed = (width * 31) ^ (height * 17);

    // 2. Добавляем случайность из времени
    seed ^= static_cast<unsigned int>(time(nullptr));

    // 3. Добавляем случайность из системного таймера
    seed ^= static_cast<unsigned int>(GetTickCount64());

    // 4. Перемешиваем биты
    seed = (seed << 13) ^ seed;
    seed = (seed >> 7) ^ seed;
    seed = (seed << 17) ^ seed;

    return seed;
}

// ============ ГЕНЕРАЦИЯ ПСЕВДОСЛУЧАЙНЫХ ПОЗИЦИЙ (для шифрования) ============
struct PixelPosition {
    int x, y;
};

// Генерирует список случайных позиций пикселей
vector<PixelPosition> GenerateRandomPositions(int width, int height, int count, unsigned int seed) {
    vector<PixelPosition> positions;
    positions.reserve(count);

    // Инициализируем генератор с нашим модным seed
    srand(seed);

    // Создаём список ВСЕХ возможных позиций (начиная с 4)
    vector<PixelPosition> allPositions;
    for (int i = 4; i < width; i++) {
        for (int j = 0; j < height; j++) {
            allPositions.push_back({ i, j });
        }
    }

    // Перемешиваем список (алгоритм Фишера-Йетса)
    for (int i = static_cast<int>(allPositions.size()) - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(allPositions[i], allPositions[j]);
    }

    // Берём первые count позиций
    for (int i = 0; i < count && i < static_cast<int>(allPositions.size()); i++) {
        positions.push_back(allPositions[i]);
    }

    return positions;
}

// ============ ЗАПИСЬ ТЕКСТА В BMP (там же проверки и тд, мне так лень разбивать на отдельные функции снова) ============
void HideTextInBMP(Bitmap* bPic, const string& text) {
    // 1. Получаем байты текста
    vector<unsigned char> bList;
    for (size_t i = 0; i < text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    // 2. Проверки
    if (CountText > (width * height) - 4) {
        MessageBox(NULL, L"No free space in the image", L"Information", MB_OK);
        return;
    }

    if (isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    // 3. Записываем признак в пиксель (0,0)
    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColor(pixel00, '/'));

    // 4. Записываем размер текста в пиксели (0,1)-(0,3)
    WriteCountText(CountText, bPic);

    // 5. Записываем текст, начиная с пикселя (4,0)
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

// ============ ЗАПИСЬ ТЕКСТА В BMP (С ШИФРОВАНИЕМ) ============
void HideTextInBMP_Encrypted(Bitmap* bPic, const string& text, unsigned int seed) {
    // 1. Получаем байты текста
    vector<unsigned char> bList;
    for (size_t i = 0; i < text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    // 2. Проверки
    if (CountText > (width * height) - 4) {
        MessageBox(NULL, L"No free space in the image", L"Information", MB_OK);
        return;
    }

    if (isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    // 3. Записываем признак в пиксель (0,0)
    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColor(pixel00, '/'));

    // 4. Записываем размер текста в пиксели (0,1)-(0,3)
    WriteCountText(CountText, bPic);

    // 5. Генерируем случайные позиции
    vector<PixelPosition> positions = GenerateRandomPositions(width, height, CountText, seed);

    // 6. Записываем текст в случайные пиксели
    for (int i = 0; i < CountText && i < static_cast<int>(positions.size()); i++) {
        int x = positions[i].x;
        int y = positions[i].y;

        PixelColor pixelColor = GetPixelColor(bPic, x, y);
        PixelColor newColor = EmbedSymbolToColor(pixelColor, bList[i]);
        SetPixelColor(bPic, x, y, newColor);
    }
}

// ============ ЧТЕНИЕ ТЕКСТА ИЗ BMP (самая простая) ============
    string ReadTextFromBMP(Bitmap* bPic) {
    // 1. Проверяем наличие признака
    if (!isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"В файле нет зашифрованной информации", L"Информация", MB_OK);
        return "";
    }

    // 2. Читаем количество символов
    int countSymbol = ReadCountText(bPic);
    if (countSymbol <= 0) {
        return "";
    }

    // 3. Читаем текст
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

    // 4. Преобразуем в строку
    return string(message.begin(), message.end());
}

// ============ СОХРАНЕНИЕ BMP ============
const CLSID CLSID_BMP = { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } };

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

bool SaveBMPWith(Bitmap* bPic, const char* originalPath, bool encrypted, unsigned int seed) {
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    _splitpath_s(originalPath, drive, _MAX_DRIVE, dir, _MAX_DIR,
        fname, _MAX_FNAME, ext, _MAX_EXT);

    // Создаём папку output_img
    char outputDir[MAX_PATH];
    sprintf_s(outputDir, sizeof(outputDir), "%s%soutput_img\\", drive, dir);
    CreateDirectoryIfNotExists(outputDir);

    // Формируем имя файла
    char fullName[MAX_PATH];

    if (g_settings.useSuffix) {
        // Если суффиксы включены
        if (encrypted) {
            // Зашифрованный файл: _encrypted + _seed (если включено)
            sprintf_s(fullName, sizeof(fullName), "%s%s_encrypted", outputDir, fname);
            if (g_settings.useSeedSuffix) {
                char seedStr[16];
                sprintf_s(seedStr, sizeof(seedStr), "_%u", seed);
                strcat_s(fullName, seedStr);
            }
        }
        else {
            // Обычный файл: _hidden
            sprintf_s(fullName, sizeof(fullName), "%s%s_hidden", outputDir, fname);
        }
    }
    else {
        // Без суффиксов
        sprintf_s(fullName, sizeof(fullName), "%s%s", outputDir, fname);
    }

    // Добавляем расширение
    strcat_s(fullName, ext);

    WCHAR wnewPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, fullName, -1, wnewPath, MAX_PATH);

    if (bPic->Save(wnewPath, &CLSID_BMP, NULL) != Ok) {
        return false;
    }

    cout << "Saved as: " << fullName << endl;
    return true;
}

// ============ ЧТЕНИЕ С ШИФРОВАНИЕМ ============
string ReadTextFromBMP_Encrypted(Bitmap* bPic, unsigned int seed) {
    if (!isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"Нет зашифрованной информации", L"Информация", MB_OK);
        return "";
    }

    int countSymbol = ReadCountText(bPic);
    if (countSymbol <= 0) {
        return "";
    }

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    vector<PixelPosition> positions = GenerateRandomPositions(width, height, countSymbol, seed);

    vector<unsigned char> message;
    message.reserve(countSymbol);

    for (int i = 0; i < countSymbol && i < static_cast<int>(positions.size()); i++) {
        PixelColor pixelColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
        message.push_back(ExtractSymbolFromColor(pixelColor));
    }

    return string(message.begin(), message.end());
}

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

        // ============ ПРОВЕРКА: ФАЙЛ УЖЕ ЗАШИФРОВАН ИЛИ СПРЯТАН ============
        if (isEncryptionInBMP(bPic)) {
            cout << "\nThis file already contains hidden information!\n";
            cout << "Use 'View information' to read the hidden text.\n";
            delete bPic;
            system("pause");
            return;
        }

        int width = bPic->GetWidth();
        int height = bPic->GetHeight();

        cout << "File successfully loaded: " << path << endl;
        cout << "Size: " << width << "x" << height << endl;
        cout << "Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << endl;

        cout << "\nInput your text: ";
        string text;
        getline(cin, text);

        if (text.empty()) {
            cout << "Text cannot be empty!\n";
            delete bPic;
            system("pause");
            return;
        }

        cout << "\nText length: " << text.length() << " symbols\n";

        bool encrypted = g_settings.useEncryption;
        unsigned int seed = 0;

        if (encrypted) {
            seed = GenerateSeed(width, height);
            g_settings.currentSeed = seed;
            cout << "Seed generated: " << seed << endl;
            cout << "SAVE THIS SEED to decrypt later!\n";
            HideTextInBMP_Encrypted(bPic, text, seed);
        }
        else {
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

        // ============ ЗАПРАШИВАЕМ SEED ============
        cout << "\nEnter seed (or press Enter for unencrypted text): ";
        string seedInput;
        getline(cin, seedInput);

        string hiddenText;

        if (!seedInput.empty()) {
            // С шифрованием
            unsigned int seed = static_cast<unsigned int>(stoul(seedInput));
            cout << "Using seed: " << seed << " (encrypted mode)\n";
            hiddenText = ReadTextFromBMP_Encrypted(bPic, seed);
        }
        else {
            // Без шифрования
            cout << "Unencrypted mode\n";
            hiddenText = ReadTextFromBMP(bPic);
        }

        // ============ ВЫВОД РЕЗУЛЬТАТА ============
        if (!hiddenText.empty()) {
            cout << "\n=== Hidden text ===\n";
            cout << hiddenText << endl;
            cout << "==================\n";
            cout << "Text length: " << hiddenText.length() << " symbols";
            cout << "\n==================\n";
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

        if (isEncryptionInBMP(bPic)) {
            cout << "Hidden information detected!\n";
            int count = ReadCountText(bPic);
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