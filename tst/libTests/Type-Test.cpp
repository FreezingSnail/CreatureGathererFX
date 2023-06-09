#include "gtest/gtest.h"
#include "lib/TypeTable.hpp"

TEST(MatchupTest, Typematchups) {

    ASSERT_EQ (getMatchupModifier(3, 2),  0);
    ASSERT_EQ (getMatchupModifier(0, 0),  2);
    ASSERT_EQ (getMatchupModifier(1, 0),  2);
    ASSERT_EQ (getMatchupModifier(0, 1),  2);
    ASSERT_EQ (getMatchupModifier(1, 3),  3);

}