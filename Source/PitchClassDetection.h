#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

// Offline pitch-class estimator, intended for tonal WAVs only.
// Percussion, chords and mixed music can be ambiguous; no false certainty.
namespace phraser
{
struct PitchClassEstimate
{
    int pitchClass = -1; // C=0 ... B=11; -1 means unknown.
    float confidence = 0.0f;
};

inline PitchClassEstimate estimatePitchClass(const juce::AudioBuffer<float>& audio,
                                            double sampleRate)
{
    constexpr int order = 12;
    constexpr int fftSize = 1 << order;
    if (audio.getNumSamples() < fftSize || audio.getNumChannels() == 0
        || !std::isfinite(sampleRate) || sampleRate <= 0.0)
        return {};

    juce::dsp::FFT fft(order);
    std::array<float, fftSize * 2> spectrum {};
    std::array<double, 12> chroma {};
    const auto* samples = audio.getReadPointer(0);
    const int total = audio.getNumSamples();
    const int frames = 7;

    for (int frame = 0; frame < frames; ++frame)
    {
        spectrum.fill(0.0f);
        const int start = (total - fftSize) * (frame + 1) / (frames + 1);
        double energy = 0.0;
        for (int i = 0; i < fftSize; ++i)
        {
            const double window = 0.5 - 0.5 * std::cos(
                juce::MathConstants<double>::twoPi * i / (fftSize - 1));
            const float value = static_cast<float>(samples[start + i] * window);
            spectrum[static_cast<size_t>(i)] = value;
            energy += static_cast<double>(value) * value;
        }
        if (energy < 0.0001) continue;
        fft.performFrequencyOnlyForwardTransform(spectrum.data());

        const int firstBin = juce::jmax(1, static_cast<int>(std::ceil(90.0 * fftSize / sampleRate)));
        const int finalBin = juce::jmin(fftSize / 2 - 1,
                                       static_cast<int>(1400.0 * fftSize / sampleRate));
        if (finalBin <= firstBin + 2) continue;
        double top = 0.0;
        for (int bin = firstBin; bin <= finalBin; ++bin)
            top = juce::jmax(top, static_cast<double>(spectrum[static_cast<size_t>(bin)]));
        if (top < 0.001) continue;

        for (int bin = firstBin + 1; bin < finalBin; ++bin)
        {
            const double magnitude = spectrum[static_cast<size_t>(bin)];
            if (magnitude < top * 0.13
                || magnitude <= spectrum[static_cast<size_t>(bin - 1)]
                || magnitude < spectrum[static_cast<size_t>(bin + 1)])
                continue;
            const double frequency = bin * sampleRate / fftSize;
            const double semitones = 69.0 + 12.0 * std::log2(frequency / 440.0);
            const int note = static_cast<int>(std::round(semitones));
            const int key = (note % 12 + 12) % 12;
            chroma[static_cast<size_t>(key)] += magnitude / top
                / (1.0 + std::abs(semitones - note));
        }
    }
    int best = -1;
    double bestScore = 0.0, second = 0.0, sum = 0.0;
    for (int key = 0; key < 12; ++key)
    {
        const double value = chroma[static_cast<size_t>(key)];
        sum += value;
        if (value > bestScore)
        {
            second = bestScore;
            bestScore = value;
            best = key;
        }
        else second = juce::jmax(second, value);
    }
    // Only decide when one pitch class dominates clearly.
    const float confidence = sum > 0.0
        ? static_cast<float>(bestScore / sum) : 0.0f;
    if (bestScore < 1.3 || confidence < 0.34f
        || second > bestScore * 0.77)
        return {};
    return { best, confidence };
}
} // namespace phraser
