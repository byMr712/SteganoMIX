#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <vector>
#include <conio.h>
#include <stdlib.h>

void exitProgram();

using namespace std;
int main() {
    //setlocale(LC_ALL, "RUS");
    int choice;
    int test = 0;

    do {
        system("cls");

        cout << "=== SteganoMIX v1.01 ===";
        cout << "\nA message from developer Mr712:";
        cout << "\nHide and view your information in a BMP file\n";

        cout << "\n=== Main menu ===\n";
        cout << "1. Hide information\n";
        cout << "2. View information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Tests this program\n";
        cout << "0. Exit\n\n";
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
            exitProgram();
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

