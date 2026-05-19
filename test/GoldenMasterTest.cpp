#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TVController.h"
#include "remoteKey.h"
#include "RecordingTuner.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string goldenRoot() {
    return "test/golden";
}

std::string goldenPath(const std::string& name) {
    return goldenRoot() + "/" + name + ".golden";
}

bool updateGoldenEnabled() {
    const char* env = std::getenv("UPDATE_GOLDEN");
    return env != nullptr && std::string(env) == "1";
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& path, const std::string& content) {
    const std::filesystem::path filePath(path);
    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out.is_open()) << "Cannot write golden file: " << path;
    out << content;
}

std::string unifiedDiff(const std::string& expected,
                        const std::string& actual,
                        const std::string& label) {
    std::istringstream expStream(expected);
    std::istringstream actStream(actual);
    std::ostringstream diff;
    diff << "--- golden (" << label << ")\n";
    diff << "+++ actual\n";

    std::string expLine;
    std::string actLine;
    int lineNo = 1;
    bool hasDiff = false;

    while (true) {
        const bool expGood = static_cast<bool>(std::getline(expStream, expLine));
        const bool actGood = static_cast<bool>(std::getline(actStream, actLine));
        if (!expGood && !actGood) {
            break;
        }
        if (expLine != actLine) {
            hasDiff = true;
            diff << "@@ line " << lineNo << " @@\n";
            if (expGood) {
                diff << "- " << expLine << '\n';
            }
            if (actGood) {
                diff << "+ " << actLine << '\n';
            }
        }
        ++lineNo;
    }

    if (!hasDiff && expected != actual) {
        diff << "@@ binary/length mismatch @@\n";
        diff << "- expected length: " << expected.size() << '\n';
        diff << "+ actual length: " << actual.size() << '\n';
    }

    return diff.str();
}

void assertMatchesGolden(const std::string& name, const std::string& actual) {
    const std::string path = goldenPath(name);
    if (updateGoldenEnabled()) {
        writeFile(path, actual);
        return;
    }

    if (!std::filesystem::exists(path)) {
        FAIL() << "Missing golden file: " << path
               << "\nRun with UPDATE_GOLDEN=1 to create it.";
    }

    const std::string expected = readFile(path);
    if (expected != actual) {
        FAIL() << "Golden mismatch for " << name << ":\n"
               << unifiedDiff(expected, actual, name);
    }
}

remoteKey digitKey(int digit) {
    static const remoteKey keys[] = {
        remoteKey::KEY_0, remoteKey::KEY_1, remoteKey::KEY_2, remoteKey::KEY_3,
        remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_7,
        remoteKey::KEY_8, remoteKey::KEY_9
    };
    return keys[digit];
}

} // namespace

class GoldenMasterTest : public ::testing::Test {
protected:
    RecordingTuner tuner;
    TVController controller{&tuner};

    void seedChannel(const std::string& ch) {
        tuner.setRecording(false);
        tuner.seedChannel(ch);
        tuner.setRecording(true);
    }

    void seedSeek(const std::vector<std::string>& sequence) {
        tuner.setRecording(false);
        tuner.seedSeekSequence(sequence);
        tuner.setRecording(true);
    }

    void pressDigit(int digit) {
        controller.pushButton(digitKey(digit));
    }

    void pressKeys(const std::vector<remoteKey>& keys) {
        for (remoteKey key : keys) {
            controller.pushButton(key);
        }
    }

    void addFavoriteAt(const std::string& ch) {
        tuner.setRecording(false);
        tuner.seedChannel(ch);
        tuner.setRecording(true);
        controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    }

    void runSearchSilent(const std::vector<std::string>& seekReturns) {
        tuner.setRecording(false);
        tuner.seedSeekSequence(seekReturns);
        tuner.setRecording(true);
        controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);
    }

    void beginRecordedAction() {
        tuner.setRecording(true);
        tuner.clearTranscript();
    }

    void assertGolden(const std::string& name) {
        assertMatchesGolden(name, tuner.transcript());
    }
};

// README scenario 1 (digit input)

TEST_F(GoldenMasterTest, Scenario_1_1_OneDigitThenOk) {
    seedChannel("0");
    beginRecordedAction();
    pressDigit(1);
    controller.pushButton(remoteKey::KEY_OK);
    assertGolden("scenario_1_1");
}

TEST_F(GoldenMasterTest, Scenario_1_2_TwoDigitsImmediate) {
    seedChannel("0");
    beginRecordedAction();
    pressDigit(1);
    pressDigit(2);
    assertGolden("scenario_1_2");
}

TEST_F(GoldenMasterTest, Scenario_1_3_FourDigitsTwelveThenThirtyFour) {
    seedChannel("0");
    beginRecordedAction();
    pressKeys({remoteKey::KEY_1, remoteKey::KEY_2, remoteKey::KEY_3, remoteKey::KEY_4});
    assertGolden("scenario_1_3");
}

TEST_F(GoldenMasterTest, Scenario_1_4a_FourFiveSixThenOk) {
    seedChannel("0");
    beginRecordedAction();
    pressKeys({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_OK});
    assertGolden("scenario_1_4a");
}

TEST_F(GoldenMasterTest, Scenario_1_4b_FourFiveSixUpInvalidatesSix) {
    seedChannel("0");
    beginRecordedAction();
    pressKeys({remoteKey::KEY_4, remoteKey::KEY_5, remoteKey::KEY_6, remoteKey::KEY_CH_UP});
    assertGolden("scenario_1_4b");
}

