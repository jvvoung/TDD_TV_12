#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <memory>
#include "FakeTuner.h"
#include "Tuner.h"
#include "TVController.h"

using ::testing::_;
using ::testing::Return;

class MockTunerForController : public Tuner {
public:
    MOCK_METHOD(std::string, seekCH, (), (override));
    MOCK_METHOD(void, setCH, (const std::string& ch), (override));
    MOCK_METHOD(std::string, getCurrentCH, (), (override));
};

class ControllerMockTest : public ::testing::Test {
protected:
    MockTunerForController mockTuner;
    std::unique_ptr<TVController> ctrl;

    void SetUp() override {
        ctrl = std::make_unique<TVController>(mockTuner);
    }
};

class ControllerTest : public ::testing::Test {
protected:
    std::unique_ptr<FakeTuner> tuner;
    std::unique_ptr<TVController> ctrl;

    void SetUp() override {
        tuner = std::make_unique<FakeTuner>(std::vector<int>{1, 4, 12, 56});
        ctrl = std::make_unique<TVController>(*tuner);
    }
};

// --- 기능 1: 숫자 버튼 채널 변경 (교재 9~10페이지) ---

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

// --- 기능 2: 선호 채널 토글 (교재 10페이지) ---

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
    EXPECT_EQ(3u, favs.size());
    EXPECT_NE(favs.end(), std::find(favs.begin(), favs.end(), 6));
    EXPECT_NE(favs.end(), std::find(favs.begin(), favs.end(), 12));
    EXPECT_NE(favs.end(), std::find(favs.begin(), favs.end(), 37));
}

// --- GMock: setCH / getCurrentCH 호출 행위 검증 (교재 11~12페이지) ---

TEST_F(ControllerMockTest, PressNumber1Confirm) {
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    ctrl->pressNumber(1);
    ctrl->pressConfirm();
}

TEST_F(ControllerMockTest, Press1Then2_SetCH12) {
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    ctrl->pressNumber(1);
    ctrl->pressNumber(2);
}

TEST_F(ControllerMockTest, PressFavorite_GetsCurrentCH) {
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(Return("12"));
    ctrl->pressFavorite();
}

TEST_F(ControllerMockTest, NextFav_CallsSetCH) {
    ctrl->addFavorite(12);
    ctrl->addFavorite(56);
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(Return("6"));
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    ctrl->pressNextFavorite();
}
