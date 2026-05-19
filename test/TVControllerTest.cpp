#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Tuner.h"
#include "TVController.h"

class MockTunerForController : public Tuner {
public:
    MOCK_METHOD(std::string, seekCH, (), (override));
    MOCK_METHOD(void, setCH, (const std::string& ch), (override));
    MOCK_METHOD(std::string, getCurrentCH, (), (override));
};

class TVControllerTest : public ::testing::Test {
protected:
    MockTunerForController mockTuner;
    TVController controller{&mockTuner};

    void givenCurrentChannel(const std::string& ch) {
        ON_CALL(mockTuner, getCurrentCH()).WillByDefault(::testing::Return(ch));
    }

    void pressDigit(int digit) {
        ASSERT_GE(digit, 0);
        ASSERT_LE(digit, 9);
        const remoteKey keys[] = {
            remoteKey::KEY_0, remoteKey::KEY_1, remoteKey::KEY_2, remoteKey::KEY_3,
            remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_7,
            remoteKey::KEY_8, remoteKey::KEY_9
        };
        controller.pushButton(keys[digit]);
    }

    void addFavorites(const std::vector<std::string>& channels) {
        for (const auto& ch : channels) {
            EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return(ch));
            controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
        }
    }

    void runSearch(const std::vector<std::string>& seekReturns) {
        ::testing::InSequence seq;
        for (const auto& ch : seekReturns) {
            EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return(ch));
        }
        controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);
    }
};

// Area 1: digit input and confirm

TEST_F(TVControllerTest, TC_1_1_OneDigitThenOk) { // 1-1
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    pressDigit(1);
    controller.pushButton(remoteKey::KEY_OK);

    // Then — EXPECT_CALL above
}

TEST_F(TVControllerTest, TC_1_2_TwoDigitsImmediateNoOk) { // 1-2
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    pressDigit(1);
    pressDigit(2);

    // Then: channel 12 without OK
}

TEST_F(TVControllerTest, TC_1_3_FourDigitsTwelveThenThirtyFour) { // 1-3
    // Given
    givenCurrentChannel("0");
    ::testing::InSequence seq;

    // When
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    EXPECT_CALL(mockTuner, setCH("34")).Times(1);
    pressDigit(1);
    pressDigit(2);
    pressDigit(3);
    pressDigit(4);

    // Then: setCH twice
}

TEST_F(TVControllerTest, TC_1_4a_FourFiveSixThenOk) { // 1-4a
    // Given
    givenCurrentChannel("0");
    ::testing::InSequence seq;

    // When
    EXPECT_CALL(mockTuner, setCH("45")).Times(1);
    EXPECT_CALL(mockTuner, setCH("6")).Times(1);
    pressDigit(4);
    pressDigit(5);
    pressDigit(6);
    controller.pushButton(remoteKey::KEY_OK);

    // Then: 45 then 6
}

TEST_F(TVControllerTest, TC_1_4b_FourFiveSixUpInvalidatesSix) { // 1-4b
    // Given
    givenCurrentChannel("0");
    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockTuner, setCH("45")).Times(1);
        EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    }
    pressDigit(4);
    pressDigit(5);
    pressDigit(6);

    // When
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: 45 kept, pending 6 cleared, no extra setCH
}

TEST_F(TVControllerTest, TC_1_5_ZeroSeven) { // 1-5
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    pressDigit(0);
    pressDigit(7);

    // Then: leading zero normalized to 7
}

TEST_F(TVControllerTest, TC_A1_EmptyBufferOkNoOp) { // A1
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_OK);

    // Then — no-op
}

TEST_F(TVControllerTest, TC_A7_OneDigitOnlyNoSetCh) { // A7
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    pressDigit(1);

    // Then: one digit pending, no setCH
}

TEST_F(TVControllerTest, TC_A2_NineNineConfirm) { // A2
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH("99")).Times(1);
    pressDigit(9);
    pressDigit(9);

    // Then: 99 confirmed once
}

TEST_F(TVControllerTest, TC_DIG_SingleZeroThenOk) { // B1 channel 0
    // Given
    givenCurrentChannel("12");

    // When
    EXPECT_CALL(mockTuner, setCH("0")).Times(1);
    pressDigit(0);
    controller.pushButton(remoteKey::KEY_OK);

    // Then: channel 0
}

