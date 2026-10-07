#include "Res/TzdStrings.h"
#include "PdbReader.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <set>

// --- PDB/MSF 格式定义 ---

struct SuperBlock {
    char signature[32];
    uint32_t blockSize;
    uint32_t freeBlockMapBlock;
    uint32_t numBlocks;
    uint32_t directorySize;
    uint32_t unknown;
    uint32_t blockMapAddr;
};

static const char PDB70_SIG[] = "Microsoft C/C++ MSF 7.00\r\n\x1A\x44\x53\0\0\0";

// 标准 DBI 头部
struct DbiHeader {
    int32_t sig;            // 0
    uint32_t ver;           // 4
    uint32_t age;           // 8
    uint16_t gssyms;        // 12 (Global Stream)
    uint16_t build;         // 14
    uint16_t pssyms;        // 16 (Public Stream)
    uint16_t pdbver;        // 18
    uint16_t symrecs;       // 20 (Symbol Records - 核心数据流)
    uint16_t rbld;          // 22
    uint32_t modisize;      // 24
    uint32_t seccontsize;   // 28
    // ... 后续字段省略
};

// --- 记录结构定义 ---

// S_PUB32 (0x110E), S_PROCREF (0x1125), S_LPROCREF (0x1127)
// S_GDATA32 (0x110D), S_LDATA32 (0x110C)
#pragma pack(push, 1)
struct PubSymLayout {
    uint32_t flags; // or checksum
    uint32_t off;   // 偏移量
    uint16_t seg;   // 段索引
    char name[1];
};

// S_GPROC32 (0x1110), S_LPROC32 (0x110F) - 函数专用
// 它们的头部比 PubSym 长很多
struct ProcSymLayout {
    uint32_t pParent;
    uint32_t pEnd;
    uint32_t pNext;
    uint32_t procLen;
    uint32_t debugStart;
    uint32_t debugEnd;
    uint32_t type;
    uint32_t off;   // 偏移量在第 28 字节 (7 * 4)
    uint16_t seg;   // 段索引
    uint8_t flags;  // flags 是 1 字节
    char name[1];
};
#pragma pack(pop)

// --- 实现 ---

PdbReader::PdbReader(const std::string& path) {
    m_file.open(path, std::ios::binary);
    if (!m_file.is_open()) {
        std::cout << TzdPdb::FILE_OPEN_FAILED << std::endl;
        return;
    }

    SuperBlock sb;
    m_file.read((char*)&sb, sizeof(sb));

    if (memcmp(sb.signature, PDB70_SIG, sizeof(PDB70_SIG) - 1) != 0) {
        std::cout << TzdPdb::SIGNATURE_MISMATCH << std::endl;
        return;
    }

    m_blockSize = sb.blockSize;
    if (m_blockSize == 0) return;

    uint32_t numDirectoryBlocks = GetBlockCount(sb.directorySize);
    std::vector<uint32_t> dirIndices(numDirectoryBlocks);
    std::vector<uint8_t> blockMapBuf(m_blockSize);
    ReadBlock(sb.blockMapAddr, blockMapBuf.data());

    size_t copySize = dirIndices.size() * sizeof(uint32_t);
    if (copySize > m_blockSize) copySize = m_blockSize;
    memcpy(dirIndices.data(), blockMapBuf.data(), copySize);

    m_directoryStreamBlocks = dirIndices;
    LoadMsfDirectory();
    m_valid = true;
}

PdbReader::~PdbReader() {
    if (m_file.is_open()) m_file.close();
}

uint32_t PdbReader::GetBlockCount(uint32_t streamSize) {
    if (m_blockSize == 0) return 0;
    return (streamSize + m_blockSize - 1) / m_blockSize;
}

void PdbReader::ReadBlock(uint32_t blockIndex, void* buffer) {
    if (m_blockSize == 0) return;
    m_file.seekg((uint64_t)blockIndex * m_blockSize, std::ios::beg);
    m_file.read((char*)buffer, m_blockSize);
}

void PdbReader::LoadMsfDirectory() {
    std::vector<uint8_t> dirData;
    for (uint32_t idx : m_directoryStreamBlocks) {
        std::vector<uint8_t> buf(m_blockSize);
        ReadBlock(idx, buf.data());
        dirData.insert(dirData.end(), buf.begin(), buf.end());
    }

    if (dirData.size() < 4) return;
    uint32_t* ptr = (uint32_t*)dirData.data();
    uint32_t numStreams = *ptr++;

    if (numStreams > 0xFFFF) return;

    std::vector<uint32_t> sizes;
    for (uint32_t i = 0; i < numStreams; i++) sizes.push_back(*ptr++);

    m_streamMap.resize(numStreams);
    for (uint32_t i = 0; i < numStreams; i++) {
        uint32_t sz = sizes[i];
        if (sz == 0xFFFFFFFF) sz = 0;
        uint32_t blks = GetBlockCount(sz);
        for (uint32_t j = 0; j < blks; j++) m_streamMap[i].push_back(*ptr++);
    }
}

std::vector<uint8_t> PdbReader::ReadStream(uint32_t streamIndex) {
    if (streamIndex >= m_streamMap.size()) return {};
    std::vector<uint8_t> res;
    for (uint32_t idx : m_streamMap[streamIndex]) {
        std::vector<uint8_t> buf(m_blockSize);
        ReadBlock(idx, buf.data());
        res.insert(res.end(), buf.begin(), buf.end());
    }
    return res;
}

