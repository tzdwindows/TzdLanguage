#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

struct PdbSymbol {
    std::string name;
    uint32_t rva;
};

class PdbReader {
public:
    PdbReader(const std::string& path);
    ~PdbReader();

    bool IsValid() const { return m_valid; }

    bool ParsePublicSymbols(const std::vector<uint32_t>& sectionRvas, std::vector<PdbSymbol>& outSymbols);

private:
    std::ifstream m_file;
    bool m_valid = false;
    uint32_t m_blockSize = 0;

    // ’‚¿Ô…æ≥˝¡À struct SuperBlock; 

    std::vector<uint32_t> m_directoryStreamBlocks;
    std::vector<std::vector<uint32_t>> m_streamMap;

    void LoadMsfDirectory();
    std::vector<uint8_t> ReadStream(uint32_t streamIndex);
    void ReadBlock(uint32_t blockIndex, void* buffer);
    uint32_t GetBlockCount(uint32_t streamSize);
};