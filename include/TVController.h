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
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

class TVController {
    Tuner& tuner_;
    int inputBuffer_ = -1;              // -1: 입력 버퍼가 비어있는 상태
    std::vector<int> favorites_;        // 선호 채널 목록은 항상 오름차순 유지

    bool isValidDigit(int digit) const {
        return digit >= 0 && digit <= 9;
    }

    bool isValidChannel(int ch) const {
        return ch >= 0 && ch <= 99;
    }

    bool isFavorite(int ch) const {
        return std::find(favorites_.begin(), favorites_.end(), ch) != favorites_.end();
    }

    void applyChannel(int ch) {
        if (!isValidChannel(ch)) {
            throw std::invalid_argument("Invalid channel");
        }
        tuner_.setCH(std::to_string(ch));
    }

    void applyBufferedDigits() {
        if (inputBuffer_ == -1) {
            return;
        }

        int ch = inputBuffer_;
        inputBuffer_ = -1;
        applyChannel(ch);
    }

public:
    explicit TVController(Tuner& tuner) : tuner_(tuner) {}

    // 숫자 버튼: 첫 자리 저장, 두 번째 자리 입력 시 자동 채널 변경
    void pressNumber(int digit) {
        if (!isValidDigit(digit)) {
            throw std::invalid_argument("Invalid digit");
        }

        if (inputBuffer_ == -1) {
            inputBuffer_ = digit;
            return;
        }

        int ch = inputBuffer_ * 10 + digit;
        if (inputBuffer_ == 0) {
            ch = digit;                 // 0,7 입력은 07이 아니라 7번 채널
        }

        inputBuffer_ = -1;
        applyChannel(ch);
    }

    // 확인 버튼: 한 자리 입력이 남아 있을 때 해당 채널로 이동
    void pressConfirm() {
        applyBufferedDigits();
    }

    // 숫자/확인이 아닌 기타 버튼: 입력 버퍼 무효화
    void pressOther() {
        inputBuffer_ = -1;
    }

    // 선호 채널 버튼: 현재 채널을 토글 방식으로 추가/삭제
    void pressFavorite() {
        int ch = std::stoi(tuner_.getCurrentCH());
        if (!isValidChannel(ch)) {
            throw std::invalid_argument("Invalid current channel");
        }

        if (isFavorite(ch)) {
            favorites_.erase(
                std::remove(favorites_.begin(), favorites_.end(), ch),
                favorites_.end());
            return;
        }

        favorites_.push_back(ch);
        std::sort(favorites_.begin(), favorites_.end());
    }

    // 다음 선호 채널: 현재 채널보다 큰 값 중 가장 작은 채널, 없으면 첫 채널로 wrap-around
    void pressNextFavorite() {
        if (favorites_.empty()) {
            return;
        }

        int cur = std::stoi(tuner_.getCurrentCH());
        if (!isValidChannel(cur)) {
            throw std::invalid_argument("Invalid current channel");
        }

        auto it = std::upper_bound(favorites_.begin(), favorites_.end(), cur);
        int next = (it != favorites_.end()) ? *it : favorites_.front();
        applyChannel(next);
    }

    const std::vector<int>& getFavoriteChannels() const {
        return favorites_;
    }

    // 테스트 준비용 헬퍼: 선호 채널을 중복 없이 추가하고 정렬 유지
    void addFavorite(int ch) {
        if (!isValidChannel(ch)) {
            throw std::invalid_argument("Invalid favorite channel");
        }

        if (!isFavorite(ch)) {
            favorites_.push_back(ch);
            std::sort(favorites_.begin(), favorites_.end());
        }
    }
};

#endif // TV_CONTROLLER_H
