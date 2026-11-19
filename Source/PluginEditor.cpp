#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour ink { 0xffedf2ef };
const juce::Colour surface { 0xfffcfdfb };
const juce::Colour inset { 0xffe3ebe6 };
const juce::Colour line { 0xffcad6ce };
const juce::Colour muted { 0xff60776e };
const juce::Colour bright { 0xff1e3029 };
const juce::Colour acid { 0xff238b60 };
const juce::Colour amber { 0xffc97939 };

juce::Colour trackColour(int slot)
{
    // Four related colour families provide separation without a rainbow UI.
    const std::array<juce::Colour, 4> families {
        juce::Colour { 0xff167d77 }, juce::Colour { 0xffb47635 },
        juce::Colour { 0xff675cb5 }, juce::Colour { 0xffb45078 }
    };
    return families[static_cast<size_t>(slot / 4)].interpolatedWith(bright, (slot % 4) * 0.055f);
}

class StudioLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    StudioLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, bright);
        setColour(juce::Slider::textBoxBackgroundColourId, inset);
        setColour(juce::Slider::textBoxOutlineColourId, line);
        setColour(juce::Label::textColourId, muted);
        setColour(juce::TextButton::textColourOffId, bright);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        const float cx = static_cast<float>(x + width / 2);
        const float cy = static_cast<float>(y + height / 2);
        const float radius = juce::jmin(width, height) * 0.35f;
        juce::Path track, amount;
        track.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
        amount.addCentredArc(cx, cy, radius, radius, 0.0f,
                            startAngle, startAngle + position * (endAngle - startAngle), true);
        g.setColour(line);
        g.strokePath(track, juce::PathStrokeType(4.0f));
        g.setColour(acid);
        g.strokePath(amount, juce::PathStrokeType(4.0f));
        g.setColour(inset);
        g.fillEllipse(cx - radius + 8.0f, cy - radius + 8.0f,
                      2.0f * (radius - 8.0f), 2.0f * (radius - 8.0f));
        const float angle = startAngle + position * (endAngle - startAngle);
        g.setColour(bright);
        g.drawLine(cx + std::sin(angle) * (radius - 12.0f),
                   cy - std::cos(angle) * (radius - 12.0f),
                   cx + std::sin(angle) * (radius - 5.0f),
                   cy - std::cos(angle) * (radius - 5.0f), 2.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& colour, bool hover, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(colour.brighter(down ? 0.12f : hover ? 0.055f : 0.0f));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(hover ? acid.withAlpha(0.6f) : line);
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    }
};
}

