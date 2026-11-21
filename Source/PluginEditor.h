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
        juce::Slider::paint(g);
        const auto area = getLocalBounds().toFloat().reduced(4.0f, 5.0f);
        const float fraction = juce::jlimit(0.0f, 1.0f, meter);
        g.setColour(juce::Colour { 0xff21a878 }.withAlpha(0.50f));
        g.fillRect(area.withWidth(area.getWidth() * fraction)
                  .withHeight(3.0f).withY(area.getBottom() - 4.0f));
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
