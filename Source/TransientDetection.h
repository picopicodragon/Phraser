#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <vector>

// Lightweight energy-flux onset approximation; run only on file load, never in processBlock.
namespace phraser
{
inline std::vector<float> detectTransients(const juce::AudioBuffer<float>& audio,
                                          double sampleRate)
{
    std::vector<float> locations { 0.0f };
    const int total = audio.getNumSamples();
    if (total < 2048 || sampleRate <= 0.0) return locations;

    const int hop = 256;
    const int window = 1024;
    const int frames = 1 + (total - window) / hop;
    std::vector<double> energy(static_cast<size_t>(frames), 0.0);
    for (int f = 0; f < frames; ++f)
    {
        const int start = f * hop;
        double sum = 0.0;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            const auto* samples = audio.getReadPointer(channel, start);
            for (int i = 0; i < window; i += 4)
                sum += static_cast<double>(samples[i]) * samples[i];
        }
        energy[static_cast<size_t>(f)] = std::sqrt(sum / (window / 4 * audio.getNumChannels()));
    }
    // Relative change is more useful than absolute volume for diverse material.
    std::vector<double> novelty(energy.size(), 0.0);
    for (size_t f = 1; f < energy.size(); ++f)
        novelty[f] = juce::jmax(0.0, energy[f] - energy[f - 1]);

    const int minimumGap = juce::jmax(1, static_cast<int>(sampleRate * 0.055 / hop));
    int lastOnset = -minimumGap;
    for (int f = 2; f < frames - 2; ++f)
    {
        const auto idx = static_cast<size_t>(f);
        double localMean = 0.0;
        int count = 0;
        for (int j = juce::jmax(0, f - 12); j <= juce::jmin(frames - 1, f + 12); ++j)
        {
            localMean += novelty[static_cast<size_t>(j)];
            ++count;
        }
        localMean /= count;
        if (f - lastOnset < minimumGap
            || novelty[idx] < 0.005
            || novelty[idx] < 1.7 * localMean
            || novelty[idx] < novelty[idx - 1]
            || novelty[idx] < novelty[idx + 1])
            continue;

        locations.push_back(juce::jlimit(0.0f, 0.99f,
            static_cast<float>(f * hop) / static_cast<float>(total)));
        lastOnset = f;
        if (locations.size() >= 512) break;
    }
    return locations;
}
} // namespace phraser