PhraserAudioProcessorEditor::PhraserAudioProcessorEditor(PhraserAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    theme = std::make_unique<StudioLookAndFeel>();
    setLookAndFeel(theme.get());
    setSize(1460, 1150);

    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffcdebd8 });
    generateButton.onClick = [this]
    {
        processor.randomizePattern();
        status.setText("A new phrase has been generated", juce::dontSendNotification);
        repaint();
    };
    addAndMakeVisible(generateButton);

    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        auto& button = slotButtons[static_cast<size_t>(i)];
        button.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffe3ece6 });
        button.onClick = [this, i]
        {
            selectedSlot = i;
            updateRanges();
            updateSlotButtons();
            loadSlot(i);
        };
        addAndMakeVisible(button);
    }

    for (int i = 0; i < 2; ++i)
    {
        auto& save = saveButtons[static_cast<size_t>(i)];
        auto& recall = recallButtons[static_cast<size_t>(i)];
        const auto memory = juce::String(i == 0 ? "A" : "B");
        save.setButtonText("KEEP " + memory);
        recall.setButtonText("RECALL " + memory);
        save.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffd0e7d8 });
        recall.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffdce7e1 });
        save.onClick = [this, i]
        {
            processor.saveKeep(i);
            status.setText("Stored phrase in Keep " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
        };
        recall.onClick = [this, i]
        {
            processor.recallKeep(i);
            status.setText("Recalled Keep " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
            repaint();
        };
        addAndMakeVisible(save);
        addAndMakeVisible(recall);
    }

    constexpr std::array<const char*, controlCount> ids {
        "density", "slice", "breaks", "swing",
        "gain", "fade", "transient", "variation",
        "pitch", "pitchspread", "bars", "division"
    };
    constexpr std::array<const char*, controlCount> titles {
        "DENSITY", "SIZE", "BREAKS", "SWING",
        "OUTPUT", "FADE", "SNAP", "VARIATION",
        "PITCH", "SPREAD", "BARS", "STEPS/BAR"
    };
    for (int i = 0; i < controlCount; ++i)
    {
        auto& slider = sliders[static_cast<size_t>(i)];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 20);
        addAndMakeVisible(slider);
        auto& label = sliderLabels[static_cast<size_t>(i)];
        label.setText(titles[static_cast<size_t>(i)], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        attachments[static_cast<size_t>(i)] =
            std::make_unique<SliderAttachment>(processor.parameters, ids[static_cast<size_t>(i)], slider);
    }

    constexpr std::array<const char*, 4> names {
        "START FROM", "START TO", "LENGTH MIN", "LENGTH MAX"
    };
    for (int i = 0; i < 4; ++i)
    {
        auto& slider = rangeSliders[static_cast<size_t>(i)];
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 53, 20);
        slider.setRange(i < 2 ? 0.0 : 0.005, 1.0, 0.005);
        slider.onValueChange = [this] { applyRanges(); };
        addAndMakeVisible(slider);
        auto& label = rangeLabels[static_cast<size_t>(i)];
        label.setText(names[static_cast<size_t>(i)], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    }

    status.setText("Select a WAV lane, then edit its start/length random ranges.",
                   juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(status);
    updateRanges();
    updateSlotButtons();
    startTimerHz(24);
}

PhraserAudioProcessorEditor::~PhraserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PhraserAudioProcessorEditor::updateSlotButtons()
{
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        auto& button = slotButtons[static_cast<size_t>(i)];
        button.setButtonText(juce::String(i + 1).paddedLeft('0', 2)
                             + "   " + processor.getSlotName(i).substring(0, 14));
        button.setColour(juce::TextButton::buttonColourId,
            i == selectedSlot ? juce::Colour { 0xffb7d9c5 } : juce::Colour { 0xffe3ece6 });
    }
}

void PhraserAudioProcessorEditor::updateRanges()
{
    const auto settings = processor.getSlotSettings(selectedSlot);
    const std::array<float, 4> values {
        settings.startMin, settings.startMax, settings.lengthMin, settings.lengthMax
    };
    for (int i = 0; i < 4; ++i)
        rangeSliders[static_cast<size_t>(i)].setValue(
            values[static_cast<size_t>(i)], juce::dontSendNotification);
    repaint(1025, 454, 425, 183);
}

void PhraserAudioProcessorEditor::applyRanges()
{
    PhraserSlotSettings settings;
    settings.startMin = static_cast<float>(rangeSliders[0].getValue());
    settings.startMax = static_cast<float>(rangeSliders[1].getValue());
    settings.lengthMin = static_cast<float>(rangeSliders[2].getValue());
    settings.lengthMax = static_cast<float>(rangeSliders[3].getValue());
    processor.setSlotSettings(selectedSlot, settings);
    repaint(193, 101, 815, 635);
}

void PhraserAudioProcessorEditor::loadSlot(int index)
{
    chooser = std::make_unique<juce::FileChooser>(
        "WAV sample " + juce::String(index + 1), juce::File {}, "*.wav");
    const juce::Component::SafePointer<PhraserAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
        [safe, index](const juce::FileChooser& result)
        {
            if (safe == nullptr) return;
            const auto file = result.getResult();
            if (file == juce::File {}) return;
            const bool loaded = safe->processor.loadWavFile(index, file);
            safe->updateSlotButtons();
            safe->status.setText(loaded
                ? "Loaded " + file.getFileName() + " / " +
                  juce::String(safe->processor.getTransientCount(index)) + " onset marks"
                : "WAV load failed (unsupported or too long)",
                juce::dontSendNotification);
            safe->repaint();
        });
}

void PhraserAudioProcessorEditor::timerCallback()
{
    const int current = processor.getCurrentStep();
    if (current != lastStep)
    {
        lastStep = current;
        repaint(20, 765, 1418, 155);
    }
    repaint(190, 106, 826, 630);
    repaint(1027, 752, 420, 380);
}

