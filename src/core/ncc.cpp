#include "driver.h"

#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
    bool        debug = false;
    bool        table = false, fullTable = false;
    bool        optimize = false;
    bool        ir       = false;
    bool        assembly = false, simpleMips = false;
    std::string asmFilename, simpleMipsFilename;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = true;
        else if (strcmp(argv[i], "-t") == 0)
            table = true;
        else if (strcmp(argv[i], "-ft") == 0)
            fullTable = table = true;
        else if (strcmp(argv[i], "-o") == 0)
            optimize = true;
        else if (strcmp(argv[i], "-ir") == 0)
            ir = true;
        else if (strcmp(argv[i], "-s") == 0) {
            assembly = true;
            if (i + 1 < argc)
                asmFilename = argv[++i];
            else {
                std::cerr << "Require output filename!\n";
                return 1;
            }
        }
        else if (strcmp(argv[i], "-ss") == 0) {
            simpleMips = true;
            if (i + 1 < argc)
                simpleMipsFilename = argv[++i];
            else {
                std::cerr << "Require output filename!\n";
                return 1;
            }
        }
    }

    for (;;) {
        Driver driver(std::cerr);

        if (driver.Parse(debug, fullTable)) {
            if (table)
                std::cout << driver.PrintSymbolTable();

            if (optimize)
                driver.Optimize();

            if (ir)
                std::cout << driver.PrintIR();

            if (assembly)
                driver.EmitAssemblyCode(asmFilename);

            if (simpleMips)
                driver.EmitSimpleMipsCode(simpleMipsFilename);
        }

        char peek = getc(stdin);
        if (feof(stdin))
            break;
        else
            ungetc(peek, stdin);
    }
}