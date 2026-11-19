#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
const juce::Colour paper { 0xffedf2ef };
const juce::Colour white { 0xfffcfdfa };
const juce::Colour slate { 0xff1e3029 };
const juce::Colour muted { 0xff668076 };
const juce::Colour rule { 0xffcedbd1 };
const juce::Colour lane { 0xffe4ebe5 };
const juce::Colour green { 0xff238b60 };
const juce::Colour orange { 0xffb76a34 };

juce::Colour sampleColour(int index)
{
    const std::array<juce::Colour, 4> families {
        juce::Colour { 0xff167d77 }, juce::Colour { 0xffb47635 },
        juce::Colour { 0xff675cb5 }, juce::Colour { 0xffb45078 }
    };
    return families[static_cast<size_t>(index / 4)];
}

class CompactLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    CompactLookAndFeel()
    {
        setColour(juce::TextButton::textColourOffId, slate);
        setColour(juce::Label::textColourId, muted);
        setColour(juce::Slider::textBoxTextColourId, slate);
        setColour(juce::Slider::textBoxBackgroundColourId, white);
        setColour(juce::Slider::textBoxOutlineColourId, rule);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& base, bool hovered, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(base.brighter(down ? 0.06f : hovered ? 0.03f : 0.0f));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(hovered ? green : rule);
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float position, float start, float end, juce::Slider&) override
    {
        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.5f;
        const float r = juce::jmin(w, h) * 0.35f;
        juce::Path track, arc;
        track.addCentredArc(cx, cy, r, r, 0.0f, start, end, true);
        arc.addCentredArc(cx, cy, r, r, 0.0f, start, start + (end - start) * position, true);
        g.setColour(rule);
        g.strokePath(track, juce::PathStrokeType(3.5f));
        g.setColour(green);
        g.strokePath(arc, juce::PathStrokeType(3.5f));
        const float angle = start + (end - start) * position;
        g.setColour(slate);
        g.drawLine(cx + std::sin(angle) * (r - 10.0f),
                   cy - std::cos(angle) * (r - 10.0f),
                   cx + std::sin(angle) * (r - 4.0f),
                   cy - std::cos(angle) * (r - 4.0f), 2.0f);
    }
};
}

