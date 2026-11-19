#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour backdrop { 0xff10141b };
const juce::Colour panel { 0xff1b212c };
const juce::Colour muted { 0xff81909f };
const juce::Colour white { 0xffedf4f8 };
const std::array<juce::Colour, 4> palette {
    juce::Colour { 0xff5dd8ca }, juce::Colour { 0xffffbf71 },
    juce::Colour { 0xffaa9cf6 }, juce::Colour { 0xfff28fac }
};

class OrbitLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    OrbitLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, white);
        setColour(juce::Slider::textBoxBackgroundColourId, panel);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, muted);
        setColour(juce::TextButton::textColourOffId, white);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float start, float end,
                          juce::Slider&) override
    {
        const float radius = juce::jmin(width, height) * 0.34f;
        const auto cx = static_cast<float>(x + width / 2);
        const auto cy = static_cast<float>(y + height / 2);
        const float thickness = 5.0f;
        juce::Path track, active;
        track.addCentredArc(cx, cy, radius, radius, 0.0f, start, end, true);
        active.addCentredArc(cx, cy, radius, radius, 0.0f,
                             start, start + position * (end - start), true);
        g.setColour(juce::Colour { 0xff35404d });
        g.strokePath(track, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        g.setColour(juce::Colour { 0xff64d9c9 });
        g.strokePath(active, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour(juce::Colour { 0xff26313b });
        g.fillEllipse(cx - radius + 11.0f, cy - radius + 11.0f,
                      (radius - 11.0f) * 2.0f, (radius - 11.0f) * 2.0f);
        const float angle = start + position * (end - start);
        const float markRadius = radius - 13.0f;
        g.setColour(white);
        g.drawLine(cx + std::sin(angle) * (markRadius - 7.0f),
                   cy - std::cos(angle) * (markRadius - 7.0f),
                   cx + std::sin(angle) * markRadius,
                   cy - std::cos(angle) * markRadius, 2.2f);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& base, bool over, bool down) override
    {
        auto r = button.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(base.brighter(down ? 0.16f : (over ? 0.09f : 0.0f)));
        g.fillRoundedRectangle(r, 9.0f);
        g.setColour(juce::Colours::white.withAlpha(over ? 0.18f : 0.07f));
        g.drawRoundedRectangle(r, 9.0f, 1.0f);
    }
};
}

PhraserAudioProcessorEditor::PhraserAudioProcessorEditor(PhraserAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    theme = std::make_unique<OrbitLookAndFeel>();
    setLookAndFeel(theme.get());
    setSize(1100, 730);

    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xff1c766d });
    generateButton.onClick = [this]
    {
        processor.randomizePattern();
        repaint();
        status.setText("A new phrase has been generated", juce::dontSendNotification);
    };
    addAndMakeVisible(generateButton);

    for (int i = 0; i < 4; ++i)
    {
        auto& button = slotButtons[static_cast<size_t>(i)];
        button.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xff252d38 });
        button.onClick = [this, i] { loadSlot(i); };
        addAndMakeVisible(button);
    }
    for (int i = 0; i < 2; ++i)
    {
        auto& keep = keepButtons[static_cast<size_t>(i)];
        auto& recall = recallButtons[static_cast<size_t>(i)];
        keep.setButtonText("SAVE " + juce::String(i == 0 ? "A" : "B"));
        recall.setButtonText("RECALL " + juce::String(i == 0 ? "A" : "B"));
        keep.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xff354b59 });
        recall.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xff243b44 });
        keep.onClick = [this, i]
        {
            processor.saveKeep(i);
            status.setText("Phrase saved to " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
        };
        recall.onClick = [this, i]
        {
            processor.recallKeep(i);
            repaint();
            status.setText("Phrase recalled from " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
        };
        addAndMakeVisible(keep);
        addAndMakeVisible(recall);
    }

    constexpr std::array<const char*, controlCount> ids {
        "density", "slice", "breaks", "swing", "gain", "fade",
        "transient", "variation", "pitch", "pitchspread"
    };
    constexpr std::array<const char*, controlCount> names {
        "DENSITY", "SLICE SIZE", "BREAKS", "SWING", "OUTPUT", "FADE",
        "TRANSIENT", "VARIATION", "PITCH", "PITCH SPREAD"
    };
    for (int i = 0; i < controlCount; ++i)
    {
        auto& slider = sliders[static_cast<size_t>(i)];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 23);
        addAndMakeVisible(slider);
        auto& label = sliderLabels[static_cast<size_t>(i)];
        label.setText(names[static_cast<size_t>(i)], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(label);
        attachments[static_cast<size_t>(i)] =
            std::make_unique<SliderAttachment>(processor.parameters, ids[static_cast<size_t>(i)], slider);
    }

    status.setColour(juce::Label::textColourId, muted);
    status.setText("Load WAV samples and press NEW PHRASE. Play from Live's transport.",
                   juce::dontSendNotification);
    addAndMakeVisible(status);
    refreshSampleButtons();
    startTimerHz(24);
}

