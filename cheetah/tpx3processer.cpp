#include "tpx3reader.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
    std::string file;
    int length;
    if (argc == 2) {
        file = argv[1];
        length = 1024;
    } 
    else if (argc == 3){
        file = argv[1];
        length = std::stoi(argv[2]);
    }
    
    TPX3 tp;
    tp.openFile_4d(file); 
    tp.process_4d(length); // dimension of the image
    tp.closeFile();
    return 0;
}