TEST_F(TVControllerTest, TC_DIG_NinetyNineBoundary) { // B1 channel 99
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, setCH("99")).Times(1);
    pressDigit(9);
    pressDigit(9);

    // Then: channel 99
}

// Area 2: favorite channel toggle

TEST_F(TVControllerTest, TC_2_1_AddFavoriteNoSetCh) { // 2-1
    // Given
    givenCurrentChannel("6");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);

    // Then: favorite added, no setCH
}

TEST_F(TVControllerTest, TC_2_2_RemoveFavoriteNoSetCh) { // 2-2
    // Given
    givenCurrentChannel("6");
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);

    // Then: favorite removed
}

TEST_F(TVControllerTest, TC_A6_FavToggleClearsDigitBuffer) { // A6
    // Given
    givenCurrentChannel("6");
    pressDigit(6);

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);

    // Then: buffer cleared, toggle only
}

TEST_F(TVControllerTest, TC_FAV_ToggleTwiceReturnsToNotFavorite) { // 2-1/2-2
    // Given
    givenCurrentChannel("4");

    // When: add then remove (indirect via NEXT_FAV no-op)
    EXPECT_CALL(mockTuner, getCurrentCH()).WillRepeatedly(::testing::Return("4"));
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: no favorites, no setCH
}

TEST_F(TVControllerTest, TC_FAV_AddMultipleChannels) { // sorted favorites
    // Given
    givenCurrentChannel("6");
    addFavorites({"56", "1", "12"});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: next favorite is 12
}

// Area 3: next favorite channel

TEST_F(TVControllerTest, TC_3_1_NextFavoriteToTwelve) { // 3-1
    // Given
    givenCurrentChannel("6");
    addFavorites({"1", "4", "12", "56"});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("12")).Times(1);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: channel 12
}

TEST_F(TVControllerTest, TC_3_2_NextFavoriteRotateToOne) { // 3-2
    // Given
    givenCurrentChannel("56");
    addFavorites({"1", "4", "12", "56"});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("56"));
    EXPECT_CALL(mockTuner, setCH("1")).Times(1);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: rotate to 1
}

TEST_F(TVControllerTest, TC_A3_NoFavoritesNoOp) { // A3
    // Given
    givenCurrentChannel("6");

    // When
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: no change
}

TEST_F(TVControllerTest, TC_NFAV_SingleFavoriteRotateToSelf) { // E5 / §9.1
    // Given
    givenCurrentChannel("7");
    addFavorites({"7"});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("7"));
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: setCH("7") once
}

TEST_F(TVControllerTest, TC_NFAV_FromTwelveToFiftySix) { // current equals favorite
    // Given
    givenCurrentChannel("12");
    addFavorites({"1", "4", "12", "56"});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("12"));
    EXPECT_CALL(mockTuner, setCH("56")).Times(1);
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);

    // Then: channel 56
}

// Area 4: channel search

TEST_F(TVControllerTest, TC_4_1_SearchUntilEmpty) { // 4-1
    // Given
    givenCurrentChannel("0");
    ::testing::InSequence seq;
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return("4"));
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return("14"));
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return(""));

    // When
    controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);

    // Then: seekCH 4 times
}

TEST_F(TVControllerTest, TC_A4_EmptySearchThenPlusOne) { // A4
    // Given
    givenCurrentChannel("50");
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return(""));
    controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("50"));
    EXPECT_CALL(mockTuner, setCH("51")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: empty search, +/-1 mode
}

TEST_F(TVControllerTest, TC_A5_SecondSearchReplacesResults) { // A5
    // Given
    givenCurrentChannel("10");
    runSearch({"4", "6", ""});
    runSearch({"10", ""});

    // When: UP with single result {10}
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("10"));
    EXPECT_CALL(mockTuner, setCH("10")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: second search results only
}

TEST_F(TVControllerTest, TC_SRCH_ClearsBufferBeforeSearch) { // E15
    // Given
    givenCurrentChannel("0");
    pressDigit(3);
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    ::testing::InSequence seq;
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return("5"));
    EXPECT_CALL(mockTuner, seekCH()).WillOnce(::testing::Return(""));

    // When
    controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);

    // Then: buffer cleared, search only
}

