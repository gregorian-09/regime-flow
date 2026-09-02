#include "regimeflow/common/result.h"

#include <gtest/gtest.h>

namespace {

TEST(ResultHelpers, SupportsSnakeCaseConstruction) {
    const auto value = regimeflow::ok(42);
    ASSERT_TRUE(value.is_ok());
    EXPECT_EQ(value.value(), 42);

    const auto error = regimeflow::err(regimeflow::Error::Code::InvalidArgument, "invalid input");
    EXPECT_EQ(error.code, regimeflow::Error::Code::InvalidArgument);
    EXPECT_EQ(error.message, "invalid input");
}

TEST(ResultHelpers, RetainsLegacyConstructionForExistingConsumers) {
    const auto value = regimeflow::Ok(42);
    ASSERT_TRUE(value.is_ok());
    EXPECT_EQ(value.value(), 42);
}

}  // namespace
