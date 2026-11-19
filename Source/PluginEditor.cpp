#include "PluginEditor.h"

PhraserAudioProcessorEditor::PhraserAudioProcessorEditor(PhraserAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(520, 270);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(randomizeButton);
    addAndMakeVisible(status);
    status.setText("Load a WAV sample to begin", juce::dontSendNotification);
    status.setJustificationType(juce::Justification::centredLeft);

    configureSlider(densitySlider, densityLabel, "Density");
    configureSlider(sliceSlider, sliceLabel, "Slice Size");
    configureSlider(gainSlider, gainLabel, "Gain");

    densityAttachment = std::make_unique<SliderAttachment>(processor.parameters, "density", densitySlider);
    sliceAttachment = std::make_unique<SliderAttachment>(processor.parameters, "slice", sliceSlider);
    gainAttachment = std::make_unique<SliderAttachment>(processor.parameters, "gain", gainSlider);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>(
            "Select a WAV sample", juce::File {}, "*.wav");
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<PhraserAudioProcessorEditor>(this)](const juce::FileChooser& fc)
            {
                if (safe == nullptr)
                    return;
                const auto file = fc.getResult();
                if (file == juce::File {})
                    return;
                const auto loaded = safe->processor.loadWavFile(file);
                safe->status.setText(loaded ? "Loaded: " + file.getFileName()
                                            : "Could not read WAV",
                                     juce::dontSendNotification);
            });
    };

    randomizeButton.onClick = [this]
    {
        processor.randomizePattern();
        status.setText("New pattern generated", juce::dontSendNotification);
    };
}

void PhraserAudioProcessorEditor::configureSlider(juce::Slider& slider,
                                                  juce::Label& label,
                                                  const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void PhraserAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(24, 27, 34));
    graphics.setColour(juce::Colours::whitesmoke);
    graphics.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    graphics.drawText("PHRASER  /  MVP", 18, 14, getWidth() - 36, 36,
                      juce::Justification::centredLeft);
    graphics.setColour(juce::Colour::fromRGB(110, 120, 130));
    graphics.drawHorizontalLine(58, 18.0f, static_cast<float>(getWidth() - 18));
}

void PhraserAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    area.removeFromTop(52);
    auto buttonRow = area.removeFromTop(40);
    loadButton.setBounds(buttonRow.removeFromLeft(150).reduced(2));
    randomizeButton.setBounds(buttonRow.removeFromLeft(150).reduced(2));
    status.setBounds(area.removeFromBottom(28));

    auto controls = area.reduced(5);
    const auto width = controls.getWidth() / 3;
    for (auto* pair : { &densitySlider, &sliceSlider, &gainSlider })
    {
        auto column = controls.removeFromLeft(width);
        auto label = column.removeFromTop(24);
        if (pair == &densitySlider) densityLabel.setBounds(label);
        if (pair == &sliceSlider) sliceLabel.setBounds(label);
        if (pair == &gainSlider) gainLabel.setBounds(label);
        pair->setBounds(column);
    }
}
