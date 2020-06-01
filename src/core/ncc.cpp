#include "driver.h"

#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = true;
    }

    for (;;) {
        Driver driver(std::cerr);

        if (driver.Parse(debug)) {
            std::cout << "Parse succeed!\n";
            driver.PrintSymbolTable(std::cout);
        }

        char peek = getc(stdin);
        if (feof(stdin))
            break;
        else
            ungetc(peek, stdin);
    }
}