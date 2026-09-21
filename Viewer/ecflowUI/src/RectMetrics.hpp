// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

class RectMetrics {
public:
    explicit RectMetrics(int penWidth);
    int topOffset() const { return topOffset_; }
    int bottomOffset() const { return bottomOffset_; }

protected:
    int topOffset_;
    int bottomOffset_;

private:
    void compute(int);
};
