#ifndef __FILE_ENUM_H__
#define __FILE_ENUM_H__

#include <string>
#include "zpack.h"

typedef bool (*EnumCallback)(const zp::String& path, zp::u32 fileSize, void* param);

bool enumFile(const zp::String& searchPath, EnumCallback callback, void* param);

bool addPackFile(const zp::String& filename, zp::u32 fileSize, void* param);

bool countFile(const zp::String& filename, zp::u32 fileSize, void* param);

#endif
