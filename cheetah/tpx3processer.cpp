#include "tpx3reader.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
    std::string file;
    int length;
    bool timestamp;
    if (argc == 2) {
        file = argv[1];
        length = 1024;
        timestamp = false;
    } 
    else if (argc == 3){
        file = argv[1];
        length = std::stoi(argv[2]);
        timestamp = false;
    }
    else if (argc == 4){
        file = argv[1];
        length = std::stoi(argv[2]);
        std::string timestamp_arg = argv[3];
        if (timestamp_arg == "true"){
            timestamp = true;
        } else {
            timestamp = false;
        }
    }
    
    TPX3 tp;
    tp.openFile(file); 
    tp.process(length, timestamp); 
    tp.closeFile();
    return 0;
}