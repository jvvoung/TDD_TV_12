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

    static std::string normalizeChannelString(const std::string& ch) {
        auto value = parseChannelValue(ch);
        if (!value) {
            return "";
        }
        return std::to_string(*value);
    }

    bool isValidChannelString(const std::string& ch) const {
        return parseChannelValue(ch).has_value();
    }

    void setTunerCh(const std::string& ch) {
        const std::string normalized = normalizeChannelString(ch);
        if (!isValidChannelString(normalized)) {
            processingCH.clear();
            return;
        }
        // 로그는 테스트의 결과가 절대 아닙니다. 로그가 있는 것을 테스트로 간주하지 마시기 바랍니다.
        std::cout << "현재 설정하는 채널 : " << normalized << std::endl;
        tuner->setCH(normalized);
        processingCH.clear();
    }

    bool clearBufferIfAny() {
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

    void handleDigit(remoteKey key) {
        const std::string digit = to_string(key);
        if (processingCH.empty()) {
            processingCH = digit;
            return;
        }
        if (processingCH.size() == 1) {
            if (processingCH == "0") {
                processingCH = digit;
                confirmBuffer();
                return;
            }
            processingCH += digit;
            confirmBuffer();
            return;
        }
        processingCH = digit;
    }

    void handleConfirm() {
        if (processingCH.size() == 1) {
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
        for (int i = 0; i <= MAX_CHANNEL; ++i) {
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

    void channelUp() {
        const auto current = getCurrentChannelValue();
        if (!current) {
            return;
        }
        if (searchResults.empty()) {
            setTunerCh(std::to_string(wrapChannel(*current + 1)));
            return;
        }
        setTunerCh(std::to_string(findNextSearchChannel(*current)));
    }

    void channelDown() {
        const auto current = getCurrentChannelValue();
        if (!current) {
            return;
        }
        if (searchResults.empty()) {
            setTunerCh(std::to_string(wrapChannel(*current - 1)));
            return;
        }
        setTunerCh(std::to_string(findPrevSearchChannel(*current)));
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
                clearBufferIfAny();
                toggleFavorite();
                break;
            case remoteKey::KEY_NEXT_FAVORITE:
                clearBufferIfAny();
                nextFavorite();
                break;
            case remoteKey::KEY_CHANNEL_SEARCH:
                clearBufferIfAny();
                runChannelSearch();
                break;
            case remoteKey::KEY_CH_UP:
                if (clearBufferIfAny()) {
                    return;
                }
                channelUp();
                break;
            case remoteKey::KEY_CH_DOWN:
                if (clearBufferIfAny()) {
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
