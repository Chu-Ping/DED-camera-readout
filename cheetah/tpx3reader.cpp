#include "tpx3reader.h"

void TPX3::openFile_event(std::string file, std::string path)
{
    std::string file_name_tdc = "tdc.dat";
    std::string file_name_event = "event.dat";

    file_size = std::filesystem::file_size(file);
    file_raw.open(file, std::ios::in | std::ios::binary);
    file_tdc.open(path+file_name_tdc, std::ios::out | std::ios::binary);
    file_event.open(path+file_name_event, std::ios::out | std::ios::binary);
    if (file_raw.is_open() & file_tdc.is_open() & file_event.is_open())
    {
        resetFile();
    }
    else
    {
        std::cout << "TPX3::open_file(): Error opening raw file!" << std::endl;
    }
}

void TPX3::openFile_4d(std::string file)
{
    size_t filesp_idx = file.find_last_of("/\\");
    std::string path = file.substr(0,filesp_idx);
    

    std::string file_name_4d = "fourd.dat";

    file_size = std::filesystem::file_size(file);
    file_raw.open(file, std::ios::in | std::ios::binary);
    file_4d.open(path+file_name_4d, std::ios::out | std::ios::binary);
    if (file_raw.is_open() & file_4d.is_open())
    {
        resetFile();
    }
    else
    {
        std::cout << "TPX3::open_file(): Error opening raw file!" << std::endl;
    }
}

void TPX3::openFile_csr(std::string file, std::string path)
{
    std::string file_name_tdc = "tdc.dat";
    std::string file_name_event = "event.dat";

    file_size = std::filesystem::file_size(file);
    file_raw.open(file, std::ios::in | std::ios::binary);
    file_val.open(path+file_name_tdc, std::ios::out | std::ios::binary);
    file_col.open(path+file_name_event, std::ios::out | std::ios::binary);
    file_row.open(path+file_name_event, std::ios::out | std::ios::binary);
    if (file_raw.is_open() & file_val.is_open() & file_col.is_open() & file_row.is_open())
    {
        resetFile();
    }
    else
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
    if (file_tdc.is_open())
    {
        file_tdc.close();
    }
    if (file_event.is_open())
    {
        file_event.close();
    }
    if (file_4d.is_open())
    {
        file_4d.close();
    }
    if (file_val.is_open())
    {
        file_val.close();
    }
    if (file_col.is_open())
    {
        file_col.close();
    }
    if (file_row.is_open())
    {
        file_row.close();
    }
}

void TPX3::process_event()
{
    uint8_t mode;
    while (pos < file_size)
    {
        read((char *) &header, header_size);
        if (header.three != 51)
        {
            printf("header error\n");
        }
        for(int i=0; i < header.chunksize; i+=packet_size)
        {
            read((char *) &packet, packet_size);
            mode = isTdc(); // 0: tdc, 1: packet, 2: unknown

            switch(mode)
            {
                case 0:
                    writeTdc();
                    break;
                case 1:
                    writeEvent();
                    break;
                case 2:
                    printf("unexpected data format\n");
                    break;
            }
        }
        std::cout << pos << " / " << file_size << std::endl;
    }
}


void TPX3::process_4d(uint16_t pix_per_line)
{
    uint8_t mode;
    bool end_of_line;
    dtype_event *event_ptr = &event_line[0][0];
    while (pos < file_size)
    {
        read((char *) &header, header_size);
        if (header.three != 51)
        {
            printf("header error\n");
        }
        for(int i=0; i < header.chunksize; i+=packet_size)
        {
            read((char *) &packet, packet_size);
            mode = isTdc(); // 0: tdc, 1: packet, 2: unknown

            if (mode==0)
            {
                end_of_line = updateLine();
                if (end_of_line)
                {
                    event_ptr = &event_line[header.chipindex][0];
                    write4d(event_ptr, pix_per_line);
                }
            }
            else if ((mode==1) & risefall[header.chipindex])
            {
                event_ptr = &event_line[header.chipindex][event_count[header.chipindex]];
                updateEvent(event_ptr);
            }

        }
        // std::cout << pos << " / " << file_size << std::endl;
        // std::cout << fourd.ry << std::endl;
    }
}


void TPX3::process_csr(uint16_t pix_per_line)
{
    uint8_t mode;
    bool end_of_line;
    std::vector<uint32_t> val_line(pix_per_line);
    std::vector<uint32_t> col_line(pix_per_line);
    

    dtype_event *event_ptr = &event_line[0][0];
    while (pos < file_size)
    {
        read((char *) &header, header_size);
        if (header.three != 51)
        {
            printf("header error\n");
        }
        for(int i=0; i < header.chunksize; i+=packet_size)
        {
            read((char *) &packet, packet_size);
            mode = isTdc(); // 0: tdc, 1: packet, 2: unknown

            if (mode==0)
            {
                end_of_line = updateLine();
                if (end_of_line)
                {
                    event_ptr = &event_line[header.chipindex][0];
                    write4d(event_ptr, pix_per_line);
                }
            }
            else if ((mode==1) & risefall[header.chipindex])
            {
                event_ptr = &event_line[header.chipindex][event_count[header.chipindex]];
                updateEvent(event_ptr);
            }

        }
        std::cout << pos << " / " << file_size << std::endl;
    }
}

