#include "tpx3reader.h"

void TPX3::openFile(std::string file)
{
    size_t filesp_idx = file.find_last_of("/\\");
    std::string path = file.substr(0,filesp_idx+1);
    

    std::string file_name_4d = "event.dat";

    file_size = std::filesystem::file_size(file);
    file_raw.open(file, std::ios::in | std::ios::binary);
    file_event.open(path+file_name_4d, std::ios::out | std::ios::binary);
    if (file_raw.is_open() & file_event.is_open())
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
    if (file_event.is_open())
    {
        file_event.close();
    }
}

void TPX3::process(uint16_t pix_per_line, bool timestamp)
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
                    if (timestamp){
                        write5d(event_ptr, pix_per_line);
                    } else {
                        write4d(event_ptr, pix_per_line);
                    }
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
        // if ( (risetime[header.chipindex]+100) < risetime[header.chipindex-1] ){
        //     printf("error");
        // }
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
                file_event.write((const char*)&fourd, fourd_size);
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
                file_event.write((const char*)&fourd, fourd_size);
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
                file_event.write((const char*)&fourd, fourd_size);
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
                file_event.write((const char*)&fourd, fourd_size);
            }
        }
        break;
    }
    std::cout << fourd.ry << ":" << event_count[header.chipindex] << std::endl;
}


void TPX3::write5d(dtype_event* event_ptr, uint16_t pix_per_line)
{
    switch (header.chipindex)
    {
    case 0:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2)) 
            {
                fived.kx = 256+(uint16_t)(*(event_ptr+i)).kx;
                fived.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fived.rx);
                fived.ry = line_count[header.chipindex];
                fived.toa = (uint64_t)(*(event_ptr+i)).toa;
                file_event.write((const char*)&fived, fived_size);
            }
        }
        break;
    case 1:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fived.kx = 511-(uint16_t)(*(event_ptr+i)).kx;
                fived.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fived.rx);
                fived.ry = line_count[header.chipindex];
                fived.toa = (uint64_t)(*(event_ptr+i)).toa;
                file_event.write((const char*)&fived, fived_size);
            }
        }
        break;
    case 2:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fived.kx = 255-(uint16_t)(*(event_ptr+i)).kx;
                fived.ky = 511-(uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fived.rx);
                fived.ry = line_count[header.chipindex];
                fived.toa = (uint64_t)(*(event_ptr+i)).toa;
                file_event.write((const char*)&fived, fived_size);
            }
        }
        break;
    case 3:
        for (int i=0; i<event_count[header.chipindex]; i++)
        {
            if ((*(event_ptr+i)).toa > (risetime[header.chipindex]*2))
            {
                fived.kx = (uint16_t)(*(event_ptr+i)).kx;
                fived.ky = (uint16_t)(*(event_ptr+i)).ky;
                toa_2_pos((*(event_ptr+i)).toa, pix_per_line, &fived.rx);
                fived.ry = line_count[header.chipindex];
                fived.toa = (uint64_t)(*(event_ptr+i)).toa;
                file_event.write((const char*)&fived, fived_size);
            }
        }
        break;
    }
    std::cout << fived.ry << ":" << event_count[header.chipindex] << std::endl;
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