PhraserAudioProcessorEditor::PhraserAudioProcessorEditor(PhraserAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    theme = std::make_unique<CompactLookAndFeel>();
    setLookAndFeel(theme.get());
    setSize(1280, 850);

    addAndMakeVisible(generateButton);
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffbfdfca });
    generateButton.onClick = [this]
    {
        processor.randomizePattern();
        status.setText("Phrase generated", juce::dontSendNotification);
        repaint();
    };

    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        auto& load = slotButtons[static_cast<size_t>(i)];
        load.onClick = [this, i]
        {
            selectedSlot = i;
            updateSlotControls();
            loadSlot(i);
        };
        addAndMakeVisible(load);

        auto& mode = modeButtons[static_cast<size_t>(i)];
        mode.onClick = [this, i]
        {
            auto setting = processor.getSlotSettings(i);
            setting.fadeAtNextStep = !setting.fadeAtNextStep;
            processor.setSlotSettings(i, setting);
            updateSlotControls();
            repaint();
        };
        addAndMakeVisible(mode);

        auto& sync = syncButtons[static_cast<size_t>(i)];
        sync.onClick = [this, i]
        {
            auto setting = processor.getSlotSettings(i);
            setting.syncToHost = !setting.syncToHost;
            processor.setSlotSettings(i, setting);
            updateSlotControls();
            repaint();
        };
        addAndMakeVisible(sync);

        auto& bpm = tempoLabels[static_cast<size_t>(i)];
        bpm.setEditable(true, true, false);
        bpm.setJustificationType(juce::Justification::centred);
        bpm.setColour(juce::Label::backgroundColourId, white);
        bpm.setColour(juce::Label::outlineColourId, rule);
        bpm.onTextChange = [this, i]
        {
            const auto input = tempoLabels[static_cast<size_t>(i)].getText().trim();
            auto setting = processor.getSlotSettings(i);
            const double requested = input.getDoubleValue();
            setting.manualBpm = requested >= 40.0 && requested <= 240.0
                ? requested : 0.0;
            processor.setSlotSettings(i, setting);
            updateSlotControls();
            repaint();
        };
        addAndMakeVisible(bpm);

        auto& tuning = pitchLabels[static_cast<size_t>(i)];
        tuning.setEditable(true, true, false);
        tuning.setJustificationType(juce::Justification::centred);
        tuning.setColour(juce::Label::backgroundColourId, white);
        tuning.setColour(juce::Label::outlineColourId, rule);
        tuning.setTooltip("Independent WAV pitch in semitones (-12..+12). Tempo SYNC preserves this tuning.");
        tuning.onTextChange = [this, i]
        {
            auto setting = processor.getSlotSettings(i);
            const auto entered = pitchLabels[static_cast<size_t>(i)].getText().trim();
            setting.pitchSemitones = juce::jlimit(-12, 12, entered.getIntValue());
            processor.setSlotSettings(i, setting);
            updateSlotControls();
            repaint();
        };
        addAndMakeVisible(tuning);
    }

    for (int i = 0; i < 2; ++i)
    {
        const auto name = juce::String(i == 0 ? "A" : "B");
        auto& save = saveButtons[static_cast<size_t>(i)];
        auto& recall = recallButtons[static_cast<size_t>(i)];
        save.setButtonText("KEEP " + name);
        recall.setButtonText("RECALL " + name);
        save.onClick = [this, i]
        {
            processor.saveKeep(i);
            status.setText("Saved Keep " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
        };
        recall.onClick = [this, i]
        {
            processor.recallKeep(i);
            repaint();
            status.setText("Recalled Keep " + juce::String(i == 0 ? "A" : "B"),
                           juce::dontSendNotification);
        };
        addAndMakeVisible(save);
        addAndMakeVisible(recall);
    }

    constexpr std::array<const char*, controlCount> ids {
        "density", "slice", "breaks", "swing", "gain", "fade",
        "transient", "variation", "pitch", "pitchspread", "bars", "division"
    };
    constexpr std::array<const char*, controlCount> names {
        "DENSITY", "SIZE", "BREAKS", "SWING", "OUTPUT", "FADE",
        "SNAP", "VARIATION", "PITCH", "SPREAD", "BARS", "STEPS/BAR"
    };
    for (int i = 0; i < controlCount; ++i)
    {
        auto& slider = sliders[static_cast<size_t>(i)];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 19);
        addAndMakeVisible(slider);
        auto& label = sliderLabels[static_cast<size_t>(i)];
        label.setText(names[static_cast<size_t>(i)], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        attachments[static_cast<size_t>(i)] =
            std::make_unique<SliderAttachment>(processor.parameters, ids[static_cast<size_t>(i)], slider);
    }

    status.setColour(juce::Label::textColourId, muted);
    status.setText("Drag wave = start range; SHIFT+drag = length. BPM: 0 = auto. PITCH: per-WAV semitones.",
                   juce::dontSendNotification);
    addAndMakeVisible(status);
    updateSlotControls();
    startTimerHz(24);
}

