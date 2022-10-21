#ifndef __UTIL_H__
#define __UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
namespace ekko {

std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");
class FSUtil {
public:
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode);
};
}
#endif