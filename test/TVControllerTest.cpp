#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <vector>
#include "FakeTuner.h"
#include "TVController.h"

class ControllerTest : public ::testing::Test {
protected:
    std::unique_ptr<FakeTuner> tuner;
    std::unique_ptr<TVController> ctrl;

    void SetUp() override {
        tuner = std::make_unique<FakeTuner>(std::vector<int>{1, 4, 12, 56});
        ctrl = std::make_unique<TVController>(*tuner);
    }
};

// --- 기능 1: 숫자 버튼 채널 변경 ---

TEST_F(ControllerTest, PressNumber1ThenConfirm) {
    ctrl->pressNumber(1);
    ctrl->pressConfirm();
    EXPECT_EQ("1", tuner->getCurrentCH());
}

TEST_F(ControllerTest, Press1Then2_AutoChange) {
    ctrl->pressNumber(1);
    ctrl->pressNumber(2);
    EXPECT_EQ("12", tuner->getCurrentCH());
}

TEST_F(ControllerTest, Press1234_TwoStageChange) {
    ctrl->pressNumber(1);
    ctrl->pressNumber(2);
    EXPECT_EQ("12", tuner->getCurrentCH());

    ctrl->pressNumber(3);
    ctrl->pressNumber(4);
    EXPECT_EQ("34", tuner->getCurrentCH());
}

TEST_F(ControllerTest, OtherButtonCancelsBuffer) {
    ctrl->pressNumber(4);
    ctrl->pressNumber(5);
    EXPECT_EQ("45", tuner->getCurrentCH());

    ctrl->pressNumber(6);
    ctrl->pressOther();
    EXPECT_EQ("45", tuner->getCurrentCH());
}

TEST_F(ControllerTest, Zero7_SingleDigit7) {
    ctrl->pressNumber(0);
    ctrl->pressNumber(7);
    EXPECT_EQ("7", tuner->getCurrentCH());
}

TEST_F(ControllerTest, PressInvalidDigitThrows) {
    EXPECT_THROW(ctrl->pressNumber(-1), std::invalid_argument);
    EXPECT_THROW(ctrl->pressNumber(10), std::invalid_argument);
}

// --- 기능 2: 선호 채널 토글 ---

TEST_F(ControllerTest, FavoriteAdd_NewChannel) {
    tuner->setCH("12");
    ctrl->pressFavorite();

    const auto& favs = ctrl->getFavoriteChannels();
    EXPECT_NE(favs.end(), std::find(favs.begin(), favs.end(), 12));
}

TEST_F(ControllerTest, FavoriteToggle_Remove) {
    tuner->setCH("12");
    ctrl->pressFavorite();
    ctrl->pressFavorite();

    const auto& favs = ctrl->getFavoriteChannels();
    EXPECT_EQ(favs.end(), std::find(favs.begin(), favs.end(), 12));
}

TEST_F(ControllerTest, FavoriteToggleScenario) {
    for (int ch : {12, 8, 37, 8, 6}) {
        tuner->setCH(std::to_string(ch));
        ctrl->pressFavorite();
    }

    const auto& favs = ctrl->getFavoriteChannels();
    ASSERT_EQ(3u, favs.size());
    EXPECT_EQ(6, favs[0]);
    EXPECT_EQ(12, favs[1]);
    EXPECT_EQ(37, favs[2]);
}

TEST_F(ControllerTest, FavoriteChannelsAreSorted) {
    for (int ch : {37, 12, 6}) {
        tuner->setCH(std::to_string(ch));
        ctrl->pressFavorite();
    }

    const std::vector<int> expected{6, 12, 37};
    EXPECT_EQ(expected, ctrl->getFavoriteChannels());
}

TEST_F(ControllerTest, AddFavoriteInvalidChannelThrows) {
    EXPECT_THROW(ctrl->addFavorite(-1), std::invalid_argument);
    EXPECT_THROW(ctrl->addFavorite(100), std::invalid_argument);
}

// --- 기능 3: 다음 선호 채널 ---

TEST_F(ControllerTest, NextFavorite_Normal) {
    for (int ch : {1, 4, 12, 56}) {
        ctrl->addFavorite(ch);
    }

    tuner->setCH("6");
    ctrl->pressNextFavorite();
    EXPECT_EQ("12", tuner->getCurrentCH());
}

TEST_F(ControllerTest, NextFavorite_WrapAround) {
    for (int ch : {1, 4, 12, 56}) {
        ctrl->addFavorite(ch);
    }

    tuner->setCH("56");
    ctrl->pressNextFavorite();
    EXPECT_EQ("1", tuner->getCurrentCH());
}

TEST_F(ControllerTest, NextFavorite_CurrentChannelNotInFavoriteList) {
    for (int ch : {1, 4, 12, 56}) {
        ctrl->addFavorite(ch);
    }

    tuner->setCH("15");
    ctrl->pressNextFavorite();
    EXPECT_EQ("56", tuner->getCurrentCH());
}

TEST_F(ControllerTest, NextFavorite_EmptyList) {
    tuner->setCH("6");
    ctrl->pressNextFavorite();
    EXPECT_EQ("6", tuner->getCurrentCH());
}