PhraserAudioProcessorEditor::~PhraserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PhraserAudioProcessorEditor::updateSlotControls()
{
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        auto setting = processor.getSlotSettings(i);
        auto& load = slotButtons[static_cast<size_t>(i)];
        load.setButtonText(juce::String(i + 1).paddedLeft('0', 2) + "  "
                           + processor.getSlotName(i).substring(0, 11));
        load.setColour(juce::TextButton::buttonColourId,
                       i == selectedSlot ? juce::Colour { 0xffc3dfd0 } : white);
        auto& mode = modeButtons[static_cast<size_t>(i)];
        mode.setButtonText(setting.fadeAtNextStep ? "5ms" : "FULL");
        mode.setColour(juce::TextButton::buttonColourId,
                       setting.fadeAtNextStep ? juce::Colour { 0xfff2dabe } : white);
        auto& sync = syncButtons[static_cast<size_t>(i)];
        sync.setButtonText(setting.syncToHost ? "SYNC" : "FREE");
        sync.setColour(juce::TextButton::buttonColourId,
                       setting.syncToHost ? juce::Colour { 0xffcce9d7 } : white);

        auto& tempo = tempoLabels[static_cast<size_t>(i)];
        const auto detected = processor.getSampleTempo(i);
        tempo.setText(detected > 0.0 ? juce::String(detected, 0) : "?",
                      juce::dontSendNotification);
        tempo.setTooltip(setting.manualBpm > 0.0
            ? "Manual BPM. Type 0 to return to auto-detection."
            : "Estimated BPM. Click to override (40-240); type 0 for auto.");
        pitchLabels[static_cast<size_t>(i)].setText(
            juce::String(setting.pitchSemitones > 0 ? "+" : "")
                + juce::String(setting.pitchSemitones),
            juce::dontSendNotification);
    }
}

void PhraserAudioProcessorEditor::loadSlot(int index)
{
    chooser = std::make_unique<juce::FileChooser>(
        "WAV slot " + juce::String(index + 1), juce::File {}, "*.wav");
    const juce::Component::SafePointer<PhraserAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
        [safe, index](const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            const auto file = fc.getResult();
            if (file == juce::File {}) return;
            const bool loaded = safe->processor.loadWavFile(index, file);
            safe->updateSlotControls();
            safe->repaint();
            safe->status.setText(loaded
                ? "Loaded " + file.getFileName() + ", BPM " +
                  juce::String(safe->processor.getSampleTempo(index), 0)
                : "WAV load failed",
                juce::dontSendNotification);
        });
}

void PhraserAudioProcessorEditor::timerCallback()
{
    const int active = processor.getCurrentStep();
    if (active != lastStep)
    {
        lastStep = active;
        repaint(18, 637, 1242, 155);
    }
    repaint(360, 98, 501, 482);
    repaint(880, 447, 378, 346);
}

void PhraserAudioProcessorEditor::paintLanes(juce::Graphics& g)
{
    for (int slot = 0; slot < PhraserAudioProcessor::numSlots; ++slot)
    {
        const float y = laneTop + lanePitch * slot;
        const float mid = y + 12.0f;
        const auto colour = sampleColour(slot);
        const bool selected = slot == selectedSlot;
        g.setColour(selected ? juce::Colour { 0xffd7e8dc } : lane);
        g.fillRect(20.0f, y - 1.0f, 842.0f, 28.0f);
        g.setColour(colour);
        g.fillRect(20.0f, y + 2.0f, 3.0f, 21.0f);

        const auto settings = processor.getSlotSettings(slot);
        const float sx = waveLeft + settings.startMin * waveWidth;
        const float ex = waveLeft + settings.startMax * waveWidth;
        g.setColour(green.withAlpha(selected ? 0.18f : 0.09f));
        g.fillRect(sx, y, juce::jmax(1.0f, ex - sx), 25.0f);

        // Independent duration interval is shown as a narrow ochre band
        // underneath the waveform, in the same 0..1 normalized time scale.
        const float lx = waveLeft + settings.lengthMin * waveWidth;
        const float rx = waveLeft + settings.lengthMax * waveWidth;
        g.setColour(orange.withAlpha(selected ? 0.95f : 0.58f));
        g.fillRect(lx, y + 23.0f, juce::jmax(2.0f, rx - lx), 2.0f);

        if (selected)
        {
            g.setColour(green);
            g.drawVerticalLine(static_cast<int>(sx), y + 1.0f, y + 22.0f);
            g.drawVerticalLine(static_cast<int>(ex), y + 1.0f, y + 22.0f);
        }

        const auto sample = processor.getSampleSnapshot(slot);
        if (!sample)
        {
            g.setColour(muted.withAlpha(0.45f));
            g.drawLine(waveLeft, mid, waveLeft + waveWidth, mid, 1.0f);
            continue;
        }
        g.setColour(colour.withAlpha(selected ? 1.0f : 0.75f));
        for (int i = 0; i < PhraserAudioProcessor::waveformPoints; ++i)
        {
            const float x = waveLeft + waveWidth * (i + 0.5f)
                                      / PhraserAudioProcessor::waveformPoints;
            const float peak = sample->waveform[static_cast<size_t>(i)] * 10.0f;
            g.drawVerticalLine(static_cast<int>(x), mid - peak, mid + peak);
        }

        const auto position = processor.getPlayPosition(slot);
        if (position >= 0.0f)
        {
            const float x = waveLeft + waveWidth * juce::jlimit(0.0f, 1.0f, position);
            g.setColour(green);
            g.drawVerticalLine(static_cast<int>(x), y, y + 25.0f);
        }
    }
}

