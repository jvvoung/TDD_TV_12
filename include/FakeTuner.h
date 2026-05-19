/**
 * Copyright 2020 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#ifndef FAKE_TUNER_H
#define FAKE_TUNER_H

#include "Tuner.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

class FakeTuner : public Tuner {
    int current_ = 0;
    std::vector<int> available_;

public:
    explicit FakeTuner(std::vector<int> avail)
        : available_(std::move(avail)) {}

    std::string seekCH() override {
        if (available_.empty()) {
            throw std::runtime_error("시청 가능 채널 목록이 비어 있습니다.");
        }
        auto it = std::find_if(
            available_.begin(), available_.end(),
            [&](int ch) { return ch > current_; });
        current_ = (it != available_.end()) ? *it : available_.front();
        return std::to_string(current_);
    }

    void setCH(const std::string& ch) override {
        int v = std::stoi(ch);
        if (v < 0 || v > 99) {
            throw std::invalid_argument("채널 범위 초과: " + ch);
        }
        current_ = v;
    }

    std::string getCurrentCH() override {
        return std::to_string(current_);
    }
};

#endif // FAKE_TUNER_H
