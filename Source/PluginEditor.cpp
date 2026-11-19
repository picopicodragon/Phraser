#include "PluginEditor.h"

PhraserAudioProcessorEditor::PhraserAudioProcessorEditor(PhraserAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(950, 450);

    addAndMakeVisible(randomizeButton);
    randomizeButton.onClick = [this]
    {
        processor.randomizePattern();
        refreshButtons();
        status.setText("New phrase generated", juce::dontSendNotification);
    };

    addAndMakeVisible(status);
    status.setJustificationType(juce::Justification::centredLeft);
    status.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    status.setText("Load up to 4 WAVs. Start the Live transport to play.",
                   juce::dontSendNotification);

    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        auto& button = slotButtons[static_cast<size_t>(i)];
        button.onClick = [this, i] { loadSlot(i); };
        addAndMakeVisible(button);
    }

    for (int i = 0; i < PhraserAudioProcessor::numSteps; ++i)
    {
        auto& step = stepButtons[static_cast<size_t>(i)];
        step.onClick = [this, i]
        {
            processor.toggleStep(i);
            refreshButtons();
        };
        addAndMakeVisible(step);

        auto& lock = lockButtons[static_cast<size_t>(i)];
        lock.onClick = [this, i]
        {
            processor.toggleLock(i);
            refreshButtons();
        };
        addAndMakeVisible(lock);
    }

    const char* ids[] = { "density", "slice", "breaks", "swing", "gain", "fade", "transient", "variation" };
    const char* titles[] = { "DENSITY", "SIZE", "BREAKS", "SWING", "GAIN", "FADE", "SNAP", "VARIATION" };
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        auto& slider = sliders[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 20);
        addAndMakeVisible(slider);

        auto& label = sliderLabels[i];
        label.setText(titles[i], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        attachments[i] = std::make_unique<SliderAttachment>(processor.parameters, ids[i], slider);
    }

    refreshButtons();
}

void PhraserAudioProcessorEditor::loadSlot(int index)
{
    chooser = std::make_unique<juce::FileChooser>(
        "Load sample for slot " + juce::String(index + 1), juce::File {}, "*.wav");
    const juce::Component::SafePointer<PhraserAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
        [safe, index](const juce::FileChooser& selected)
        {
            if (safe == nullptr) return;
            const auto file = selected.getResult();
            if (file == juce::File {}) return;

            const bool loaded = safe->processor.loadWavFile(index, file);
            safe->status.setText(loaded ? "Loaded: " + file.getFileName() + " / " + juce::String(safe->processor.getTransientCount(index)) + " onset marks"
                                        : "Could not load the selected WAV",
                                 juce::dontSendNotification);
            safe->refreshButtons();
        });
}

void PhraserAudioProcessorEditor::refreshButtons()
{
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        const auto name = processor.getSlotName(i);
        slotButtons[static_cast<size_t>(i)].setButtonText(
            juce::String(i + 1) + "  " + name.substring(0, 18));
    }

    const auto snapshot = processor.getPatternSnapshot();
    for (int i = 0; i < PhraserAudioProcessor::numSteps; ++i)
    {
        const auto& e = snapshot.events[static_cast<size_t>(i)];
        auto& step = stepButtons[static_cast<size_t>(i)];
        step.setButtonText(juce::String(i + 1) + (e.enabled ? "" : " -"));
        step.setColour(juce::TextButton::buttonColourId,
                       e.enabled ? juce::Colour::fromRGB(43, 142, 133)
                                 : juce::Colour::fromRGB(53, 56, 64));
        auto& lock = lockButtons[static_cast<size_t>(i)];
        lock.setButtonText(e.locked ? "LOCK" : "free");
        lock.setColour(juce::TextButton::buttonColourId,
                       e.locked ? juce::Colour::fromRGB(150, 95, 51)
                                : juce::Colour::fromRGB(44, 47, 54));
    }
}

void PhraserAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(20, 23, 30));
    g.setColour(juce::Colours::whitesmoke);
    g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    g.drawText("PHRASER", 22, 14, 380, 36, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(130, 145, 154));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("4 SAMPLE SLOTS / 16-STEP PHRASE ENGINE v0.3", 22, 47, 410, 20,
               juce::Justification::centredLeft);
    g.drawText("STEPS: click to toggle, LOCK: preserve across generation",
               22, 290, getWidth() - 44, 18, juce::Justification::centredLeft);
    g.drawHorizontalLine(74, 22.0f, static_cast<float>(getWidth() - 22));
}

void PhraserAudioProcessorEditor::resized()
{
    const int margin = 22;
    randomizeButton.setBounds(getWidth() - margin - 155, 20, 155, 40);

    const int slotY = 86;
    const int slotGap = 8;
    const int slotWidth = (getWidth() - 2 * margin - slotGap * 3) / 4;
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
        slotButtons[static_cast<size_t>(i)].setBounds(
            margin + i * (slotWidth + slotGap), slotY, slotWidth, 35);

    const int knobsY = 134;
    const int knobWidth = (getWidth() - 2 * margin) / static_cast<int>(sliders.size());
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        const int x = margin + static_cast<int>(i) * knobWidth;
        sliderLabels[i].setBounds(x, knobsY, knobWidth, 21);
        sliders[i].setBounds(x + 3, knobsY + 21, knobWidth - 6, 119);
    }

    const int stepY = 312;
    const int stepGap = 3;
    const int stepWidth = (getWidth() - 2 * margin - stepGap * 15) / 16;
    for (int i = 0; i < PhraserAudioProcessor::numSteps; ++i)
    {
        const int x = margin + i * (stepWidth + stepGap);
        stepButtons[static_cast<size_t>(i)].setBounds(x, stepY, stepWidth, 36);
        lockButtons[static_cast<size_t>(i)].setBounds(x, stepY + 39, stepWidth, 25);
    }
    status.setBounds(margin, getHeight() - 29, getWidth() - 2 * margin, 23);
}
