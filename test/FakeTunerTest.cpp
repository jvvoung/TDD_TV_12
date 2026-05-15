#include <gtest/gtest.h>
#include "FakeTuner.h"

class FakeTunerTest : public ::testing::Test {
protected:
    FakeTuner tuner{{1, 4, 12, 56}};
};

TEST_F(FakeTunerTest, initChannelInRange) {
    EXPECT_EQ("0", tuner.getCurrentCH());
    int ch = std::stoi(tuner.getCurrentCH());
    EXPECT_GE(ch, 0);
    EXPECT_LE(ch, 99);
}

TEST_F(FakeTunerTest, setCHValidChannel) {
    tuner.setCH("12");
    EXPECT_EQ("12", tuner.getCurrentCH());
}

TEST_F(FakeTunerTest, setCHInvalidChannelThrows) {
    EXPECT_THROW(tuner.setCH("100"), std::invalid_argument);
    EXPECT_THROW(tuner.setCH("-1"), std::invalid_argument);
}

TEST_F(FakeTunerTest, seekCHMovesToNextAvailable) {
    tuner.setCH("6");
    EXPECT_EQ("12", tuner.seekCH());
    EXPECT_EQ("12", tuner.getCurrentCH());
}

TEST_F(FakeTunerTest, seekCHWrapAround) {
    tuner.setCH("56");
    EXPECT_EQ("1", tuner.seekCH());
    EXPECT_EQ("1", tuner.getCurrentCH());
}

TEST_F(FakeTunerTest, seekCH10timesReturnsValidChannels) {
    for (int i = 0; i < 10; ++i) {
        std::string ch = tuner.seekCH();
        int v = std::stoi(ch);
        EXPECT_GE(v, 0);
        EXPECT_LE(v, 99);
    }
}