// 核心扫描函数
int ScanStreamRobust(const std::vector<uint8_t>& data,
    const std::vector<uint32_t>& sectionRvas,
    std::vector<PdbSymbol>& outSymbols,
    std::set<uint32_t>& processedRvas)
{
    if (data.empty()) return 0;
    int count = 0;
    size_t maxOff = data.size();
    const uint8_t* raw = data.data();

    // 步进1字节扫描
    for (size_t i = 0; i < maxOff - 40; ++i) { // 预留足够的头部空间
        uint16_t len = *(uint16_t*)(raw + i);
        uint16_t type = *(uint16_t*)(raw + i + 2);

        if (len < 6 || len > 4096) continue;
        if (i + len + 2 > maxOff) continue;

        uint32_t off = 0;
        uint16_t seg = 0;
        const char* nameStart = nullptr;
        bool isFunc = false;

        // --- 分类处理 ---

        // 1. 类 Public 符号 (偏移量在 4)
        // S_PUB32(0x110E), S_PROCREF(0x1125), S_LPROCREF(0x1127), S_GDATA32(0x110D), S_LDATA32(0x110C)
        if (type == 0x110E || type == 0x1125 || type == 0x1127 || type == 0x110D || type == 0x110C) {
            const PubSymLayout* body = (const PubSymLayout*)(raw + i + 4); // 跳过 len(2)+type(2)
            off = body->off;
            seg = body->seg;
            nameStart = body->name;
            isFunc = true;
        }
        // 2. 类 Procedure 符号 (偏移量在 28, 结构更复杂)
        // S_GPROC32(0x1110), S_LPROC32(0x110F), S_GPROC32_ID(0x1147), S_LPROC32_ID(0x1146)
        else if (type == 0x1110 || type == 0x110F || type == 0x1147 || type == 0x1146) {
            const ProcSymLayout* body = (const ProcSymLayout*)(raw + i + 4);
            off = body->off;
            seg = body->seg;
            nameStart = body->name;
            isFunc = true;
        }

        if (isFunc) {
            // 校验 Seg 索引
            if (seg > 0 && seg <= sectionRvas.size()) {
                uint32_t rvaBase = sectionRvas[seg - 1];
                uint32_t finalRva = rvaBase + off;

                if (processedRvas.find(finalRva) == processedRvas.end()) {
                    // 安全读取字符串
                    // 计算 body 已经占用的字节数，剩余的就是名字
                    // 粗略计算：maxNameLen = len - (nameStart - (raw+i+2))
                    size_t headerSize = (size_t)(nameStart - (const char*)(raw + i + 2));
                    if (len > headerSize) {
                        size_t maxNameLen = len - headerSize;
                        size_t actLen = 0;
                        while (actLen < maxNameLen && nameStart[actLen] != 0) actLen++;

                        if (actLen > 0) {
                            std::string s(nameStart, actLen);
                            // 过滤掉编译器生成的特殊符号 (可选)
                            // if (s.find("__") != 0) ...
                            outSymbols.push_back({ s, finalRva });
                            processedRvas.insert(finalRva);
                            count++;

                            // 优化：跳过当前记录
                            // i += (len + 1); 
                        }
                    }
                }
            }
        }
    }
    return count;
}

bool PdbReader::ParsePublicSymbols(const std::vector<uint32_t>& sectionRvas, std::vector<PdbSymbol>& outSymbols) {
    if (!m_valid) return false;

    // 读取 DBI
    std::vector<uint8_t> dbiData = ReadStream(3);
    if (dbiData.size() < sizeof(DbiHeader)) return false;
    DbiHeader* dbi = (DbiHeader*)dbiData.data();
    if (dbi->sig != -1) return false;

    // 收集所有可能的符号流
    std::vector<uint16_t> streamsToCheck;
    if (dbi->symrecs > 0 && dbi->symrecs < m_streamMap.size()) streamsToCheck.push_back(dbi->symrecs);
    if (dbi->gssyms > 0 && dbi->gssyms < m_streamMap.size()) streamsToCheck.push_back(dbi->gssyms);
    if (dbi->pssyms > 0 && dbi->pssyms < m_streamMap.size()) streamsToCheck.push_back(dbi->pssyms);

    if (streamsToCheck.empty()) {
        std::cout << TzdPdb::NO_VALID_STREAMS << std::endl;
        return false;
    }

    std::set<uint32_t> processedRvas;
    int total = 0;

    // 去重流索引 (symrecs 和 gssyms 有时指向同一个)
    std::sort(streamsToCheck.begin(), streamsToCheck.end());
    streamsToCheck.erase(std::unique(streamsToCheck.begin(), streamsToCheck.end()), streamsToCheck.end());

    for (uint16_t idx : streamsToCheck) {
        std::vector<uint8_t> sData = ReadStream(idx);
        if (!sData.empty()) {
            // std::cout << "[PdbReader] 扫描流 #" << idx << " (Size: " << sData.size() << ")..." << std::endl;
            total += ScanStreamRobust(sData, sectionRvas, outSymbols, processedRvas);
        }
    }

    if (total == 0) {
        std::cout << TzdPdb::STREAM_WARNING;
        for (auto i : streamsToCheck) std::cout << i << " ";
        std::cout << "但未发现任何符号 (PUB32/GPROC32)。" << std::endl;
    }
    else {
        std::cout << "[PdbReader] 解析成功: 找到 " << total << " 个符号。" << std::endl;
    }

    return total > 0;
}