TEST_F(GoldenMasterTest, Scenario_1_5_ZeroSeven) {
    seedChannel("0");
    beginRecordedAction();
    pressDigit(0);
    pressDigit(7);
    assertGolden("scenario_1_5");
}

// README scenario 2 (favorite toggle)

TEST_F(GoldenMasterTest, Scenario_2_1_AddFavoriteNoSetCh) {
    seedChannel("6");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    assertGolden("scenario_2_1");
}

TEST_F(GoldenMasterTest, Scenario_2_2_RemoveFavoriteNoSetCh) {
    seedChannel("6");
    tuner.setRecording(false);
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    tuner.setRecording(true);
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    assertGolden("scenario_2_2");
}

// README scenario 3 (next favorite)

TEST_F(GoldenMasterTest, Scenario_3_1_NextFavoriteToTwelve) {
    seedChannel("6");
    tuner.setRecording(false);
    addFavoriteAt("1");
    addFavoriteAt("4");
    addFavoriteAt("12");
    addFavoriteAt("56");
    tuner.seedChannel("6");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);
    assertGolden("scenario_3_1");
}

TEST_F(GoldenMasterTest, Scenario_3_2_NextFavoriteRotateToOne) {
    seedChannel("56");
    tuner.setRecording(false);
    addFavoriteAt("1");
    addFavoriteAt("4");
    addFavoriteAt("12");
    addFavoriteAt("56");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);
    assertGolden("scenario_3_2");
}

// README scenario 4 (channel search)

TEST_F(GoldenMasterTest, Scenario_4_1_SearchUntilEmpty) {
    seedChannel("0");
    beginRecordedAction();
    seedSeek({"4", "6", "14", ""});
    controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);
    assertGolden("scenario_4_1");
}

// README scenario 5 (up/down without search)

TEST_F(GoldenMasterTest, Scenario_5_1_ChannelUpFromSix) {
    seedChannel("6");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_5_1");
}

TEST_F(GoldenMasterTest, Scenario_5_2_ChannelDownFromSix) {
    seedChannel("6");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_DOWN);
    assertGolden("scenario_5_2");
}

TEST_F(GoldenMasterTest, Scenario_5_3_NinetyNineUpWrapsToZero) {
    seedChannel("99");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_5_3");
}

TEST_F(GoldenMasterTest, Scenario_5_4_ZeroDownWrapsToNinetyNine) {
    seedChannel("0");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_DOWN);
    assertGolden("scenario_5_4");
}

// README scenario 6 (up/down with search results)

TEST_F(GoldenMasterTest, Scenario_6_1_SearchUpFromSixToFourteen) {
    seedChannel("6");
    runSearchSilent({"4", "6", "14", ""});
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_6_1");
}

TEST_F(GoldenMasterTest, Scenario_6_2_SearchDownFromSixToFour) {
    seedChannel("6");
    runSearchSilent({"4", "6", "14", ""});
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_DOWN);
    assertGolden("scenario_6_2");
}

TEST_F(GoldenMasterTest, Scenario_6_3_SearchUpFromFifteenToFour) {
    seedChannel("15");
    runSearchSilent({"4", "6", "14", ""});
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_6_3");
}

TEST_F(GoldenMasterTest, Scenario_6_4_SearchDownFromFifteenToFourteen) {
    seedChannel("15");
    runSearchSilent({"4", "6", "14", ""});
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_DOWN);
    assertGolden("scenario_6_4");
}

// Boundary scenarios A1, A2, A3, A4, A6, A7, A8

TEST_F(GoldenMasterTest, Scenario_A1_EmptyBufferOkNoOp) {
    seedChannel("0");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_OK);
    assertGolden("scenario_A1");
}

TEST_F(GoldenMasterTest, Scenario_A2_NineNineConfirm) {
    seedChannel("0");
    beginRecordedAction();
    pressDigit(9);
    pressDigit(9);
    assertGolden("scenario_A2");
}

TEST_F(GoldenMasterTest, Scenario_A3_NoFavoritesNoOp) {
    seedChannel("6");
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_NEXT_FAVORITE);
    assertGolden("scenario_A3");
}

TEST_F(GoldenMasterTest, Scenario_A4_EmptySearchThenPlusOne) {
    seedChannel("50");
    tuner.setRecording(false);
    tuner.seedSeekSequence({""});
    controller.pushButton(remoteKey::KEY_CHANNEL_SEARCH);
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_A4");
}

TEST_F(GoldenMasterTest, Scenario_A6_FavToggleClearsDigitBuffer) {
    seedChannel("6");
    tuner.setRecording(false);
    pressDigit(6);
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_FAVORITE_TOGGLE);
    assertGolden("scenario_A6");
}

TEST_F(GoldenMasterTest, Scenario_A7_OneDigitOnlyNoSetCh) {
    seedChannel("0");
    beginRecordedAction();
    pressDigit(1);
    assertGolden("scenario_A7");
}

TEST_F(GoldenMasterTest, Scenario_A8_SingleSearchResultWrap) {
    seedChannel("6");
    runSearchSilent({"6", ""});
    beginRecordedAction();
    controller.pushButton(remoteKey::KEY_CH_UP);
    assertGolden("scenario_A8");
}
