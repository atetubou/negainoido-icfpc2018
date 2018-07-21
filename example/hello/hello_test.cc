#include "example/hello/hello.h"

#include <memory>

#include "gtest/gtest.h"

TEST(Hello, Hello) {
  std::unique_ptr<char> p;
  hello();
  EXPECT_EQ(p.get(), nullptr);
}