void PhraserAudioProcessorEditor::paintSequence(juce::Graphics& g)
{
    const auto phrase = processor.getPatternSnapshot();
    const int active = processor.getActiveSteps();
    const int division = processor.getStepsPerBar();
    const int current = processor.getCurrentStep();
    g.setColour(slate);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("PHRASE / " + juce::String(active) + " STEPS / "
               + juce::String(active / division) + " BARS", 24, 625, 360, 22,
               juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("CLICK mute   SHIFT+CLICK assign selected WAV   RIGHT-CLICK lock",
               377, 625, 475, 22, juce::Justification::centredRight);

    for (int i = 0; i < PhraserAudioProcessor::numSteps; ++i)
    {
        const int row = i / 16;
        const int col = i % 16;
        const float x = 24.0f + col * 52.1f;
        const float y = 654.0f + row * 29.0f;
        const auto& e = phrase.events[static_cast<size_t>(i)];
        const bool on = i < active && e.enabled;
        const bool now = i == current && i < active;
        g.setColour(i >= active ? juce::Colour { 0xffe7ece7 }
                    : on ? sampleColour(juce::jlimit(0, 15, e.slot))
                    : juce::Colour { 0xffd3dfd6 });
        g.fillRoundedRectangle(x, y, 48.0f, 23.0f, 1.5f);
        if (now)
        {
            g.setColour(green);
            g.drawRoundedRectangle(x, y, 48.0f, 23.0f, 1.5f, 2.0f);
        }
        if (i < active && e.locked)
        {
            g.setColour(orange);
            g.fillRect(x + 43.0f, y + 2.0f, 3.0f, 3.0f);
        }
        g.setColour(on ? juce::Colours::white : muted);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(juce::String(i + 1).paddedLeft('0', 2)
                   + (on ? ":" + juce::String(e.slot + 1) : ""),
                   static_cast<int>(x + 3.0f), static_cast<int>(y),
                   42, 22, juce::Justification::centredLeft);
    }
}

void PhraserAudioProcessorEditor::paintOverlapOrbit(juce::Graphics& g)
{
    constexpr float cx = 1075.0f;
    constexpr float cy = 618.0f;
    constexpr float radius0 = 27.0f;
    constexpr float spacing = 6.3f;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int active = processor.getActiveSteps();
    const auto phrase = processor.getPatternSnapshot();
    const double hostBpm = juce::jmax(20.0, processor.getHostBpm());
    const double phraseSeconds = 4.0 * (active / processor.getStepsPerBar()) * 60.0 / hostBpm;

    g.setColour(white);
    g.fillRoundedRectangle(881.0f, 454.0f, 376.0f, 337.0f, 3.0f);
    g.setColour(rule);
    g.drawRoundedRectangle(881.0f, 454.0f, 376.0f, 337.0f, 3.0f, 1.0f);
    g.setColour(slate);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("OVERLAP / 16 SOURCES", 896, 464, 325, 22, juce::Justification::centredLeft);

    for (int slot = 0; slot < PhraserAudioProcessor::numSlots; ++slot)
    {
        const float r = radius0 + spacing * slot;
        const int playing = processor.getVoiceCountForSlot(slot);
        const auto colour = sampleColour(slot);
        g.setColour(rule.withAlpha(0.7f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        const auto source = processor.getSampleSnapshot(slot);
        if (source)
        {
            const auto settings = processor.getSlotSettings(slot);
            const double sourceTempo = processor.getSampleTempo(slot);
            const double tempoRatio = settings.syncToHost && sourceTempo > 0.0
                ? hostBpm / sourceTempo : 1.0;
            for (int i = 0; i < active; ++i)
            {
                const auto& e = phrase.events[static_cast<size_t>(i)];
                if (!e.enabled || e.slot != slot) continue;
                const double seconds = e.length * source->audio.getNumSamples()
                    / source->sampleRate / tempoRatio;
                const double shown = settings.fadeAtNextStep
                    ? juce::jmin(seconds, phraseSeconds / active + 0.005) : seconds;
                const float a0 = (static_cast<float>(i) / active) * twoPi;
                const float span = juce::jlimit(0.012f, twoPi - 0.025f,
                    static_cast<float>(shown / phraseSeconds) * twoPi);
                juce::Path arc;
                arc.addCentredArc(cx, cy, r, r, 0.0f, a0, a0 + span, true);
                g.setColour(colour.withAlpha(playing > 0 ? 0.92f : 0.57f));
                g.strokePath(arc, juce::PathStrokeType(3.4f));
            }
        }
        if (playing > 0)
        {
            g.setColour(colour.withAlpha(0.75f));
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);
        }
    }

    const int current = processor.getCurrentStep();
    if (current >= 0 && active > 0)
    {
        const float angle = current * twoPi / active;
        g.setColour(green);
        g.drawLine(cx, cy, cx + std::sin(angle) * 134.0f,
                   cy - std::cos(angle) * 134.0f, 1.6f);
    }
    g.setColour(white);
    g.fillEllipse(cx - 24.0f, cy - 24.0f, 48.0f, 48.0f);
    g.setColour(slate);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText(juce::String(processor.getActiveVoiceCount()),
               static_cast<int>(cx - 23), static_cast<int>(cy - 18),
               46, 24, juce::Justification::centred);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("VOICE", static_cast<int>(cx - 24), static_cast<int>(cy + 5),
               48, 15, juce::Justification::centred);
    g.setColour(muted);
    g.drawText("INNER: WAV 01   /   OUTER: WAV 16",
               905, 769, 335, 16, juce::Justification::centred);
}

void PhraserAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(paper);
    g.setColour(slate);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("PHRASER", 23, 16, 182, 35, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("PITCH-INDEPENDENT SYNC / v0.8", 203, 23, 285, 21,
               juce::Justification::centredLeft);
    g.drawText("DRAG wave: START FROM / TO     SHIFT+DRAG: LENGTH MIN / MAX",
               23, 58, 850, 22, juce::Justification::centredLeft);
    g.setColour(rule);
    g.drawHorizontalLine(86, 20.0f, 1261.0f);
    g.setColour(slate);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("SAMPLE", 28, 88, 130, 15, juce::Justification::centredLeft);
    g.drawText("END", 170, 88, 45, 15, juce::Justification::centredLeft);
    g.drawText("CLOCK", 226, 88, 60, 15, juce::Justification::centredLeft);
    g.drawText("BPM", 293, 88, 55, 15, juce::Justification::centred);
    g.drawText("TUNE", 355, 88, 60, 15, juce::Justification::centred);
    g.drawText("WAVEFORM / START (GREEN) / DURATION (OCHRE)",
               428, 88, 430, 15, juce::Justification::centredLeft);

    paintLanes(g);
    g.setColour(white);
    g.fillRoundedRectangle(882.0f, 93.0f, 376.0f, 355.0f, 3.0f);
    g.setColour(rule);
    g.drawRoundedRectangle(882.0f, 93.0f, 376.0f, 355.0f, 3.0f, 1.0f);
    g.setColour(slate);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("PHRASE ENGINE", 896, 101, 305, 19, juce::Justification::centredLeft);

    g.setColour(white);
    g.fillRoundedRectangle(15.0f, 613.0f, 850.0f, 174.0f, 3.0f);
    paintSequence(g);
    paintOverlapOrbit(g);

    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("SYNC changes duration without changing pitch. TUNE = -12..+12 semitones; BPM = 0 for automatic.",
               22, 789, 843, 19, juce::Justification::centredLeft);
}

void PhraserAudioProcessorEditor::resized()
{
    for (int i = 0; i < PhraserAudioProcessor::numSlots; ++i)
    {
        const int y = static_cast<int>(laneTop + i * lanePitch) + 1;
        slotButtons[static_cast<size_t>(i)].setBounds(27, y, 132, 24);
        modeButtons[static_cast<size_t>(i)].setBounds(166, y, 54, 24);
        syncButtons[static_cast<size_t>(i)].setBounds(224, y, 60, 24);
        tempoLabels[static_cast<size_t>(i)].setBounds(291, y, 60, 24);
        pitchLabels[static_cast<size_t>(i)].setBounds(355, y, 62, 24);
    }
    for (int i = 0; i < controlCount; ++i)
    {
        const int x = 895 + (i % 4) * 90;
        const int y = 124 + (i / 4) * 87;
        sliderLabels[static_cast<size_t>(i)].setBounds(x, y, 83, 19);
        sliders[static_cast<size_t>(i)].setBounds(x + 3, y + 17, 76, 70);
    }
    generateButton.setBounds(899, 389, 345, 29);
    for (int i = 0; i < 2; ++i)
    {
        saveButtons[static_cast<size_t>(i)].setBounds(899 + i * 175, 422, 82, 23);
        recallButtons[static_cast<size_t>(i)].setBounds(987 + i * 175, 422, 81, 23);
    }
    status.setBounds(22, 814, 1238, 25);
}

void PhraserAudioProcessorEditor::applyWaveDrag(const juce::MouseEvent& event)
{
    if (!juce::isPositiveAndBelow(dragSlot, PhraserAudioProcessor::numSlots)) return;
    const float current = juce::jlimit(0.0f, 1.0f,
        (static_cast<float>(event.x) - waveLeft) / waveWidth);
    auto settings = processor.getSlotSettings(dragSlot);
    if (dragDuration)
    {
        settings.lengthMin = juce::jmax(0.005f, juce::jmin(dragAnchor, current));
        settings.lengthMax = juce::jmax(settings.lengthMin, juce::jmax(dragAnchor, current));
    }
    else
    {
        settings.startMin = juce::jmin(dragAnchor, current);
        settings.startMax = juce::jmax(dragAnchor, current);
    }
    processor.setSlotSettings(dragSlot, settings);
    repaint(18, static_cast<int>(laneTop + dragSlot * lanePitch) - 1, 848, 30);
}

void PhraserAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    const float y = static_cast<float>(event.y);
    if (event.x >= waveLeft && event.x <= waveLeft + waveWidth
        && y >= laneTop && y < laneTop + lanePitch * PhraserAudioProcessor::numSlots)
    {
        dragSlot = juce::jlimit(0, PhraserAudioProcessor::numSlots - 1,
                               static_cast<int>((y - laneTop) / lanePitch));
        selectedSlot = dragSlot;
        dragAnchor = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(event.x) - waveLeft) / waveWidth);
        dragDuration = event.mods.isShiftDown();
        updateSlotControls();
        applyWaveDrag(event);
        return;
    }
    if (event.y < 654 || event.y > 764 || event.x < 24 || event.x > 858) return;
    const int col = juce::jlimit(0, 15, static_cast<int>((event.x - 24) / 52.1f));
    const int row = juce::jlimit(0, 3, (event.y - 654) / 29);
    const int step = row * 16 + col;
    if (step >= processor.getActiveSteps()) return;
    if (event.mods.isRightButtonDown()) processor.toggleLock(step);
    else if (event.mods.isShiftDown()) processor.setStepSlot(step, selectedSlot);
    else processor.toggleStep(step);
    repaint(15, 612, 850, 175);
}

void PhraserAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (dragSlot >= 0) applyWaveDrag(event);
}

void PhraserAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (dragSlot >= 0) applyWaveDrag(event);
    dragSlot = -1;
}
