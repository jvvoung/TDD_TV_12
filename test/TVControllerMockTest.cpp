#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "MockTuner.h"
#include "TVController.h"

using ::testing::InSequence;
using ::testing::Return;

class ControllerMockTest : public ::testing::Test {
protected:
    MockTuner mockTuner;
    std::unique_ptr<TVController> ctrl;

    void SetUp() override {
        ctrl = std::make_unique<TVController>(mockTuner);
    }
};

// --- 교재 12페이지: GMock setCH / getCurrentCH 호출 행위 검증 ---

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

// --- 다음 선호 채널 wrap-around: setCH 호출 검증 ---

TEST_F(ControllerMockTest, NextFavorite_WrapAround_SetCH1) {
    ctrl->addFavorite(1);
    ctrl->addFavorite(56);
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(Return("56"));
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    ctrl->pressNextFavorite();
}

// --- 숫자 버튼 setCH 호출 추가 검증 ---

TEST_F(ControllerMockTest, Press1234_SetCH12Then34) {
    InSequence seq;
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    EXPECT_CALL(mockTuner, setCH("34")).Times(1);
    ctrl->pressNumber(1);
    ctrl->pressNumber(2);
    ctrl->pressNumber(3);
    ctrl->pressNumber(4);
}

TEST_F(ControllerMockTest, Zero7_SetCH7) {
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    ctrl->pressNumber(0);
    ctrl->pressNumber(7);
}

TEST_F(ControllerMockTest, NextFavorite_EmptyList_NoSetCH) {
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    EXPECT_CALL(mockTuner, getCurrentCH()).Times(0);
    ctrl->pressNextFavorite();
}