void PhraserAudioProcessorEditor::paintLanes(juce::Graphics& g)
{
    constexpr float x0 = 207.0f;
    constexpr float x1 = 1005.0f;
    constexpr float width = x1 - x0;

    for (int slot = 0; slot < PhraserAudioProcessor::numSlots; ++slot)
    {
        const float y = 116.0f + 39.0f * slot;
        const float mid = y + 16.0f;
        const bool selected = slot == selectedSlot;
        g.setColour(selected ? juce::Colour { 0xffd5e5db } : inset);
        g.fillRect(27.0f, y - 2.0f, 978.0f, 36.0f);
        g.setColour(line);
        g.drawHorizontalLine(static_cast<int>(y + 35), 29.0f, 1006.0f);
        g.setColour(trackColour(slot));
        g.fillRect(27.0f, y + 2.0f, 3.0f, 27.0f);

        auto sample = processor.getSampleSnapshot(slot);
        if (!sample)
        {
            g.setColour(muted.withAlpha(0.35f));
            g.drawLine(x0, mid, x1, mid, 1.0f);
            continue;
        }

        const auto settings = processor.getSlotSettings(slot);
        const float from = x0 + width * settings.startMin;
        const float to = x0 + width * settings.startMax;
        g.setColour(amber.withAlpha(selected ? 0.15f : 0.07f));
        g.fillRect(from, y + 1.0f, juce::jmax(1.0f, to - from), 30.0f);
        if (selected)
        {
            g.setColour(amber.withAlpha(0.8f));
            g.drawVerticalLine(static_cast<int>(from), y + 1.0f, y + 31.0f);
            g.drawVerticalLine(static_cast<int>(to), y + 1.0f, y + 31.0f);
        }

        g.setColour(trackColour(slot).withAlpha(selected ? 1.0f : 0.7f));
        for (int i = 0; i < PhraserAudioProcessor::waveformPoints; ++i)
        {
            const float x = x0 + width * (i + 0.5f)
                                     / PhraserAudioProcessor::waveformPoints;
            const float amplitude = sample->waveform[static_cast<size_t>(i)] * 13.5f;
            g.drawVerticalLine(static_cast<int>(x), mid - amplitude, mid + amplitude);
        }

        const auto position = processor.getPlayPosition(slot);
        if (position >= 0.0f)
        {
            const float playX = x0 + width * juce::jlimit(0.0f, 1.0f, position);
            g.setColour(acid);
            g.drawVerticalLine(static_cast<int>(playX), y - 1.0f, y + 33.0f);
            g.fillEllipse(playX - 2.5f, y + 30.5f, 5.0f, 5.0f);
        }
    }
}

