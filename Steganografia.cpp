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
void exitProgram();
void infoHider();
void infoViewer();
void infoDetector();
void programTester();
void programHelper();

// ============ БИБЛИОТЕКИ ============
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
        cout << "=== SteganoMIX v1.01 ===";
        cout << "\nHide and view your information in a BMP file\n";
        cout << "\n=== Main menu ===\n";
        cout << "1. Hide information\n";
        cout << "2. View information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Tests this program\n";
        cout << "5. Help\n";
        cout << "0. Exit\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1: infoHider(); break;
        case 2: infoViewer(); break;
        case 3: infoDetector(); break;
        case 4: programTester(); break;
        case 5: programHelper(); break;
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
std::bitset<8> ByteToBit(unsigned char src) {
    return std::bitset<8>(src);
}

// Преобразование битов в байт (ОДНА ВЕРСИЯ!)
unsigned char BitToByte(const std::bitset<8>& scr) {
    return static_cast<unsigned char>(scr.to_ulong());
}

// Встраивание символа в цвет пикселя
PixelColor EmbedSymbolToColor(const PixelColor& curColor, unsigned char symbol) {
    std::bitset<8> symbolBits = ByteToBit(symbol);
    PixelColor result = curColor;

    // R: младшие 2 бита
    std::bitset<8> tempR = ByteToBit(curColor.R);
    tempR[0] = symbolBits[0];
    tempR[1] = symbolBits[1];
    result.R = BitToByte(tempR);

    // G: младшие 3 бита
    std::bitset<8> tempG = ByteToBit(curColor.G);
    tempG[0] = symbolBits[2];
    tempG[1] = symbolBits[3];
    tempG[2] = symbolBits[4];
    result.G = BitToByte(tempG);

    // B: младшие 3 бита
    std::bitset<8> tempB = ByteToBit(curColor.B);
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

// ============ ЗАПИСЬ ТЕКСТА В BMP (там же проверки и тд, мне так лень разбивать на отдельные функции снова) ============
void HideTextInBMP(Bitmap* bPic, const std::string& text) {
    // 1. Получаем байты текста
    std::vector<unsigned char> bList;
    for (size_t i = 0; i < text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    // 2. Проверки
    if (CountText > (width * height) - 4) {
        MessageBox(NULL, L"Выбранная картинка мала для размещения выбранного текста", L"Информация", MB_OK);
        return;
    }

    if (isEncryptionInBMP(bPic)) {
        MessageBox(NULL, L"Файл уже зашифрован", L"Информация", MB_OK);
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

// ============ ЧТЕНИЕ ТЕКСТА ИЗ BMP (самая простая) ============

std::string ReadTextFromBMP(Bitmap* bPic) {
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
    std::vector<unsigned char> message;
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
    return std::string(message.begin(), message.end());
}

// ============ СОХРАНЕНИЕ В BMP ============
const CLSID CLSID_BMP = { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } }; // Константа - CLSID для BMP формата
bool SaveBMPAs(Bitmap* bPic, const char* originalPath, const char* suffix) {
    char newPath[MAX_PATH];
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    // Разбираем путь
    _splitpath_s(originalPath, drive, _MAX_DRIVE, dir, _MAX_DIR,
        fname, _MAX_FNAME, ext, _MAX_EXT);

    // Собираем новый путь с суффиксом
    sprintf_s(newPath, sizeof(newPath), "%s%s%s%s%s", drive, dir, fname, suffix, ext);

    WCHAR wnewPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, newPath, -1, wnewPath, MAX_PATH);

    if (bPic->Save(wnewPath, &CLSID_BMP, NULL) == Ok) {
        cout << "Saved as: " << newPath << endl;
        return true;
    }
    return false;
}

void infoHider() {
    system("cls");
    cout << "=== Welcome to InfoHider ===\n";
    cout << "1. LSB (Least Significant Bit)\n";
    cout << "2. Algorithm 2\n";
    cout << "3. Algorithm 3\n";
    cout << "0. Return to menu\n\n";

    int alg = _getch() - '0';

    if (alg == 0) {
        return;  // Возврат в главное меню
    }

    if (alg == 1) {
        system("cls");
        cout << "=== LSB Algorithm ===\n\n";

        // 1. Выбираем файл
        char path[MAX_PATH];
        cout << "Select a BMP file to hide information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected. Operation cancelled.\n";
            system("pause");
            return;  // Возврат в главное меню
        }

        // 2. Загружаем BMP
        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;  // Возврат в главное меню
        }

        cout << "File successfully loaded: " << path << endl;
        cout << "Size: " << bPic->GetWidth() << "x" << bPic->GetHeight() << endl;

        // 3. Вводим текст
        cout << "\nInput your text: ";
        string text;
        getline(cin, text);

        if (text.empty()) {
            cout << "Text cannot be empty!\n";
            delete bPic;
            system("pause");
            return;  // Возврат в главное меню
        }

        cout << "Text length: " << text.length() << " symbols\n";

        // 4. Скрываем текст
        cout << "\nHiding information...\n";
        HideTextInBMP(bPic, text);

        // 5. Сохраняем
        cout << "Saving file...\n";

        if (SaveBMPAs(bPic, path, "_hidden")) {
            cout << "\nInformation successfully hidden and saved!\n";
        }
        else {
            cout << "\nError saving file!\n";
        }

        delete bPic;
        system("pause");
        return;  // Возврат в главное меню
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
        return;  // Возврат в главное меню
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to read hidden information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;  // Возврат в главное меню
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;  // Возврат в главное меню
        }

        string hiddenText = ReadTextFromBMP(bPic);

        if (!hiddenText.empty()) {
            cout << "\n=== Hidden text ===\n";
            cout << hiddenText << endl;
            cout << "==================\n";
            cout << "Text length: " << hiddenText.length() << " symbols\n";
        }

        delete bPic;
        system("pause");
        return;  // Возврат в главное меню
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
        return;  // Возврат в главное меню
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to check...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;  // Возврат в главное меню
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;  // Возврат в главное меню
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
        return;  // Возврат в главное меню
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