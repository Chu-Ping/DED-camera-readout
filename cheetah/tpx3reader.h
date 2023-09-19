/* Copyright (C) 2021 Thomas Friedrich, Chu-Ping Yu, 
 * University of Antwerp - All Rights Reserved. 
 * You may use, distribute and modify
 * this code under the terms of the GPL3 license.
 * You should have received a copy of the GPL3 license with
 * this file. If not, please visit: 
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * 
 * Authors: 
 *   Thomas Friedrich <thomas.friedrich@uantwerpen.be>
 *   Chu-Ping Yu <chu-ping.yu@uantwerpen.be>
 */


#ifndef TPX3READER_H
#define TPX3READER_h
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <array>
#include <vector>


const size_t PRELOC_SIZE = 4000*25;
struct dtype_4d
    {
        uint16_t kx;
        uint16_t ky;
        uint16_t rx;
        uint16_t ry;
    };
struct dtype_full
    {
        uint16_t kx;
        uint16_t ky;
        uint16_t rx;
        uint16_t ry;
        uint64_t toa;
        uint64_t tot;
    };



class TPX3
{
private:
    // process buffer
    bool proc_started = false;
    int preloc_size = PRELOC_SIZE;
    std::array<uint64_t, PRELOC_SIZE>buffer;

    // Data
    uint64_t packet;
    uint64_t probe_position;
    uint64_t toa;
    uint64_t rise_t[4];
    uint64_t fall_t[4];
    bool rise_fall[4] = {false, false, false, false};
    int line_count[4] = {0, 0, 0, 0};
    int total_line = 0;
    uint64_t line_interval;
    uint64_t dwell_time;
    bool started = false;

    // Chip
    int chip_id;
    uint64_t tpx_header = 861425748; //(b'TPX3', 'little')
    int address_multiplier[4] = {1,-1,-1,1};
    int address_bias_x[4] = {256, 511, 255, 0};
    int address_bias_y[4] = {0, 511, 511, 0};

    // File
    std::ifstream file_raw;
    std::ofstream file_event;
    std::uintmax_t file_size;
    std::uintmax_t pos=0;

    // Method
    void openFile();
    void closeFile();
    void read();
    void process();
    int which_type(uint64_t *packet);
    void process_tdc(uint64_t *packet);
    void write(uint64_t *packet, dtype_full &data);
    void write(uint64_t *packet, dtype_4d &data);
    void run();


public:
    // File
    std::string file;
    uint16_t scan_x;
    uint16_t scan_y;
    bool timestamp;


    TPX3(std::string file, uint16_t scan_x, bool timestamp){
        this->file = file;
        this->scan_x = scan_x;
        this->scan_y = scan_x;
        this->timestamp = timestamp;
        this->run();
    }


};
#endif