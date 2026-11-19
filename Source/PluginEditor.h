#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

// Phraser's orbit interface is custom-painted JUCE, not an embedded browser.
class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    static constexpr int controlCount = 10;
    static constexpr float orbitX = 307.0f;
    static constexpr float orbitY = 340.0f;
    void timerCallback() override;
    void refreshSampleButtons();
    void loadSlot(int);
    void paintOrbit(juce::Graphics&);
    void paintPanel(juce::Graphics&);

    PhraserAudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel_V4> theme;
    juce::TextButton generateButton { "NEW PHRASE" };
    std::array<juce::TextButton, 4> slotButtons;
    std::array<juce::TextButton, 2> keepButtons;
    std::array<juce::TextButton, 2> recallButtons;
    juce::Label status;
    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> sliderLabels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
    std::unique_ptr<juce::FileChooser> chooser;
    int cachedStep = -2;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