void PhraserAudioProcessorEditor::paintSequence(juce::Graphics& g)
{
    const auto pattern = processor.getPatternSnapshot();
    const int active = processor.getActiveSteps();
    const int division = processor.getStepsPerBar();
    const int playing = processor.getCurrentStep();
    g.setColour(bright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("PHRASE GRID", 34, 768, 135, 23, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String(active) + " STEPS  /  " +
               juce::String(active / division) + " BARS  /  " +
               juce::String(division) + " STEPS PER BAR",
               174, 768, 440, 23, juce::Justification::centredLeft);
    g.drawText("CLICK: MUTE   SHIFT: ROUTE   RIGHT: LOCK",
               585, 768, 398, 23, juce::Justification::centredRight);

    // 4 rows of 16 cells. In 8-step mode, each physical row is two bars.
    for (int i = 0; i < PhraserAudioProcessor::numSteps; ++i)
    {
        const int row = i / 16;
        const int col = i % 16;
        const float x = 34.0f + col * 60.0f;
        const float y = 797.0f + row * 30.0f;
        const bool visible = i < active;
        const auto& event = pattern.events[static_cast<size_t>(i)];
        const bool playingNow = visible && i == playing;

        auto colour = visible && event.enabled
            ? trackColour(juce::jlimit(0, 15, event.slot)).withAlpha(0.8f)
            : juce::Colour { 0xffdce6df };
        if (!visible) colour = juce::Colour { 0xffe7ede9 };
        g.setColour(colour);
        g.fillRoundedRectangle(x, y, 55.0f, 22.0f, 2.0f);
        if (playingNow)
        {
            g.setColour(acid);
            g.drawRoundedRectangle(x, y, 55.0f, 22.0f, 2.0f, 2.0f);
        }
        if (event.locked && visible)
        {
            g.setColour(amber);
            g.fillRect(x + 48.0f, y + 2.0f, 4.0f, 4.0f);
        }
        g.setColour(visible && event.enabled ? juce::Colours::white : muted.withAlpha(0.75f));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(juce::String(i + 1).paddedLeft('0', 2)
                   + (visible && event.enabled ? "  S" + juce::String(event.slot + 1) : ""),
                   static_cast<int>(x + 6), static_cast<int>(y),
                   48, 22, juce::Justification::centredLeft);
    }
}

void PhraserAudioProcessorEditor::paintOverlapOrbit(juce::Graphics& g)
{
    constexpr float cx = 1231.0f;
    constexpr float cy = 937.0f;
    constexpr float firstRadius = 45.0f;
    constexpr float spacing = 6.5f;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int active = processor.getActiveSteps();
    const auto phrase = processor.getPatternSnapshot();
    const double bpm = juce::jmax(20.0, processor.getHostBpm());
    const int subdivision = processor.getStepsPerBar();
    const double phraseSeconds = 4.0 * (active / subdivision) * 60.0 / bpm;

    g.setColour(surface);
    g.fillRoundedRectangle(1027.0f, 753.0f, 410.0f, 364.0f, 5.0f);
    g.setColour(line);
    g.drawRoundedRectangle(1027.0f, 753.0f, 410.0f, 364.0f, 5.0f, 1.0f);
    g.setColour(bright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("POLYPHONIC OVERLAP / 16 LAYERS", 1048, 764, 367, 25,
               juce::Justification::centredLeft);

    // Each ring represents one WAV. Arcs run for the estimated independent
    // slice duration in song time, so multiple rings can overlap visually.
    for (int slot = 0; slot < PhraserAudioProcessor::numSlots; ++slot)
    {
        const float radius = firstRadius + spacing * slot;
        const int playing = processor.getVoiceCountForSlot(slot);
        const auto colour = trackColour(slot);
        g.setColour(line.withAlpha(0.48f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.3f);

        const auto source = processor.getSampleSnapshot(slot);
        if (source)
        {
            for (int step = 0; step < active; ++step)
            {
                const auto& e = phrase.events[static_cast<size_t>(step)];
                if (!e.enabled || e.slot != slot) continue;
                const auto* pitchParameter = processor.parameters.getRawParameterValue("pitch");
                const double semitones = juce::jlimit(-24.0, 24.0,
                    static_cast<double>(e.semitones) + pitchParameter->load());
                const double seconds = e.length * source->audio.getNumSamples()
                    / source->sampleRate / std::pow(2.0, semitones / 12.0);
                const float start = (static_cast<float>(step) / active) * twoPi;
                const float span = juce::jlimit(0.012f, twoPi - 0.03f,
                    static_cast<float>(seconds / phraseSeconds) * twoPi);
                juce::Path arc;
                arc.addCentredArc(cx, cy, radius, radius, 0.0f,
                                 start, start + span, true);
                g.setColour(colour.withAlpha(playing > 0 ? 0.9f : 0.58f));
                g.strokePath(arc, juce::PathStrokeType(4.4f));
            }
        }
        if (playing > 0)
        {
            g.setColour(colour.withAlpha(0.94f));
            g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.6f);
        }
    }

    const int current = processor.getCurrentStep();
    if (current >= 0 && active > 0)
    {
        const float angle = (static_cast<float>(current) / active) * twoPi;
        g.setColour(acid.withAlpha(0.8f));
        g.drawLine(cx, cy, cx + std::sin(angle) * 148.0f,
                   cy - std::cos(angle) * 148.0f, 1.5f);
    }

    g.setColour(surface);
    g.fillEllipse(cx - 39.0f, cy - 39.0f, 78.0f, 78.0f);
    g.setColour(bright);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText(juce::String(processor.getActiveVoiceCount()), static_cast<int>(cx - 34),
               static_cast<int>(cy - 26), 68, 33, juce::Justification::centred);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("VOICES", static_cast<int>(cx - 35), static_cast<int>(cy + 11),
               70, 19, juce::Justification::centred);
    g.setColour(muted);
    g.drawText("RINGS: WAV 01 INNER -> WAV 16 OUTER", 1048, 1084, 365, 18,
               juce::Justification::centred);
}

void PhraserAudioProcessorEditor::paintPanel(juce::Graphics& g)
{
    g.setColour(surface);
    g.fillRoundedRectangle(1027.0f, 100.0f, 410.0f, 631.0f, 5.0f);
    g.setColour(line);
    g.drawRoundedRectangle(1027.0f, 100.0f, 410.0f, 631.0f, 5.0f, 1.0f);
    g.setColour(bright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("ENGINE PARAMETERS", 1046, 105, 358, 24, juce::Justification::centredLeft);
    g.setColour(line);
    g.drawLine(1046.0f, 440.0f, 1417.0f, 440.0f);
    g.setColour(bright);
    g.drawText("SAMPLE " + juce::String(selectedSlot + 1).paddedLeft('0', 2)
               + "  /  RANDOM WINDOWS", 1046, 447, 363, 25,
               juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("START RANGE  =  portion of WAV available to the generator",
               1046, 619, 367, 17, juce::Justification::centredLeft);
    g.drawText("LENGTH RANGE =  random slice duration in WAV proportions",
               1046, 634, 367, 17, juce::Justification::centredLeft);
}

void PhraserAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ink);
    g.setColour(bright);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    g.drawText("PHRASER", 28, 20, 220, 43, juce::Justification::centredLeft);
    g.setColour(acid);
    g.fillRect(28, 71, 92, 3);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("WAVEFORM MATRIX + OVERLAP ORBIT  /  v0.6", 250, 36, 300, 20,
               juce::Justification::centredLeft);
    g.drawText("16 SOURCES   /   1-4 BARS   /   8 OR 16 STEPS PER BAR",
               612, 36, 570, 20, juce::Justification::centredRight);
    g.setColour(line);
    g.drawHorizontalLine(88, 27.0f, 1435.0f);
    g.setColour(bright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("SAMPLE / WAVEFORMS / PLAYHEAD", 29, 94, 562, 20,
               juce::Justification::centredLeft);
    paintLanes(g);
    paintPanel(g);

    g.setColour(surface);
    g.fillRoundedRectangle(20.0f, 753.0f, 990.0f, 181.0f, 5.0f);
    paintSequence(g);
    paintOverlapOrbit(g);
    g.setColour(bright);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("INDEPENDENT VOICES", 34, 954, 460, 22, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(12.0f));
    g.setColour(muted);
    g.drawText("Each sample keeps playing for its full slice duration, even after the next step.",
               34, 980, 890, 30, juce::Justification::centredLeft);
    g.drawText("Colored arcs in the circle show scheduled overlaps; brighter rings show voices active now.",
               34, 1010, 930, 28, juce::Justification::centredLeft);
}

void PhraserAudioProcessorEditor::resized()
{
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
        slotButtons[static_cast<size_t>(i)].setBounds(39, 120 + 39 * i, 148, 26);

    for (int i = 0; i < controlCount; ++i)
    {
        const int x = 1045 + (i % 4) * 97;
        const int y = 131 + (i / 4) * 102;
        sliderLabels[static_cast<size_t>(i)].setBounds(x, y, 89, 20);
        sliders[static_cast<size_t>(i)].setBounds(x + 4, y + 19, 82, 76);
    }
    for (int i = 0; i < 4; ++i)
    {
        rangeLabels[static_cast<size_t>(i)].setBounds(1047, 480 + 35 * i, 106, 27);
        rangeSliders[static_cast<size_t>(i)].setBounds(1154, 480 + 35 * i, 259, 27);
    }
    generateButton.setBounds(1046, 657, 367, 32);
    for (int i = 0; i < 2; ++i)
    {
        saveButtons[static_cast<size_t>(i)].setBounds(1046 + i * 187, 699, 90, 26);
        recallButtons[static_cast<size_t>(i)].setBounds(1141 + i * 187, 699, 86, 26);
    }
    status.setBounds(24, 1120, 1400, 22);
}

void PhraserAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    // Click a waveform lane to inspect its independent start/duration range.
    if (event.x >= 193 && event.x <= 1005 && event.y >= 114 && event.y < 740)
    {
        selectedSlot = juce::jlimit(0, 15, (event.y - 116) / 39);
        updateRanges();
        updateSlotButtons();
        repaint();
        return;
    }

    if (event.y < 797 || event.y > 910 || event.x < 34 || event.x > 997)
        return;
    const int column = juce::jlimit(0, 15, static_cast<int>((event.x - 34) / 60.0f));
    const int row = juce::jlimit(0, 3, (event.y - 797) / 30);
    const int step = row * 16 + column;
    if (step >= processor.getActiveSteps()) return;

    if (event.mods.isRightButtonDown()) processor.toggleLock(step);
    else if (event.mods.isShiftDown()) processor.setStepSlot(step, selectedSlot);
    else processor.toggleStep(step);
    repaint(20, 750, 1418, 370);
}
