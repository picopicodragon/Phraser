#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

// Small, allocation-free waveform-similarity search for each new granular
// overlap. This avoids phase cancellation / audible pitch wobble on sustained
// periodic tones when the timeline advances at a different rate than pitch.
// This is a lightweight WSOLA-style alignment, not full spectral stretching.
namespace phraser
{
inline float grainSampleAt(const float* data, int frames, double position,
                          double low, double high) noexcept
{
    const double p = juce::jlimit(low, high, position);
    const int a = juce::jlimit(0, frames - 1, static_cast<int>(p));
    const int b = juce::jmin(a + 1, frames - 1);
    const float fraction = static_cast<float>(p - a);
    return data[a] + (data[b] - data[a]) * fraction;
}

inline double alignGrainStart(const juce::AudioBuffer<float>& audio,
                             double predicted, double priorStart,
                             double pitchIncrement, int hop, double rate,
                             double low, double high) noexcept
{
    if (audio.getNumSamples() < 2 || audio.getNumChannels() < 1 || hop < 16
        || !(pitchIncrement > 0.0) || !(rate > 0.0))
        return predicted;

    const int radius = juce::jmin(500, juce::jmax(1, static_cast<int>(rate * 0.008)));
    const int stride = juce::jmax(1, static_cast<int>(rate / 8000.0));
    const int channels = juce::jmin(2, audio.getNumChannels());
    constexpr int comparisons = 20;
    const double availableMax = juce::jmax(low, high);
    const double nominal = juce::jlimit(low, availableMax, predicted);
    double best = nominal;
    double bestError = std::numeric_limits<double>::infinity();

    for (int shift = -radius; shift <= radius; shift += stride)
    {
        const double candidate = predicted + shift;
        if (candidate < low || candidate > availableMax) continue;

        double error = 0.0;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto* data = audio.getReadPointer(ch);
            for (int j = 0; j < comparisons; ++j)
            {
                const double offset = (static_cast<double>(j) / (comparisons - 1)) * hop;
                const double oldValue = grainSampleAt(
                    data, audio.getNumSamples(),
                    priorStart + (hop + offset) * pitchIncrement, low, availableMax);
                const double newValue = grainSampleAt(
                    data, audio.getNumSamples(),
                    candidate + offset * pitchIncrement, low, availableMax);
                const double difference = oldValue - newValue;
                error += difference * difference;
            }
        }

        const double relativeShift = static_cast<double>(shift) / radius;
        error /= (channels * comparisons);
        error += 0.0005 * relativeShift * relativeShift;
        if (error < bestError)
        {
            bestError = error;
            best = candidate;
        }
    }

    return best;
}
}
