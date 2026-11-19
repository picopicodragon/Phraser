#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Offline onset-interval tempo estimator. Not a key/time-stretch engine.
// Returns 0 when the source doesn't contain enough rhythmic evidence.
// Half/double time may still be ambiguous; the user can override BPM per slot.
namespace phraser
{
inline double estimateSampleBpm(const std::vector<float>& onsetPositions,
                                int totalSamples, double sampleRate)
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0
        || totalSamples <= 0 || onsetPositions.size() < 5)
        return 0.0;
    const double duration = static_cast<double>(totalSamples) / sampleRate;
    if (!std::isfinite(duration) || duration < 1.5) return 0.0;

    // Pairwise onset spacings are compared with candidate beat durations.
    // 0.5, 1, 2, and 4 beat intervals are permitted, with nearest neighbors weighted most.
    constexpr int minBpm = 70;
    constexpr int maxBpm = 180;
    // Indexed directly by BPM. The old 161-element array was written at
    // indices 161..180 and corrupted the stack during WAV loading.
    std::array<double, maxBpm + 1> scores {};
    for (size_t i = 1; i < onsetPositions.size(); ++i)
    {
        for (size_t j = i + 1; j < std::min(onsetPositions.size(), i + 9); ++j)
        {
            const double delta = (onsetPositions[j] - onsetPositions[i]) * duration;
            if (!std::isfinite(delta) || delta < 0.085 || delta > 4.0) continue;
            for (int bpm = minBpm; bpm <= maxBpm; ++bpm)
            {
                const double beat = 60.0 / bpm;
                double nearest = 0.0;
                for (const double multiple : { 0.5, 1.0, 2.0, 4.0 })
                {
                    const double error = std::abs(delta - beat * multiple);
                    const double confidence = juce::jmax(0.0, 1.0 - error / (beat * 0.11));
                    nearest = juce::jmax(nearest, confidence);
                }
                scores[static_cast<size_t>(bpm)] += nearest / std::sqrt(static_cast<double>(j - i));
            }
        }
    }
    int bestBpm = 0;
    double bestScore = 0.0;
    for (int bpm = 70; bpm <= 180; ++bpm)
    {
        if (scores[static_cast<size_t>(bpm)] > bestScore)
        {
            bestBpm = bpm;
            bestScore = scores[static_cast<size_t>(bpm)];
        }
    }
    // Require several reasonably aligned onset intervals.
    return bestScore >= 3.0 ? static_cast<double>(bestBpm) : 0.0;
}
}
