/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef TV_CONTROLLER_H
#define TV_CONTROLLER_H

#include "Tuner.h"
#include "remoteKey.h"
#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

class TVController {
private:
    static constexpr int MIN_CHANNEL = 0;
    static constexpr int MAX_CHANNEL = 99;
    static constexpr std::size_t MAX_DIGIT_BUFFER = 2;
    static constexpr std::size_t SINGLE_DIGIT_LENGTH = 1;
    static constexpr int SEEK_LOOP_GUARD = MAX_CHANNEL + 1;
    static constexpr const char* CHANNEL_ZERO = "0";

    Tuner* tuner;
    std::string processingCH;
    std::set<int> favoriteChannels;
    std::vector<int> searchResults;

    static bool isDigitKey(remoteKey key) {
        return key >= remoteKey::KEY_0 && key <= remoteKey::KEY_9;
    }

    static std::optional<int> parseChannelValue(const std::string& ch) {
        if (ch.empty()) {
            return std::nullopt;
        }
        try {
            int value = std::stoi(ch);
            if (value < MIN_CHANNEL || value > MAX_CHANNEL) {
                return std::nullopt;
            }
            return value;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    std::optional<int> getCurrentChannelValue() const {
        return parseChannelValue(tuner->getCurrentCH());
    }

    void setTunerCh(const std::string& ch) {
        const auto value = parseChannelValue(ch);
        if (!value) {
            processingCH.clear();
            return;
        }
        const std::string normalized = std::to_string(*value);
        // 로그는 테스트의 결과가 절대 아닙니다. 로그가 있는 것을 테스트로 간주하지 마시기 바랍니다.
        std::cout << "현재 설정하는 채널 : " << normalized << std::endl;
        tuner->setCH(normalized);
        processingCH.clear();
    }

    void clearDigitBuffer() {
        processingCH.clear();
    }

    // 미확정 숫자 버퍼가 있으면 버리고 true. CH_UP/DOWN은 true일 때 튜닝하지 않음.
    bool discardDigitBufferIfNonEmpty() {
        if (processingCH.empty()) {
            return false;
        }
        processingCH.clear();
        return true;
    }

    void confirmBuffer() {
        if (processingCH.empty()) {
            return;
        }
        setTunerCh(processingCH);
    }

    bool hasSingleDigitInBuffer() const {
        return processingCH.size() == SINGLE_DIGIT_LENGTH;
    }

    bool hasLeadingZeroInBuffer() const {
        return hasSingleDigitInBuffer() && processingCH == CHANNEL_ZERO;
    }

    void startDigitEntry(const std::string& digit) {
        processingCH = digit;
    }

    void appendSecondDigitAndConfirm(const std::string& digit) {
        processingCH += digit;
        confirmBuffer();
    }

    void replaceLeadingZeroAndConfirm(const std::string& digit) {
        processingCH = digit;
        confirmBuffer();
    }

    void restartDigitEntry(const std::string& digit) {
        processingCH = digit;
    }

    void handleDigit(remoteKey key) {
        const std::string digit = to_string(key);
        if (processingCH.empty()) {
            startDigitEntry(digit);
            return;
        }
        if (hasSingleDigitInBuffer()) {
            if (hasLeadingZeroInBuffer()) {
                replaceLeadingZeroAndConfirm(digit);
                return;
            }
            appendSecondDigitAndConfirm(digit);
            return;
        }
        restartDigitEntry(digit);
    }

    void handleConfirm() {
        if (hasSingleDigitInBuffer()) {
            confirmBuffer();
        }
    }

    void toggleFavorite() {
        const auto current = getCurrentChannelValue();
        if (!current) {
            return;
        }
        if (favoriteChannels.count(*current) > 0) {
            favoriteChannels.erase(*current);
        } else {
            favoriteChannels.insert(*current);
        }
    }

    void nextFavorite() {
        if (favoriteChannels.empty()) {
            return;
        }
        const auto current = getCurrentChannelValue();
        if (!current) {
            return;
        }
        auto it = favoriteChannels.upper_bound(*current);
        if (it == favoriteChannels.end()) {
            setTunerCh(std::to_string(*favoriteChannels.begin()));
        } else {
            setTunerCh(std::to_string(*it));
        }
    }

    void runChannelSearch() {
        searchResults.clear();
        std::vector<int> found;
        // seekCH()는 빈 문자열 반환 시 종료; 최대 SEEK_LOOP_GUARD회 가드
        for (int i = 0; i < SEEK_LOOP_GUARD; ++i) {
            const std::string ch = tuner->seekCH();
            if (ch.empty()) {
                break;
            }
            const auto value = parseChannelValue(ch);
            if (value) {
                found.push_back(*value);
            }
        }
        std::sort(found.begin(), found.end());
        found.erase(std::unique(found.begin(), found.end()), found.end());
        searchResults = found;
    }

    static int wrapChannel(int ch) {
        if (ch > MAX_CHANNEL) {
            return MIN_CHANNEL;
        }
        if (ch < MIN_CHANNEL) {
            return MAX_CHANNEL;
        }
        return ch;
    }

    int findNextSearchChannel(int current) const {
        for (const int ch : searchResults) {
            if (ch > current) {
                return ch;
            }
        }
        return searchResults.front();
    }

    int findPrevSearchChannel(int current) const {
        int candidate = -1;
        for (const int ch : searchResults) {
            if (ch < current) {
                candidate = ch;
            }
        }
        if (candidate >= 0) {
            return candidate;
        }
        return searchResults.back();
    }

    void tuneToChannelValue(int channel) {
        setTunerCh(std::to_string(channel));
    }

    void adjustChannelStep(int delta) {
        const auto current = getCurrentChannelValue();
        if (!current) {
            return;
        }
        if (searchResults.empty()) {
            tuneToChannelValue(wrapChannel(*current + delta));
            return;
        }
        if (delta > 0) {
            tuneToChannelValue(findNextSearchChannel(*current));
        } else {
            tuneToChannelValue(findPrevSearchChannel(*current));
        }
    }

    void channelUp() {
        adjustChannelStep(1);
    }

    void channelDown() {
        adjustChannelStep(-1);
    }

public:
    explicit TVController(Tuner* tuner) : tuner(tuner), processingCH("") {}

    void pushButton(remoteKey key) {
        if (isDigitKey(key)) {
            handleDigit(key);
            return;
        }

        switch (key) {
            case remoteKey::KEY_OK:
                handleConfirm();
                break;
            case remoteKey::KEY_FAVORITE_TOGGLE:
                clearDigitBuffer();
                toggleFavorite();
                break;
            case remoteKey::KEY_NEXT_FAVORITE:
                clearDigitBuffer();
                nextFavorite();
                break;
            case remoteKey::KEY_CHANNEL_SEARCH:
                clearDigitBuffer();
                runChannelSearch();
                break;
            case remoteKey::KEY_CH_UP:
                if (discardDigitBufferIfNonEmpty()) {
                    return;
                }
                channelUp();
                break;
            case remoteKey::KEY_CH_DOWN:
                if (discardDigitBufferIfNonEmpty()) {
                    return;
                }
                channelDown();
                break;
            default:
                break;
        }
    }
};

#endif // TV_CONTROLLER_H
