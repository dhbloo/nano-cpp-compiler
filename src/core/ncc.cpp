#include "driver.h"

#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
    bool debug = false;
    bool table = false, fullTable = false;
    bool optimize = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = true;
        else if (strcmp(argv[i], "-t") == 0)
            table = true;
        else if (strcmp(argv[i], "-ft") == 0)
            fullTable = table = true;
        else if (strcmp(argv[i], "-o") == 0)
            optimize = true;
    }

    for (;;) {
        Driver driver(std::cerr);

        if (driver.Parse(debug, fullTable)) {
            if (table)
                driver.PrintSymbolTable(std::cout);

            if (optimize)
                driver.Optimize();

            driver.PrintIR();
        }

        char peek = getc(stdin);
        if (feof(stdin))
            break;
        else
            ungetc(peek, stdin);
    }
}