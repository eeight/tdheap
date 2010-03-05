#include "t2_procmap.h"

extern "C" {
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_vki.h"

#include "fcntl.h"
}

#include "m_stl/std/string" 
#include "m_stl/std/vector" 

namespace {

class Mapping {
public:
    Mapping() {}
    Mapping(Addr begin, Addr end, int mode) :
        begin_(begin), end_(end), mode_(mode)
    {}

    bool within(Addr addr) {
        return addr >= begin_ && addr <= end_;
    }

    int mode() const { return mode_; }

private:
    Addr begin_, end_;
    int mode_;
};

class File {
public:
    File(char *filename)
    {
        char buffer[1024];
        SysRes res = VG_(open)((Char *)filename, O_RDONLY, 0);
        if (res._isError) {
            VG_(tool_panic)((Char *)"Cannot open file");
        }
        int fd = res._val;
        while (int r = VG_(read)(fd, buffer, 1023)) {
            buffer[r] = 0;
            buffer_ += buffer;
        }
        VG_(close)(fd);
    }

    ~File() {
    }

    bool readLine(std::string *result) {
        if (buffer_.empty()) {
            return false;
        }

        size_t pos = buffer_.find('\n');
        if (pos != std::string::npos) {
            ++pos;
        }

        *result = buffer_.substr(0, pos);
        buffer_ = buffer_.substr(pos, std::string::npos);

        return true;
    }
    
private:
    std::string buffer_;
};

Addr charToHex(char c) {
    if (VG_(isdigit)(c)) {
        return c - '0';
    } else {
        return 10 + c - 'a';
    }
}

Addr stringToHex(const std::string &s) {
    Addr res = 0;

    for (size_t i = 0; i < s.size(); ++i) {
        res = res*16 + charToHex(s[i]);
    }

    return res;
}

enum {
    READ_MOD = 0x1,
    WRITE_MOD = 0x2
};

std::vector<Mapping> *g_mappings;

bool checkMode(Addr addr, int mask) {
    for (size_t i = 0; i != g_mappings->size(); ++i) {
        if ((*g_mappings)[i].within(addr)) {
            return (*g_mappings)[i].mode() & mask;
        }
    }

    return false;
}

}

void initProcmap() {
    char filename[1024];
    int pid = VG_(getpid)();
    VG_(snprintf)((Char *)filename, 1024, (HChar *)"/proc/%d/maps", pid);

    File map(filename);
    std::string line;

    g_mappings = new std::vector<Mapping>();

    while (map.readLine(&line)) {
        size_t begin_pos = line.find('-');
        size_t end_pos = line.find(' ');
        size_t mode_pos = line.find(' ', end_pos + 1);
        std::string mode = line.substr(end_pos + 1, mode_pos - end_pos - 1);

        Addr begin = stringToHex(line.substr(0, begin_pos));
        Addr end = stringToHex(line.substr(begin_pos + 1, end_pos - begin_pos - 1));
        bool can_read = mode.find('r') != std::string::npos;
        bool can_write = mode.find('w') != std::string::npos;

#if 0
        VG_(printf)((HChar *)"Line: %s", line.c_str());
        VG_(printf)("%p-%p, read: %d, write: %d\n", begin, end, (int)can_read, (int)can_write);
#endif
        g_mappings->push_back(Mapping(begin, end, (can_read*READ_MOD)|(can_write*WRITE_MOD)));
    }
}
void shutdownProcmap() {
    delete g_mappings;
}

bool isAddressReadable(Addr addr) {
    return checkMode(addr, READ_MOD);
}

bool isAddressWritable(Addr addr) {
    return checkMode(addr, WRITE_MOD);
}
