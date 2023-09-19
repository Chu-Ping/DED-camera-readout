#include "tpx3reader.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[])
// int main()
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
        if (timestamp_arg == "full"){
            timestamp = true;
        } else {
            timestamp = false;
        }
    }

    // std::string file = "/mnt/B2C2DD54C2DD1E03/data/20230407/Measurement_apr_07_2023_11h03m15s/raw/test/fB3F_000000.tpx3";
    // int length = 2048;
    // bool timestamp = false;


    
    TPX3 tp(file, length, timestamp);
    return 0;
}