void TPX3::read(char *buffer, size_t buffer_size)
{
    file_raw.read(buffer, buffer_size);
    pos += buffer_size;
}

uint8_t TPX3::isTdc()
{
    // std::cout << (packet >> 60) << std::endl; 
    if ( packet >> 60 == 0x6 )
    {
        return 0;
    }
    else if ( packet >> 60 == 0xb )
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

bool TPX3::updateLine()
{
    if (((packet >> 56) & 0x0F) == 15)
    {
        if (!risefall[header.chipindex])
        {
        risefall[header.chipindex] = true;
        risetime[header.chipindex] = ((packet >> 9)& 0x7FFFFFFFF) % 8589934592;
        if ( (risetime[header.chipindex]+100) < risetime[header.chipindex-1] ){
            printf("error");
        }
        event_count[header.chipindex] = 0;
        ++line_count[header.chipindex];
        } 
        else { 
            printf("repeated rise\n"); 
        }
        return false;
    }
    else if (((packet >> 56) & 0x0F) == 10)
    {
        if (risefall[header.chipindex])
        {
        risefall[header.chipindex] = false;
        falltime[header.chipindex] = ((packet >> 9)& 0x7FFFFFFFF) % 8589934592;
        } 
        else { printf("repeated fall\n"); }
        return true;
    }
    else { 
        printf("unused trigger detected\n"); return false;
    }

}


void TPX3::writeTdc()
{
    tdc.trigger &= 0x00;
    tdc.trigger |= (packet >> 56) & 0x0F;
    tdc.toa = (packet >> 9)& 0x7FFFFFFFF;
    tdc.chip = header.chipindex;
    file_tdc.write((const char *)&tdc, tdc_size);
}

void TPX3::writeEvent()
{
    pixaddr = packet >> 44;

    // dc = pixaddr & 0x0FE00 >> 8
    // sp = pixaddr & 0x001F8 >> 1
    // pi = pixaddr & 0x00007
    // time = (((d & 0xffff) << 14) + ((d >> (16 + 14)) & 0x3fff) << 4) - ((d >> 16) & 0xf)     # 1.5625ns units

    event.kx = ((pixaddr & 0x0FE00) >> 8) + ((pixaddr & 0x00007) >> 2);
    event.ky = ((pixaddr & 0x001F8) >> 1) + (pixaddr & 0x00003);
    event.toa = (((packet & 0xFFFF) << 14) + ((packet >> (16 + 14)) & 0x3FFF) << 4) - ((packet >> 16) & 0xF);
    event.chip = header.chipindex;
    file_event.write((const char*)&event, event_size);
}

void TPX3::write4d(dtype_event* event_ptr, uint16_t pix_per_line)
{
    switch (header.chipindex)
    {
    case 0:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2)) 
            {
                fourd.kx = 256+(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 1:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = 511-(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 2:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = 255-(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 3:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = (uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    }
    std::cout << fourd.ry << std::endl;
}

void TPX3::writeCsr(dtype_event* event_ptr, uint16_t pix_per_line)
{
    switch (header.chipindex)
    {
    case 0:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = 256+(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 1:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = 511-(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 2:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = 255-(uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    case 3:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fourd.kx = (uint16_t)(*(event_ptr+i)).kx;
                fourd.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fourd.rx);
                fourd.ry = line_count[header.chipindex];
                file_4d.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    }
}
void TPX3::updateEvent(dtype_event* event_ptr)
{
    pixaddr = packet >> 44;
    (*event_ptr).kx = ((pixaddr & 0x0FE00) >> 8) + ((pixaddr & 0x00007) >> 2);
    (*event_ptr).ky = ((pixaddr & 0x001F8) >> 1) + (pixaddr & 0x00003);
    (*event_ptr).toa = (((packet & 0xFFFF) << 14) + ((packet >> (16 + 14)) & 0x3FFF) << 4) - ((packet >> 16) & 0xF);
    (*event_ptr).chip = header.chipindex;
    ++event_count[header.chipindex];
    // std::cout << event_count[header.chipindex] << std::endl;
}

void TPX3::toa_2_pos(uint64_t toa, uint16_t pix_per_line, uint16_t* pos_ptr)
{
    *pos_ptr = (uint16_t)(pix_per_line * (float)( (toa - risetime[header.chipindex]*2))  / (float)((falltime[header.chipindex] - risetime[header.chipindex])*2));
} 

void TPX3::resetFile()
{
    pos = 0;
    file_raw.clear();
    file_raw.seekg(0, std::ios::beg);
}
