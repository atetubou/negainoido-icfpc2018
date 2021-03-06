#ifndef SRC_BASE_BASE_H_
#define SRC_BASE_BASE_H_

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "json/json.h"

std::string ReadFile(absl::string_view filename);

using v = std::vector<int>;
using vv = std::vector<v>;
using vvv = std::vector<vv>;

vvv ReadMDL(absl::string_view filename);
void WriteMDL(absl::string_view filename, const vvv &M);

void OutputMDL(const vvv &M);
std::string encodecommand(const Json::Value& command);
std::string Json2Binary(const Json::Value& json);

#endif // SRC_BASE_BASE_H_
