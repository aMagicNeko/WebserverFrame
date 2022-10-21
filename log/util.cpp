#include "util.h"

namespace ekko{
bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode)
{
    ofs.open(filename.c_str(), mode);   
    return ofs.is_open();
}
}