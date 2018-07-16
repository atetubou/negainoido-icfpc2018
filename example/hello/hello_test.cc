#include "example/hello/hello.h"

#include <unistd.h>
#include <cstdlib>

#include <memory>

#include "gtest/gtest.h"

TEST(Hello, Hello) {
  std::unique_ptr<char, decltype(&std::free)> p(get_current_dir_name(), &std::free);
  hello();
  EXPECT_NE(p.get(), nullptr);
}
