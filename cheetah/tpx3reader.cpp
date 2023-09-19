#include "tpx3reader.h"

void TPX3::openFile()
{
    size_t filesp_idx = file.find_last_of("/\\");
    std::string path = file.substr(0,filesp_idx+1);
    std::string file_name_4d = "event.dat";

    file_size = std::filesystem::file_size(file);
    file_raw.open(file, std::ios::in | std::ios::binary);
    file_event.open(path+file_name_4d, std::ios::out | std::ios::binary);
    if (!(file_raw.is_open() & file_event.is_open()))
    {
        std::cout << "TPX3::open_file(): Error opening raw file!" << std::endl;
    }
}

void TPX3::closeFile()
{
    if (file_raw.is_open())
    {
        file_raw.close();
    }
    if (file_event.is_open())
    {
        file_event.close();
    }
}


void TPX3::run(){
    openFile();
    while (pos < file_size){
        read();
        process();
    }
    closeFile();
}


void TPX3::read()
{
    file_raw.read((char*)(&buffer), sizeof(buffer));
    pos += sizeof(buffer);
}


void TPX3::process(){
    int type;
    dtype_4d data4;
    dtype_full dataf;

    for (int j = 0; j < PRELOC_SIZE; j++){
        type = which_type(&buffer[j]);
        if ((type == 2) & rise_fall[chip_id] & (line_count[chip_id] != 0)){
            if (timestamp){
                write(&buffer[j], dataf);
            }
            else{
                write(&buffer[j], data4);
            }
            
        }
    }
    if (timestamp) {
        std::cout << dataf.ry << std::endl;

    }
    else{
        std::cout << data4.ry << std::endl;
    }
}


int TPX3::which_type(uint64_t *packet)
{
    // if ( (*packet & ((1 << 32) - 1) == tpx_header)  ) {
    if ((*packet & 0xFFFFFFFF) == tpx_header)
    {
        chip_id = (*packet >> 32) & 0xff;
        return 0;
    } // header
    else if (*packet >> 60 == 0x6)
    {
        process_tdc(packet);
        return 1;
    } // TDC
    else if (*packet >> 60 == 0xb)
    {
        return 2;
    } // event
    else
    {
        return 3;
    } // unknown
}

void TPX3::process_tdc(uint64_t *packet)
{
    if (((*packet >> 56) & 0x0F) == 15)
    {
        rise_fall[chip_id] = true;
        rise_t[chip_id] = (((*packet >> 9) & 0x7FFFFFFFF) % 8589934592);
    }
    else if (((*packet >> 56) & 0x0F) == 10)
    {
        rise_fall[chip_id] = false;
        fall_t[chip_id] = ((*packet >> 9) & 0x7FFFFFFFF) % 8589934592;
        ++line_count[chip_id];
        if ((line_count[chip_id] <= line_count[0]) &
            (line_count[chip_id] <= line_count[1]) &
            (line_count[chip_id] <= line_count[2]) &
            (line_count[chip_id] <= line_count[3]))
        {
            total_line = line_count[chip_id];
        }
        line_interval = (fall_t[chip_id] - rise_t[chip_id]) * 2;
        dwell_time = line_interval / scan_x;
    }
}

void TPX3::write(uint64_t* packet, dtype_full &data)
{
    toa = (((*packet & 0xFFFF) << 14) + ((*packet >> 30) & 0x3FFF)) << 4;
    probe_position = ( toa - (rise_t[chip_id] * 2)) / dwell_time;

    if (probe_position < scan_x)
    {
        uint64_t pack_44 = (*packet >> 44);

        data.rx = probe_position;
        data.ry = line_count[chip_id];
        data.kx = (address_multiplier[chip_id] *
                (((pack_44 & 0x0FE00) >> 8) +
                ((pack_44 & 0x00007) >> 2)) +
            address_bias_x[chip_id]);
        data.ky = (address_multiplier[chip_id] *
                (((pack_44 & 0x001F8) >> 1) +
                (pack_44 & 0x00003)) +
            address_bias_y[chip_id]);
        data.toa = ((((*packet) & 0xFFFF) << 14) + (((*packet) >> (16 + 14)) & 0x3FFF) << 4) - (((*packet) >> 16) & 0xF);
        data.tot = ((*packet) >> (16 + 4)) & 0x3ff;
        file_event.write((const char*)&data, sizeof(data));
    }
}

void TPX3::write(uint64_t* packet, dtype_4d &data)
{
    toa = (((*packet & 0xFFFF) << 14) + ((*packet >> 30) & 0x3FFF)) << 4;
    probe_position = ( toa - (rise_t[chip_id] * 2)) / dwell_time;

    if (probe_position < scan_x)
    {
        uint64_t pack_44 = (*packet >> 44);

        data.rx = probe_position;
        data.ry = line_count[chip_id];
        data.kx = (address_multiplier[chip_id] *
                (((pack_44 & 0x0FE00) >> 8) +
                ((pack_44 & 0x00007) >> 2)) +
            address_bias_x[chip_id]);
        data.ky = (address_multiplier[chip_id] *
                (((pack_44 & 0x001F8) >> 1) +
                (pack_44 & 0x00003)) +
            address_bias_y[chip_id]);
        file_event.write((const char*)&data, sizeof(data));
    }
}


