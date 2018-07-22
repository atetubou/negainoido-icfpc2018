#ifndef SRC_NBT_LOADER_H_
#define SRC_NBT_LOADER_H_

#include <string>

#include "json/json.h"

const char kHALT = 0b11111111;
const char kWAIT = 0b11111110;
const char kFLIP = 0b11111101;

bool getshort(int a, int i, int* dx, int* dy, int* dz);

bool getlong(int a, int i, int* dx, int* dy, int* dz);

bool getnearcoordinate(int nd, int* dx, int* dy, int* dz);

std::string binary(int x);

int parse_command(const std::string& nbt_content, int i, int* nanobot_num, std::ostringstream* ss,
                  Json::Value* command);

#endif  // SRC_NBT_LOADER_H_
 
