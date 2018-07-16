#include "gtest/gtest.h"

#include <sstream>

#include "absl/strings/str_cat.h"
#include "json/json.h"

TEST(Abseil, StrCat) {
  EXPECT_EQ("ab", absl::StrCat("a", "b"));
}

TEST(jsoncpp, Set) {
  Json::CharReaderBuilder builder;
  Json::Value v;
  std::string err;
  std::istringstream ss("1");
  ASSERT_TRUE(Json::parseFromStream(builder, ss, &v, &err));
  EXPECT_EQ(v, 1);
}
