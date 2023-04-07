#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <array>
#include <vector>

#ifndef TPX3READER_H
#define TPX3READER_h

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif


PACK(struct dtype_header
    {
        uint8_t tee;
        uint8_t pee;
        uint8_t ax;
        uint8_t three;
        uint8_t chipindex;
        uint8_t reserved;
        uint16_t chunksize;
    });
PACK(struct dtype_event
    {
        uint8_t kx;
        uint8_t ky;
        uint64_t toa;
        uint8_t chip;
    });
PACK(struct dtype_tdc
    {
        uint8_t trigger;
        uint64_t toa;
        uint8_t chip;
    });
PACK(struct dtype_4d
    {
        uint16_t kx;
        uint16_t ky;
        uint16_t rx;
        uint16_t ry;
    });



class TPX3
{
public:
    void openFile_event(std::string file, std::string path);
    void openFile_4d(std::string file);
    void openFile_csr(std::string file, std::string path);
    void closeFile();
    void process_event();
    void process_4d(uint16_t pix_per_line);
    void process_csr(uint16_t pix_per_line);
    TPX3(){};

private:
    std::ifstream file_raw;
    std::ofstream file_tdc;
    std::ofstream file_event;
    std::ofstream file_4d;
    std::ofstream file_val;
    std::ofstream file_col;
    std::ofstream file_row;
    std::uintmax_t file_size;
    std::uintmax_t pos;

    unsigned int pixaddr : 20;
    uint64_t toa;

    uint64_t risetime[4];
    uint64_t falltime[4];
    bool risefall[4] = {false, false, false, false};
    int event_count[4] = {0,0,0,0};
    int line_count[4] = {-1,-1,-1,-1};
    std::vector<std::vector<dtype_event>> event_line = std::vector<std::vector<dtype_event>>(4, std::vector<dtype_event>(1000000));    

    dtype_header header;
    dtype_tdc tdc;
    dtype_event event;
    dtype_4d fourd;
    uint64_t packet;
    size_t header_size{ sizeof(header) };
    size_t tdc_size{ sizeof(tdc) };
    size_t event_size{ sizeof(event) };
    size_t packet_size{ sizeof(packet) };
    size_t fourd_size{ sizeof(fourd) };

    void resetFile();
    void read(char *buffer, size_t buffer_size);
    uint8_t isTdc();
    void writeTdc();
    void writeEvent();
    void write4d(dtype_event* event_ptr, uint16_t pix_per_line);
    void writeCsr(dtype_event* event_ptr, uint16_t pix_per_line);
    void updateEvent(dtype_event* event_ptr);
    bool updateLine();
    void toa_2_pos(uint64_t toa, uint16_t pix_per_line, uint16_t* pos_ptr);
};
#endif