TEST_F(TVControllerTest, TC_SRCH_DedupSortViaNavigate) { // dedup and sort
    // Given
    givenCurrentChannel("6");
    runSearch({"14", "4", "6", "14", ""});

    // When: UP uses sorted unique {4,6,14}
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: channel 14
}

// Area 5: channel up/down without search results

TEST_F(TVControllerTest, TC_5_1_ChannelUpFromSix) { // 5-1
    // Given
    givenCurrentChannel("6");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("7")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then
}

TEST_F(TVControllerTest, TC_5_2_ChannelDownFromSix) { // 5-2
    // Given
    givenCurrentChannel("6");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("5")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);

    // Then
}

TEST_F(TVControllerTest, TC_5_3_NinetyNineUpWrapsToZero) { // 5-3
    // Given
    givenCurrentChannel("99");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("99"));
    EXPECT_CALL(mockTuner, setCH("0")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: wrap 99 to 0
}

TEST_F(TVControllerTest, TC_5_4_ZeroDownWrapsToNinetyNine) { // 5-4
    // Given
    givenCurrentChannel("0");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("0"));
    EXPECT_CALL(mockTuner, setCH("99")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);

    // Then: wrap 0 to 99
}

TEST_F(TVControllerTest, TC_5_ChannelOneUpToTwo) { // B1 경계 1
    // Given
    givenCurrentChannel("1");

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("1"));
    EXPECT_CALL(mockTuner, setCH("2")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then
}

// Area 6: channel up/down with search results

TEST_F(TVControllerTest, TC_6_1_SearchUpFromSixToFourteen) { // 6-1
    // Given
    givenCurrentChannel("6");
    runSearch({"4", "6", "14", ""});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then
}

TEST_F(TVControllerTest, TC_6_2_SearchDownFromSixToFour) { // 6-2
    // Given
    givenCurrentChannel("6");
    runSearch({"4", "6", "14", ""});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);

    // Then
}

TEST_F(TVControllerTest, TC_6_3_SearchUpFromFifteenToFour) { // 6-3
    // Given
    givenCurrentChannel("15");
    runSearch({"4", "6", "14", ""});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("15"));
    EXPECT_CALL(mockTuner, setCH("4")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: off-list wraps to min 4
}

TEST_F(TVControllerTest, TC_6_4_SearchDownFromFifteenToFourteen) { // 6-4
    // Given
    givenCurrentChannel("15");
    runSearch({"4", "6", "14", ""});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("15"));
    EXPECT_CALL(mockTuner, setCH("14")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_DOWN);

    // Then: off-list wraps to max 14
}

TEST_F(TVControllerTest, TC_A8_SingleSearchResultWrap) { // A8 / §9.2
    // Given
    givenCurrentChannel("6");
    runSearch({"6", ""});

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return("6"));
    EXPECT_CALL(mockTuner, setCH("6")).Times(1);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then: single result wrap setCH("6")
}

// Optional parameterized: wrap boundaries

struct WrapChannelParam {
    std::string current;
    remoteKey key;
    std::string expected;
};

class TVControllerWrapChannelTest
    : public TVControllerTest,
      public ::testing::WithParamInterface<WrapChannelParam> {};

TEST_P(TVControllerWrapChannelTest, WrapChannelBoundary) {
    const auto& param = GetParam();
    // Given
    givenCurrentChannel(param.current);

    // When
    EXPECT_CALL(mockTuner, getCurrentCH()).WillOnce(::testing::Return(param.current));
    EXPECT_CALL(mockTuner, setCH(param.expected)).Times(1);
    controller.pushButton(param.key);

    // Then — EXPECT_CALL above
}

INSTANTIATE_TEST_SUITE_P(
    WrapChannels,
    TVControllerWrapChannelTest,
    ::testing::Values(
        WrapChannelParam{"99", remoteKey::KEY_CH_UP, "0"},
        WrapChannelParam{"0", remoteKey::KEY_CH_DOWN, "99"}
    ));

// E14: invalid getCurrentCH

TEST_F(TVControllerTest, TC_ERR_InvalidCurrentChUpNoOp) { // E14
    // Given
    ON_CALL(mockTuner, getCurrentCH()).WillByDefault(::testing::Return("abc"));

    // When
    EXPECT_CALL(mockTuner, setCH(::testing::_)).Times(0);
    controller.pushButton(remoteKey::KEY_CH_UP);

    // Then — no-op
}