PhraserAudioProcessorEditor::~PhraserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PhraserAudioProcessorEditor::refreshSampleButtons()
{
    for (int i = 0; i < 4; ++i)
        slotButtons[static_cast<size_t>(i)].setButtonText(
            juce::String(i + 1) + "  " + processor.getSlotName(i).substring(0, 23));
}

void PhraserAudioProcessorEditor::loadSlot(int index)
{
    chooser = std::make_unique<juce::FileChooser>("Load WAV for orbit " + juce::String(index + 1),
                                                   juce::File {}, "*.wav");
    const juce::Component::SafePointer<PhraserAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
        [safe, index](const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            const auto file = fc.getResult();
            if (file == juce::File {}) return;
            const bool ok = safe->processor.loadWavFile(index, file);
            safe->refreshSampleButtons();
            safe->repaint();
            safe->status.setText(ok ? "Loaded " + file.getFileName() + " / " +
                                 juce::String(safe->processor.getTransientCount(index)) + " onsets"
                                  : "Could not load WAV",
                                  juce::dontSendNotification);
        });
}

void PhraserAudioProcessorEditor::timerCallback()
{
    const int active = processor.getCurrentStep();
    if (active != cachedStep)
    {
        cachedStep = active;
        repaint(15, 80, 585, 565);
    }
}

