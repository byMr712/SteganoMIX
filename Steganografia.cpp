#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <vector>
#include <conio.h>
#include <stdlib.h>

void exitProgram();
void infoHider();
void infoViewer();
void infoDetector();
void programTester();
void programHelper();

using namespace std;
int main() {
    //setlocale(LC_ALL, "RUS");
    int choice;

    do {
        system("cls");

        cout << "=== SteganoMIX v1.01 ===";
        cout << "\nHide and view your information in a BMP file";
        cout << "\nYou seed for hide: ******\n"; //for future, not for release 

        cout << "\n=== Main menu ===\n";
        cout << "1. Hide information\n";
        cout << "2. View information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Tests this program\n";
        cout << "5. Help\n";
        cout << "0. Exit\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            infoHider();
            break;
        case 2:
            infoViewer();
            break;
        case 3:
            infoDetector();
            break;
        case 4:
            programTester();
            break;
        case 5:
            programHelper();
            break;
        case 6:
            //for future
            break;
        case 0:
            exitProgram();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
}

void infoHider() {
    int choice;

    do {
        system("cls");

        cout << "=== Welcome to InfoHider ===\n";
        cout << "1. Start\n";
        cout << "2. Options\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            //for future
            break;
        case 2:
            //for future
            break;
        case 3:
            //for future
            break;
        case 4:
            //for future
            break;
        case 5:
            //for future
            break;
        case 6:
            //for future
            break;
        case 0:
            main();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
}

void infoViewer() {
    int choice;

    do {
        system("cls");

        cout << "=== Welcome to InfoViewer ===\n";
        cout << "1. Start\n";
        cout << "2. Options\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            //for future
            break;
        case 2:
            //for future
            break;
        case 3:
            //for future
            break;
        case 4:
            //for future
            break;
        case 5:
            //for future
            break;
        case 6:
            //for future
            break;
        case 0:
            main();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
}

void infoDetector() {
    int choice;

    do {
        system("cls");

        cout << "=== Welcome to InfoDetector ===\n";
        cout << "1. Start\n";
        cout << "2. Options\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            //for future
            break;
        case 2:
            //for future
            break;
        case 3:
            //for future
            break;
        case 4:
            //for future
            break;
        case 5:
            //for future
            break;
        case 6:
            //for future
            break;
        case 0:
            main();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
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
        case 1:
            //for future
            break;
        case 2:
            //for future
            break;
        case 3:
            //for future
            break;
        case 4:
            //for future
            break;
        case 5:
            //for future
            break;
        case 6:
            //for future
            break;
        case 0:
            main();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
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
        case 1:
            //for future
            break;
        case 2:
            //for future
            break;
        case 3:
            //for future
            break;
        case 4:
            //for future
            break;
        case 5:
            //for future
            break;
        case 6:
            //for future
            break;
        case 0:
            main();
            break;
        default:
            //for future
            break;
        }
    } while (choice != 7);
}

void exitProgram() {
    exit(0);
}