// Pure layout and list helpers for the Switch-style HOME menu.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <vector>

namespace pe::ui::switch_home
{
constexpr float kCoverWidth = 340.0f;
constexpr float kCoverStep = 366.0f;
constexpr float kCarouselLeft = 184.0f;
constexpr float kCarouselRight = 1824.0f;
constexpr int kLastFullSlot = 3;

// With the default dimensions, 4 complete covers plus 176px of the fifth
// cover fit in the 1640px viewport.
constexpr float visible_fifth_fraction()
{
    return (kCarouselRight - kCarouselLeft - 4.0f * kCoverStep) / kCoverWidth;
}
constexpr float scroll_target(int selected)
{
    return static_cast<float>(std::max(0, selected - kLastFullSlot)) * kCoverStep;
}

template <typename Entry>
void append_unique(std::vector<Entry> &entries, const Entry &candidate)
{
    if (candidate.file.empty())
        return;
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const Entry &entry) {
        return entry.file == candidate.file;
    });
    if (found == entries.end())
        entries.push_back(candidate);
}
} // namespace pe::ui::switch_home
