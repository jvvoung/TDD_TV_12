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
#include <string>
#include <vector>

class TVController {
    Tuner& tuner_;
    int inputBuffer_ = -1;
    std::vector<int> favorites_;

    bool isValidChannel(int ch) const { return ch >= 0 && ch <= 99; }

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
        applyChannel(inputBuffer_);
        inputBuffer_ = -1;
    }

public:
    explicit TVController(Tuner& tuner) : tuner_(tuner) {}

    void pressNumber(int digit) {
        if (inputBuffer_ == -1) {
            inputBuffer_ = digit;
            return;
        }

        int ch = inputBuffer_ * 10 + digit;
        if (inputBuffer_ == 0) {
            ch = digit;
        }
        inputBuffer_ = -1;
        applyChannel(ch);
    }

    void pressConfirm() {
        applyBufferedDigits();
    }

    void pressOther() {
        inputBuffer_ = -1;
    }

    void pressFavorite() {
        int ch = std::stoi(tuner_.getCurrentCH());
        if (isFavorite(ch)) {
            favorites_.erase(
                std::remove(favorites_.begin(), favorites_.end(), ch),
                favorites_.end());
        } else {
            favorites_.push_back(ch);
            std::sort(favorites_.begin(), favorites_.end());
        }
    }

    void pressNextFavorite() {
        if (favorites_.empty()) {
            return;
        }
        int cur = std::stoi(tuner_.getCurrentCH());
        auto it = std::upper_bound(favorites_.begin(), favorites_.end(), cur);
        int next = (it != favorites_.end()) ? *it : favorites_.front();
        applyChannel(next);
    }

    const std::vector<int>& getFavoriteChannels() const { return favorites_; }

    void addFavorite(int ch) {
        if (!isFavorite(ch)) {
            favorites_.push_back(ch);
            std::sort(favorites_.begin(), favorites_.end());
        }
    }
};

#endif // TV_CONTROLLER_H
