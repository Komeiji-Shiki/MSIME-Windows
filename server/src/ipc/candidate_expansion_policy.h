#pragma once

#include "global/globals.h"
#include "ipc/candidate_selection_policy.h"

#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

namespace FanyImeIpc
{
// Session expansion rebuilds local candidates; asynchronous results only live in the UI list.
// Quick phrases are rebuilt by the caller, so carry over only asynchronous sources, deduplicating
// against the expanded list before applying the same ordering policy as worker callbacks.
inline void SetExpandedCandidatesKeepingPagePosition(Global::CandidateUiState &ui, std::vector<WordItem> expanded,
                                                     const EnglishPlacement &english_placement = {})
{
    if (ui.selection_started)
    {
        // 翻页扩展只追加尚未显示的词。引擎可能已经有新的重排结果，不能挪动用户正在选的项。
        std::unordered_set<std::string> seen;
        for (const auto &candidate : ui.items)
            seen.insert(candidate.word);
        for (auto &candidate : expanded)
            if (seen.insert(candidate.word).second)
                ui.items.push_back(std::move(candidate));
        ui.item_total_count = static_cast<int>(ui.items.size());
        ui.clear_page();
        return;
    }
    for (const auto &candidate : ui.items)
    {
        switch (candidate.source)
        {
        case CandidateSource::CloudSuggestion:
        case CandidateSource::AiSuggestion:
        case CandidateSource::EnglishDictionary:
        case CandidateSource::Emoji:
        case CandidateSource::Kaomoji:
        case CandidateSource::DateTime:
            if (std::none_of(expanded.begin(), expanded.end(),
                             [&](const WordItem &item) { return item.word == candidate.word; }))
                expanded.push_back(candidate);
            break;
        default:
            break;
        }
    }
    NormalizeMixedCandidateOrder(expanded, 1, english_placement);
    const int current_page = ui.page_index;
    const int current_selection = ui.selected_index_in_page;
    ui.set_items(std::move(expanded));
    ui.page_index = current_page;
    ui.selected_index_in_page = current_selection;
}
} // namespace FanyImeIpc
