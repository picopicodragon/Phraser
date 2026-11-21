#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class SampleSlotButton final : public juce::TextButton
{
public:
    int sampleSlot = -1;
    bool dragStarted = false;
    std::function<void()> onContextMenu;
    void mouseDown(const juce::MouseEvent& event) override
    {
        dragStarted = false;
        juce::TextButton::mouseDown(event);
    }
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (event.mods.isRightButtonDown() && onContextMenu)
        {
            onContextMenu();
            return;
        }
        juce::TextButton::mouseUp(event);
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!dragStarted && event.getDistanceFromDragStart() > 7)
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                dragStarted = true;
                container->startDragging("PhraserSample:" + juce::String(sampleSlot), this);
            }
        juce::TextButton::mouseDrag(event);
    }
};

class SampleLevelSlider final : public juce::Slider
{
public:
    float meter = 0.0f;

    void paint(juce::Graphics& g) override
    {
        // One integrated bar: light background, volume fill, translucent
        // live peak level over the same track and a white volume marker.
        // Slider still handles all input/gestures and automation normally.
        const auto bounds = getLocalBounds().toFloat();
        const auto track = juce::Rectangle<float>(
            3.0f, 4.0f, juce::jmax(20.0f, bounds.getWidth() - 36.0f),
            juce::jmax(10.0f, bounds.getHeight() - 8.0f));
        const float volume = juce::jlimit(0.0f, 1.0f,
            static_cast<float>(getValue() / 1.5));
        const float peak = juce::jlimit(0.0f, 1.0f, meter);

        g.setColour(juce::Colour { 0xfff8fbf8 });
        g.fillRoundedRectangle(track, 3.0f);
        g.setColour(juce::Colour { 0xffc6d7cb });
        g.drawRoundedRectangle(track, 3.0f, 1.0f);

        g.saveState();
        g.reduceClipRegion(track.toNearestInt());
        g.setColour(juce::Colour { 0xff98b7a6 });
        g.fillRect(track.withWidth(track.getWidth() * volume));
        g.setColour(juce::Colour { 0xff24b774 }.withAlpha(0.68f));
        g.fillRect(track.withWidth(track.getWidth() * peak));
        g.restoreState();

        const float markerX = track.getX() + track.getWidth() * volume;
        g.setColour(juce::Colour { 0xff193e2f });
        g.drawLine(markerX, track.getY() - 1.0f,
                   markerX, track.getBottom() + 1.0f, 2.0f);

        g.setColour(juce::Colour { 0xff1e3029 });
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(juce::String(juce::roundToInt(getValue() * 100.0)) + "%",
            track.getRight() + 4.0f, 2.0f, 31.0f,
            bounds.getHeight() - 4.0f, juce::Justification::centredRight);
    }
};

class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          public juce::DragAndDropTarget,
                                          public juce::FileDragAndDropTarget,
                                          private juce::Timer
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int x, int y) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    static constexpr int controlCount = 12;
    static constexpr float waveLeft = 372.0f;
    static constexpr float waveWidth = 314.0f;
    static constexpr float laneTop = 102.0f;
    static constexpr float lanePitch = 30.0f;

    PhraserAudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel_V4> theme;
    juce::TextButton generateButton { "GENERATE" };
    std::array<SampleSlotButton, PhraserAudioProcessor::numSlots> slotButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> modeButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> slotMuteButtons;
    std::array<SampleLevelSlider, PhraserAudioProcessor::numSlots> slotVolumeSliders;
    std::array<juce::TextButton, 4> trackMuteButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> syncButtons;
    std::array<juce::Label, PhraserAudioProcessor::numSlots> tempoLabels;
    std::array<juce::Label, PhraserAudioProcessor::numSlots> pitchLabels;
    std::array<juce::TextButton, 2> saveButtons, recallButtons;
    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> sliderLabels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
    juce::Label status;
    juce::ComboBox referenceKeyBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> referenceKeyAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    int selectedSlot = 0;
    int dragSlot = -1;
    float dragAnchor = 0.0f;
    bool dragDuration = false;
    int lastStep = -2;
    int lastColumns = -1;
    int lastTracks = -1;
    int lastReferenceKey = -1;
    int lastBars = -1;

    void timerCallback() override;
    void updateSlotControls();
    void loadSlot(int index);
    void paintLanes(juce::Graphics&);
    void paintSequence(juce::Graphics&);
    void paintOverlapOrbit(juce::Graphics&);
    void applyWaveDrag(const juce::MouseEvent&);
    int getCellAt(juce::Point<int>) const;
    void importSample(int slot, const juce::File&);
    void showCellAssignmentMenu(int cell, juce::Point<int> screen);
    void showSampleKeyMenu(int slot);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