void PhraserAudioProcessorEditor::paintOrbit(juce::Graphics& g)
{
    const auto snapshot = processor.getPatternSnapshot();
    // Concentric 16-step bands, one band for each loaded sample.
    const float cx = orbitX;
    const float cy = orbitY;
    g.setColour(juce::Colour { 0xff19222c });
    g.fillEllipse(cx - 246.0f, cy - 246.0f, 492.0f, 492.0f);
    for (int layer = 3; layer >= 0; --layer)
    {
        const float radius = 133.0f + 29.0f * static_cast<float>(layer);
        for (int step = 0; step < 16; ++step)
        {
            const auto& event = snapshot.events[static_cast<size_t>(step)];
            const bool selected = event.enabled && event.slot == layer;
            const bool playing = cachedStep == step;
            const float a0 = static_cast<float>(step) * juce::MathConstants<float>::twoPi / 16.0f + 0.026f;
            const float a1 = static_cast<float>(step + 1) * juce::MathConstants<float>::twoPi / 16.0f - 0.026f;
            juce::Path arc;
            arc.addCentredArc(cx, cy, radius, radius, 0.0f, a0, a1, true);
            auto colour = selected ? palette[static_cast<size_t>(layer)] : juce::Colour { 0xff35404c };
            if (selected && playing) colour = colour.brighter(0.55f);
            g.setColour(colour.withAlpha(selected ? 0.98f : 0.65f));
            g.strokePath(arc, juce::PathStrokeType(playing && selected ? 17.0f : 13.0f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
            if (event.locked)
            {
                const auto mid = (a0 + a1) * 0.5f;
                const float d = radius + 12.0f;
                g.setColour(juce::Colour { 0xffffe4a0 });
                g.fillEllipse(cx + std::sin(mid) * d - 2.0f,
                              cy - std::cos(mid) * d - 2.0f, 4.0f, 4.0f);
            }
        }
    }

    g.setColour(juce::Colour { 0xff26313d });
    g.fillEllipse(cx - 100.0f, cy - 100.0f, 200.0f, 200.0f);
    g.setColour(juce::Colour { 0xff43525f });
    g.drawEllipse(cx - 100.0f, cy - 100.0f, 200.0f, 200.0f, 1.2f);
    g.setColour(white);
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    g.drawText("PHRASER", static_cast<int>(cx - 98), static_cast<int>(cy - 32),
               196, 35, juce::Justification::centred);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(cachedStep < 0 ? "READY / 16 STEPS"
                               : "STEP " + juce::String(cachedStep + 1).paddedLeft('0', 2) + " / 16",
               static_cast<int>(cx - 95), static_cast<int>(cy + 8),
               190, 24, juce::Justification::centred);
    g.setColour(muted.withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("CLICK ARC  mute / unmute",
               48, 609, 245, 20, juce::Justification::centredLeft);
    g.drawText("RIGHT CLICK  lock   |   SHIFT CLICK  change layer",
               48, 630, 465, 20, juce::Justification::centredLeft);
}

void PhraserAudioProcessorEditor::paintPanel(juce::Graphics& g)
{
    g.setColour(panel);
    g.fillRoundedRectangle(630.0f, 77.0f, 447.0f, 625.0f, 18.0f);
    g.setColour(juce::Colour { 0xff384551 });
    g.drawRoundedRectangle(630.0f, 77.0f, 447.0f, 625.0f, 18.0f, 1.0f);
    g.setColour(white);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("SHAPE / TIMING / PITCH", 653, 92, 310, 27,
               juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("GENERATIVE CONTROLS", 653, 121, 300, 17,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour { 0xff35424e });
    g.drawLine(653.0f, 460.0f, 1052.0f, 460.0f, 1.0f);
    g.setColour(white);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("SAMPLE LAYERS", 653, 472, 320, 24, juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        g.setColour(palette[static_cast<size_t>(i)]);
        g.fillRoundedRectangle(653.0f, 500.0f + 38.0f * i, 5.0f, 30.0f, 2.5f);
    }
}

void PhraserAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(backdrop);
    g.setColour(juce::Colour { 0xff17202a });
    g.fillEllipse(-155.0f, -215.0f, 610.0f, 610.0f);
    g.setColour(white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("PHRASER", 29, 18, 220, 38, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("ORBIT ENGINE  /  v0.4", 184, 29, 260, 24,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour { 0xff32414d });
    g.drawLine(28.0f, 72.0f, 1073.0f, 72.0f, 1.0f);
    paintOrbit(g);
    paintPanel(g);
}

void PhraserAudioProcessorEditor::resized()
{
    generateButton.setBounds(445, 22, 166, 38);
    const int knobWidth = 77;
    for (int i = 0; i < controlCount; ++i)
    {
        const int x = 649 + (i % 5) * 82;
        const int y = 154 + (i / 5) * 144;
        sliderLabels[static_cast<size_t>(i)].setBounds(x - 2, y, knobWidth + 6, 24);
        sliders[static_cast<size_t>(i)].setBounds(x, y + 24, knobWidth, 107);
    }
    for (int i = 0; i < 4; ++i)
        slotButtons[static_cast<size_t>(i)].setBounds(670, 500 + 38 * i, 382, 31);
    for (int i = 0; i < 2; ++i)
    {
        keepButtons[static_cast<size_t>(i)].setBounds(55 + i * 255, 662, 116, 31);
        recallButtons[static_cast<size_t>(i)].setBounds(177 + i * 255, 662, 116, 31);
    }
    status.setBounds(642, 668, 422, 25);
}

void PhraserAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    const float dx = static_cast<float>(event.x) - orbitX;
    const float dy = static_cast<float>(event.y) - orbitY;
    const float radius = std::sqrt(dx * dx + dy * dy);
    if (radius < 117.0f || radius > 236.0f) return;

    const auto layer = juce::jlimit(0, 3, static_cast<int>(std::lround((radius - 133.0f) / 29.0f)));
    float angle = std::atan2(dy, dx) + juce::MathConstants<float>::halfPi;
    if (angle < 0.0f) angle += juce::MathConstants<float>::twoPi;
    const auto step = juce::jlimit(0, 15, static_cast<int>(
        std::floor(angle * 16.0f / juce::MathConstants<float>::twoPi)));

    if (event.mods.isRightButtonDown()) processor.toggleLock(step);
    else if (event.mods.isShiftDown()) processor.setStepSlot(step, layer);
    else processor.toggleStep(step);
    repaint(15, 80, 585, 565);
}
