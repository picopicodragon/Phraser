#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TransientDetection.h"
#include "TempoDetection.h"
#include "GrainAlignment.h"
#include "PitchClassDetection.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr int maxSampleFrames = 4000000;
constexpr int maxStoredSteps = PhraserPattern::maxSteps;

void writeStep(juce::XmlElement& parent, const SliceEvent& e)
{
    auto* element = parent.createNewChildElement("Step");
    element->setAttribute("on", e.enabled);
    element->setAttribute("hold", e.locked);
    element->setAttribute("slot", e.slot);
    element->setAttribute("offset", e.offset);
    element->setAttribute("length", e.length);
    element->setAttribute("pitch", e.semitones);
}

PhraserPattern readPattern(const juce::XmlElement& parent)
{
    PhraserPattern restored;
    const int count = juce::jmin(maxStoredSteps, parent.getNumChildElements());
    for (int i = 0; i < count; ++i)
    {
        const auto* element = parent.getChildElement(i);
        if (!element || !element->hasTagName("Step")) continue;
        auto& e = restored.events[static_cast<size_t>(i)];
        e.enabled = element->getBoolAttribute("on", false);
        e.locked = element->getBoolAttribute("hold", false);
        e.slot = juce::jlimit(0, PhraserAudioProcessor::numSlots - 1,
                              element->getIntAttribute("slot", 0));
        e.offset = juce::jlimit(0.0f, 0.999f,
            static_cast<float>(element->getDoubleAttribute("offset", 0.0)));
        e.length = juce::jlimit(0.005f, 1.0f,
            static_cast<float>(element->getDoubleAttribute("length", 0.25)));
        e.semitones = juce::jlimit(-12, 12, element->getIntAttribute("pitch", 0));
    }
    return restored;
}
}

juce::AudioProcessorValueTreeState::ParameterLayout PhraserAudioProcessor::makeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> result;
    auto add = [&result](const char* id, const char* name,
                         float low, float high, float initial, float interval = 0.0f)
    {
        result.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { low, high, interval }, initial));
    };
    add("density", "Density", 0.0f, 1.0f, 0.8f);
    add("slice", "Slice Size", 0.02f, 1.0f, 0.25f);
    add("breaks", "Breaks", 0.0f, 1.0f, 0.2f);
    add("swing", "Swing", 0.0f, 0.75f, 0.0f);
    add("gain", "Gain", 0.0f, 1.0f, 0.65f);
    add("fade", "Fade", 0.001f, 0.03f, 0.006f);
    add("transient", "Transient Snap", 0.0f, 1.0f, 0.75f);
    add("variation", "Variation", 0.0f, 1.0f, 0.5f);
    add("pitch", "Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
    add("pitchspread", "Pitch Spread", 0.0f, 12.0f, 0.0f, 1.0f);
    add("bars", "Bars", 1.0f, 4.0f, 1.0f, 1.0f);
    // Preserved for old project-state compatibility. v0.9 always uses 16 steps/bar.
    add("division", "Steps per Bar (Legacy)", 8.0f, 16.0f, 16.0f, 8.0f);
    add("targetKey", "Reference Key (C=0)", 0.0f, 11.0f, 0.0f, 1.0f);
    return { result.begin(), result.end() };
}

PhraserAudioProcessor::PhraserAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", makeParameters())
{
    formatManager.registerBasicFormats();
    for (auto& position : playPositions)
        position.store(-1.0f, std::memory_order_relaxed);
    for (auto& mode : fadeAtNextStepModes)
        mode.store(false, std::memory_order_relaxed);
    for (auto& mode : syncToHostModes)
        mode.store(false, std::memory_order_relaxed);
    for (auto& tempo : manualTempos)
        tempo.store(0.0, std::memory_order_relaxed);
    for (auto& pitch : slotPitches)
        pitch.store(0, std::memory_order_relaxed);
    randomizePattern();
}

bool PhraserAudioProcessor::isBusesLayoutSupported(const BusesLayout& buses) const
{
    return buses.getMainInputChannelSet().isDisabled()
        && buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PhraserAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engineSampleRate = sampleRate;
    previousStep = std::numeric_limits<int64_t>::min();
    currentStep.store(-1, std::memory_order_relaxed);
    for (auto& voice : voices) voice = {};
    for (auto& pos : playPositions) pos.store(-1.0f, std::memory_order_relaxed);
    nextVoice = 0;
    voiceSerial = 0;
    activeVoiceCount.store(0, std::memory_order_relaxed);
    for (auto& count : slotVoiceCounts) count.store(0, std::memory_order_relaxed);
}

bool PhraserAudioProcessor::loadWavFile(int slot, const juce::File& file)
{
    if (!juce::isPositiveAndBelow(slot, numSlots)
        || !file.existsAsFile() || !file.hasFileExtension("wav"))
        return false;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (!reader || reader->numChannels < 1 || reader->sampleRate <= 0.0
        || reader->lengthInSamples < 2 || reader->lengthInSamples > maxSampleFrames)
        return false;

    auto loaded = std::make_shared<PhraserSample>();
    loaded->audio.setSize(juce::jmin(2, static_cast<int>(reader->numChannels)),
                          static_cast<int>(reader->lengthInSamples));
    if (!reader->read(&loaded->audio, 0, loaded->audio.getNumSamples(), 0, true, true))
        return false;

    loaded->sampleRate = reader->sampleRate;
    loaded->sourcePath = file.getFullPathName();
    loaded->transients = phraser::detectTransients(loaded->audio, loaded->sampleRate);
    loaded->detectedBpm = phraser::estimateSampleBpm(
        loaded->transients, loaded->audio.getNumSamples(), loaded->sampleRate);
    const auto estimatedPitch = phraser::estimatePitchClass(
        loaded->audio, loaded->sampleRate);
    loaded->detectedPitchClass = estimatedPitch.pitchClass;
    loaded->pitchConfidence = estimatedPitch.confidence;

    const int frames = loaded->audio.getNumSamples();
    for (int cell = 0; cell < waveformPoints; ++cell)
    {
        const int from = static_cast<int>((static_cast<int64_t>(cell) * frames) / waveformPoints);
        const int to = juce::jmin(frames, static_cast<int>(
            (static_cast<int64_t>(cell + 1) * frames) / waveformPoints));
        float peak = 0.0f;
        for (int channel = 0; channel < loaded->audio.getNumChannels(); ++channel)
        {
            const float* data = loaded->audio.getReadPointer(channel);
            for (int k = from; k < to; ++k)
                peak = juce::jmax(peak, std::abs(data[k]));
        }
        loaded->waveform[static_cast<size_t>(cell)] = juce::jlimit(0.0f, 1.0f, peak);
    }

    std::atomic_store(&samples[static_cast<size_t>(slot)],
                      std::shared_ptr<const PhraserSample>(loaded));
    return true;
}

std::shared_ptr<const PhraserSample> PhraserAudioProcessor::getSampleSnapshot(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return {};
    return std::atomic_load(&samples[static_cast<size_t>(slot)]);
}

juce::String PhraserAudioProcessor::getSlotName(int slot) const
{
    auto sample = getSampleSnapshot(slot);
    return sample ? juce::File(sample->sourcePath).getFileName() : "LOAD WAV";
}

double PhraserAudioProcessor::getSampleTempo(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return 0.0;
    const double manual = manualTempos[static_cast<size_t>(slot)].load(std::memory_order_acquire);
    if (manual > 0.0) return manual;
    const auto sample = getSampleSnapshot(slot);
    return sample ? sample->detectedBpm : 0.0;
}

int PhraserAudioProcessor::getSamplePitchClass(int slot) const
{
    const auto sample = getSampleSnapshot(slot);
    return sample ? sample->detectedPitchClass : -1;
}

int PhraserAudioProcessor::getBasePitchClass() const
{
    return juce::jlimit(0, 11, static_cast<int>(
        std::round(parameters.getRawParameterValue("targetKey")->load())));
}

int PhraserAudioProcessor::getAutomaticTranspose(int slot) const
{
    const int detected = getSamplePitchClass(slot);
    if (detected < 0) return 0;
    int shift = (getBasePitchClass() - detected + 12) % 12;
    if (shift > 6) shift -= 12;
    return shift;
}

int PhraserAudioProcessor::getTransientCount(int slot) const
{
    auto sample = getSampleSnapshot(slot);
    return sample ? static_cast<int>(sample->transients.size()) : 0;
}

float PhraserAudioProcessor::getPlayPosition(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return -1.0f;
    return playPositions[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
}

int PhraserAudioProcessor::getStepsPerBar() const { return 16; }

int PhraserAudioProcessor::getActiveSteps() const
{
    const int bars = juce::jlimit(1, 4,
        static_cast<int>(std::round(parameters.getRawParameterValue("bars")->load())));
    return 16 * bars;
}

int PhraserAudioProcessor::getTrackCount() const
{
    const int bars = getActiveSteps() / 16;
    return bars == 1 ? 4 : (bars == 2 ? 2 : 1);
}

int PhraserAudioProcessor::getCellCount() const
{
    return getActiveSteps() * getTrackCount();
}

PhraserSlotSettings PhraserAudioProcessor::getSlotSettings(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return {};
    const juce::ScopedLock guard(settingsLock);
    return settings[static_cast<size_t>(slot)];
}

void PhraserAudioProcessor::setSlotSettings(int slot, PhraserSlotSettings value)
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return;
    value.startMin = juce::jlimit(0.0f, 1.0f, value.startMin);
    value.startMax = juce::jlimit(value.startMin, 1.0f, value.startMax);
    value.lengthMin = juce::jlimit(0.005f, 1.0f, value.lengthMin);
    value.lengthMax = juce::jlimit(value.lengthMin, 1.0f, value.lengthMax);
    value.pitchSemitones = juce::jlimit(-12, 12, value.pitchSemitones);
    value.manualBpm = (std::isfinite(value.manualBpm) && value.manualBpm >= 40.0
                       && value.manualBpm <= 240.0) ? value.manualBpm : 0.0;
    const juce::ScopedLock guard(settingsLock);
    settings[static_cast<size_t>(slot)] = value;
    fadeAtNextStepModes[static_cast<size_t>(slot)].store(value.fadeAtNextStep, std::memory_order_release);
    syncToHostModes[static_cast<size_t>(slot)].store(value.syncToHost, std::memory_order_release);
    manualTempos[static_cast<size_t>(slot)].store(value.manualBpm, std::memory_order_release);
    slotPitches[static_cast<size_t>(slot)].store(value.pitchSemitones, std::memory_order_release);
}

PhraserPattern PhraserAudioProcessor::getPatternSnapshot() const
{
    const auto p = std::atomic_load(&pattern);
    return p ? *p : PhraserPattern {};
}

void PhraserAudioProcessor::randomizePattern()
{
    auto next = std::make_shared<PhraserPattern>();
    const auto old = std::atomic_load(&pattern);
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto breaks = parameters.getRawParameterValue("breaks")->load();
    const auto size = parameters.getRawParameterValue("slice")->load();
    const auto variation = parameters.getRawParameterValue("variation")->load();
    const auto snap = parameters.getRawParameterValue("transient")->load();
    const int pitchSpread = juce::jlimit(0, 12,
        static_cast<int>(std::round(parameters.getRawParameterValue("pitchspread")->load())));
    const int active = getCellCount();
    const int subdivision = getStepsPerBar();
    const int phraseSteps = getActiveSteps();

    std::array<int, numSlots> loaded {};
    int count = 0;
    for (int i = 0; i < numSlots; ++i)
        if (getSampleSnapshot(i))
            loaded[static_cast<size_t>(count++)] = i;

    std::array<PhraserSlotSettings, numSlots> slotSettings;
    {
        const juce::ScopedLock guard(settingsLock);
        slotSettings = settings;
    }

    const int motif = variation < 0.33f ? subdivision / 2
                    : variation < 0.67f ? subdivision : phraseSteps;
    for (int i = 0; i < numSteps; ++i)
    {
        auto& e = next->events[static_cast<size_t>(i)];
        if (old && old->events[static_cast<size_t>(i)].locked)
        {
            e = old->events[static_cast<size_t>(i)];
            continue;
        }
        if (i >= active || count == 0)
        {
            e.enabled = false;
            continue;
        }

        const int withinTrack = i % phraseSteps;
        if (withinTrack >= motif && random.nextFloat() > variation)
        {
            e = next->events[static_cast<size_t>(i - motif)];
            e.locked = false;
            continue;
        }

        e.enabled = random.nextFloat() < density
            && !(withinTrack % subdivision >= subdivision / 2 && random.nextFloat() < breaks);
        e.slot = loaded[static_cast<size_t>(random.nextInt(count))];

        const auto& range = slotSettings[static_cast<size_t>(e.slot)];
        e.offset = range.startMin + random.nextFloat() * (range.startMax - range.startMin);
        // Each event receives a new independent duration from its sample's configured interval.
        e.length = range.lengthMin + random.nextFloat() * (range.lengthMax - range.lengthMin);
        // Size remains a usable global scaling control, centered at the v0.4 default 0.25.
        e.length = juce::jlimit(0.005f, 1.0f, e.length * size / 0.25f);
        e.semitones = pitchSpread > 0 ? random.nextInt(pitchSpread * 2 + 1) - pitchSpread : 0;

        const auto source = getSampleSnapshot(e.slot);
        if (source && !source->transients.empty() && random.nextFloat() < snap)
        {
            const auto& marks = source->transients;
            auto it = std::lower_bound(marks.begin(), marks.end(), e.offset);
            if (it == marks.end()) e.offset = marks.back();
            else if (it == marks.begin()) e.offset = *it;
            else e.offset = e.offset - *(it - 1) < *it - e.offset ? *(it - 1) : *it;
        }

        // Ensure a slice starting near the end still has a usable duration.
        e.offset = juce::jlimit(0.0f, 0.995f, e.offset);
    }

    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::toggleStep(int index)
{
    if (!juce::isPositiveAndBelow(index, getCellCount())) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    auto& e = next->events[static_cast<size_t>(index)];
    e.enabled = !e.enabled;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::toggleLock(int index)
{
    if (!juce::isPositiveAndBelow(index, getCellCount())) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    auto& e = next->events[static_cast<size_t>(index)];
    e.locked = !e.locked;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::setStepSlot(int index, int slot)
{
    if (!juce::isPositiveAndBelow(index, getCellCount())
        || !juce::isPositiveAndBelow(slot, numSlots)) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    auto& e = next->events[static_cast<size_t>(index)];
    e.slot = slot;
    e.enabled = true;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::setStepAt(int track, int column, int slot)
{
    if (!juce::isPositiveAndBelow(track, getTrackCount())
        || !juce::isPositiveAndBelow(column, getActiveSteps())) return;
    setStepSlot(getCellIndex(track, column, getActiveSteps()), slot);
}

void PhraserAudioProcessor::saveKeep(int index)
{
    if (!juce::isPositiveAndBelow(index, 2)) return;
    std::atomic_store(&keeps[static_cast<size_t>(index)], std::atomic_load(&pattern));
}

void PhraserAudioProcessor::recallKeep(int index)
{
    if (!juce::isPositiveAndBelow(index, 2)) return;
    auto kept = std::atomic_load(&keeps[static_cast<size_t>(index)]);
    if (kept) std::atomic_store(&pattern, kept);
}

// New steps no longer release previous voices. Each voice sustains for its
// independently generated slice duration (or until the sample end).
// When all 32 slots are occupied, the oldest voice is stolen.
void PhraserAudioProcessor::startStep(
    const SliceEvent& event,
    const std::array<std::shared_ptr<const PhraserSample>, numSlots>& available,
    double /*releaseFrames*/)
{
    if (!event.enabled || !juce::isPositiveAndBelow(event.slot, numSlots)) return;
    auto source = available[static_cast<size_t>(event.slot)];
    if (!source || source->audio.getNumSamples() < 2) return;

    int selected = -1;
    for (int i = 0; i < maxVoices; ++i)
        if (!voices[static_cast<size_t>(i)].source) { selected = i; break; }
    if (selected < 0)
    {
        // Oldest voice is preferred, so current musical material survives where possible.
        uint64_t oldest = std::numeric_limits<uint64_t>::max();
        for (int i = 0; i < maxVoices; ++i)
            if (voices[static_cast<size_t>(i)].serial < oldest)
            {
                oldest = voices[static_cast<size_t>(i)].serial;
                selected = i;
            }
    }

    Voice voice;
    voice.source = std::move(source);
    voice.slot = event.slot;
    voice.fadeAtNextStep = fadeAtNextStepModes[static_cast<size_t>(event.slot)].load(std::memory_order_acquire);
    voice.syncToHost = syncToHostModes[static_cast<size_t>(event.slot)].load(std::memory_order_acquire);
    voice.tempoBpm = manualTempos[static_cast<size_t>(event.slot)].load(std::memory_order_acquire);
    if (voice.tempoBpm <= 0.0) voice.tempoBpm = voice.source->detectedBpm;
    voice.pitchSemitones = slotPitches[static_cast<size_t>(event.slot)].load(std::memory_order_acquire);
    voice.serial = ++voiceSerial;
    const double last = static_cast<double>(voice.source->audio.getNumSamples() - 1);
    voice.position = juce::jlimit(0.0, last - 1.0, static_cast<double>(event.offset) * last);
    voice.sourceStart = voice.position;
    voice.end = juce::jmin(last,
        voice.position + static_cast<double>(event.length) * (last + 1.0));
    const double globalPitch = parameters.getRawParameterValue("pitch")->load();
    // Tune tonal samples to the chosen reference note. Percussive/ambiguous
    // samples have detectedPitchClass=-1 and are left unchanged.
    int automatic = 0;
    if (voice.source->detectedPitchClass >= 0)
    {
        automatic = (getBasePitchClass() - voice.source->detectedPitchClass + 12) % 12;
        if (automatic > 6) automatic -= 12;
    }
    const double semitones = juce::jlimit(-24.0, 24.0,
        static_cast<double>(event.semitones) + static_cast<double>(globalPitch)
        + static_cast<double>(voice.pitchSemitones) + automatic);
    // Source timeline moves only with musical tempo. Pitch changes the
    // internal grain playback rate, not the duration of a slice.
    voice.baseIncrement = voice.source->sampleRate / engineSampleRate;
    voice.pitchIncrement = voice.baseIncrement * std::pow(2.0, semitones / 12.0);
    voice.increment = voice.baseIncrement;
    if (voice.syncToHost && voice.tempoBpm > 0.0)
        voice.increment *= lastBpm.load(std::memory_order_relaxed) / voice.tempoBpm;
    voice.duration = juce::jmax(1.0, (voice.end - voice.position) / voice.increment);
    // Two overlapping 60ms grains, staggered every 30ms. Fixed-size
    // voice state means this introduces no buffer allocation per trigger.
    voice.grainHopFrames = juce::jmax(32, static_cast<int>(engineSampleRate * 0.03));
    voice.grainStarts[0] = voice.sourceStart;
    voice.grainStarts[1] = voice.sourceStart
        - voice.grainHopFrames * voice.pitchIncrement;
    voice.currentGrainHop = -1;
    voices[static_cast<size_t>(selected)] = std::move(voice);
}

void PhraserAudioProcessor::publishVoiceMeters()
{
    std::array<int, numSlots> counts {};
    std::array<float, numSlots> positions {};
    positions.fill(-1.0f);
    int total = 0;
    for (const auto& voice : voices)
    {
        if (!voice.source || !juce::isPositiveAndBelow(voice.slot, numSlots)) continue;
        const auto slot = static_cast<size_t>(voice.slot);
        ++counts[slot];
        ++total;
        // Furthest-progressed active sample is indicated on each waveform.
        positions[slot] = juce::jmax(positions[slot],
            static_cast<float>(voice.position / voice.source->audio.getNumSamples()));
    }
    for (int i = 0; i < numSlots; ++i)
    {
        slotVoiceCounts[static_cast<size_t>(i)].store(
            counts[static_cast<size_t>(i)], std::memory_order_relaxed);
        playPositions[static_cast<size_t>(i)].store(
            positions[static_cast<size_t>(i)], std::memory_order_relaxed);
    }
    activeVoiceCount.store(total, std::memory_order_relaxed);
}

void PhraserAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    buffer.clear();
    if (engineSampleRate <= 0.0) return;

    const auto localPattern = std::atomic_load(&pattern);
    if (!localPattern) return;
    std::array<std::shared_ptr<const PhraserSample>, numSlots> available;
    for (int i = 0; i < numSlots; ++i)
        available[static_cast<size_t>(i)] = std::atomic_load(&samples[static_cast<size_t>(i)]);

    const auto* host = getPlayHead();
    if (host == nullptr)
    {
        previousStep = std::numeric_limits<int64_t>::min();
        currentStep.store(-1, std::memory_order_relaxed);
        for (auto& voice : voices) voice = {};
        publishVoiceMeters();
        return;
    }
    const auto position = host->getPosition();
    if (!position || !position->getIsPlaying())
    {
        previousStep = std::numeric_limits<int64_t>::min();
        currentStep.store(-1, std::memory_order_relaxed);
        for (auto& voice : voices) voice = {};
        for (auto& indicator : playPositions) indicator.store(-1.0f, std::memory_order_relaxed);
        return;
    }

    const auto ppq = position->getPpqPosition();
    const double bpm = position->getBpm().orFallback(120.0);
    if (!ppq || bpm <= 0.0) return;
    lastBpm.store(bpm, std::memory_order_relaxed);
    // Recalculate play rate for already-sounding tempo-synchronized voices.
    // Free-running voices retain their original pitch/speed.
    for (auto& voice : voices)
        if (voice.source && voice.syncToHost && voice.tempoBpm > 0.0)
        {
            voice.increment = voice.baseIncrement * bpm / voice.tempoBpm;
            voice.duration = voice.rendered + juce::jmax(
                0.0, (voice.end - voice.position) / voice.increment);
        }

    const int subdivision = getStepsPerBar();
    const int active = getActiveSteps();
    const double stepAdvance = (bpm / 60.0 * subdivision / 4.0) / engineSampleRate;
    const double baseStep = *ppq * (subdivision / 4.0);
    const double swing = parameters.getRawParameterValue("swing")->load();
    const double fadeFrames = juce::jmax(1.0,
        parameters.getRawParameterValue("fade")->load() * engineSampleRate);
    const float gain = parameters.getRawParameterValue("gain")->load();
    const double crossfadeFrames = juce::jmax(1.0, 0.005 * engineSampleRate);

    for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
    {
        const double sequencePosition = baseStep + frame * stepAdvance;
        const auto whole = static_cast<int64_t>(std::floor(sequencePosition));
        const double fraction = sequencePosition - static_cast<double>(whole);
        const int64_t step = ((whole & 1LL) && fraction < swing) ? whole - 1 : whole;

        if (step != previousStep)
        {
            previousStep = step;
            const int column = static_cast<int>((step % active + active) % active);
            currentStep.store(column, std::memory_order_relaxed);
            // Release only voices whose *sample slot* is configured for NEXT STEP 5ms.
            // FULL LENGTH voices remain independent and can overlap for their entire slice.
            // This happens on rests too, as the phrase advances to the next step.
            for (auto& voice : voices)
                if (voice.source && voice.fadeAtNextStep && voice.releaseStart < 0.0)
                {
                    voice.releaseStart = voice.rendered;
                    voice.releaseFrames = crossfadeFrames;
                }
            // Each lane is a parallel track, with its own event at this
            // absolute step. All tracks can trigger at the same instant.
            const int tracks = getTrackCount();
            for (int track = 0; track < tracks; ++track)
            {
                const int cell = getCellIndex(track, column, active);
                startStep(localPattern->events[static_cast<size_t>(cell)],
                          available, crossfadeFrames);
            }
        }

        for (auto& voice : voices)
        {
            if (!voice.source) continue;
            const auto& audio = voice.source->audio;
            if (voice.position >= voice.end
                || voice.position >= audio.getNumSamples() - 1)
            {
                voice = {};
                continue;
            }

            double envelope = juce::jlimit(0.0, 1.0,
                juce::jmin(voice.rendered / fadeFrames,
                           (voice.duration - voice.rendered) / fadeFrames));
            if (voice.releaseStart >= 0.0)
            {
                const double released = voice.rendered - voice.releaseStart;
                if (released >= voice.releaseFrames)
                {
                    voice = {};
                    continue;
                }
                envelope *= juce::jlimit(0.0, 1.0,
                    1.0 - released / voice.releaseFrames);
            }

            // 50%-overlap granular OLA: the two read heads advance at
            // the requested pitch, while their anchors follow the tempo
            // timeline. Thus Live BPM changes do not transpose the source.
            const int hop = voice.grainHopFrames;
            const auto grain = static_cast<int64_t>(voice.rendered) / hop;
            const double age = voice.rendered - static_cast<double>(grain) * hop;
            if (grain != voice.currentGrainHop)
            {
                voice.currentGrainHop = grain;
                auto start = voice.position;
                if (grain > 0 && std::abs(voice.increment - voice.pitchIncrement) > 1.0e-5)
                {
                    const auto earlier = static_cast<size_t>(1LL - (grain & 1LL));
                    start = phraser::alignGrainStart(
                        audio, start, voice.grainStarts[earlier],
                        voice.pitchIncrement, hop, voice.source->sampleRate,
                        voice.sourceStart,
                        juce::jmin(voice.end - 0.0001,
                                   static_cast<double>(audio.getNumSamples() - 1)));
                }
                voice.grainStarts[static_cast<size_t>(grain & 1LL)] = start;
            }
            const auto current = static_cast<size_t>(grain & 1LL);
            const auto previous = 1U - current;
            const double fresh = voice.grainStarts[current]
                + age * voice.pitchIncrement;
            const double older = voice.grainStarts[previous]
                + (age + hop) * voice.pitchIncrement;
            const double t = juce::jlimit(0.0, 1.0, age / hop);
            const float freshWeight = static_cast<float>(t * t * (3.0 - 2.0 * t));
            const float olderWeight = 1.0f - freshWeight;
            const double highest = juce::jmin(
                voice.end - 0.0001, static_cast<double>(audio.getNumSamples() - 1));
            const float amp = static_cast<float>(envelope) * gain;

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                const int srcChannel = juce::jmin(channel, audio.getNumChannels() - 1);
                const auto* data = audio.getReadPointer(srcChannel);
                auto sampleAt = [&](double index)
                {
                    return phraser::grainSampleAt(
                        data, audio.getNumSamples(), index,
                        voice.sourceStart, juce::jmax(voice.sourceStart, highest));
                };
                const float value = sampleAt(fresh) * freshWeight
                    + sampleAt(older) * olderWeight;
                buffer.addSample(channel, frame, value * amp);
            }
            voice.position += voice.increment;
            voice.rendered += 1.0;
        }
    }

    publishVoiceMeters();
}

juce::AudioProcessorEditor* PhraserAudioProcessor::createEditor()
{
    return new PhraserAudioProcessorEditor(*this);
}

void PhraserAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto xml = parameters.copyState().createXml();
    if (!xml) return;
    xml->setAttribute("phraserVersion", 8);
    std::array<PhraserSlotSettings, numSlots> ranges;
    {
        const juce::ScopedLock guard(settingsLock);
        ranges = settings;
    }
    for (int i = 0; i < numSlots; ++i)
    {
        auto loaded = getSampleSnapshot(i);
        auto* slot = xml->createNewChildElement("SampleSlot");
        slot->setAttribute("index", i);
        slot->setAttribute("path", loaded ? loaded->sourcePath : juce::String());
        const auto& v = ranges[static_cast<size_t>(i)];
        slot->setAttribute("startMin", v.startMin);
        slot->setAttribute("startMax", v.startMax);
        slot->setAttribute("lengthMin", v.lengthMin);
        slot->setAttribute("lengthMax", v.lengthMax);
        slot->setAttribute("fadeAtNextStep", v.fadeAtNextStep);
        slot->setAttribute("syncToHost", v.syncToHost);
        slot->setAttribute("manualBpm", v.manualBpm);
        slot->setAttribute("pitchSemitones", v.pitchSemitones);
    }

    auto* phrase = xml->createNewChildElement("Phrase");
    for (const auto& e : getPatternSnapshot().events) writeStep(*phrase, e);
    for (int i = 0; i < 2; ++i)
    {
        const auto kept = std::atomic_load(&keeps[static_cast<size_t>(i)]);
        if (!kept) continue;
        auto* save = xml->createNewChildElement("Keep");
        save->setAttribute("index", i);
        for (const auto& e : kept->events) writeStep(*save, e);
    }
    copyXmlToBinary(*xml, destination);
}

void PhraserAudioProcessor::setStateInformation(const void* data, int size)
{
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(parameters.state.getType())) return;

    // Backward compatible with v0.2-v0.4: old 'slotN' attributes and 16 Step children.
    for (int i = 0; i < numSlots; ++i)
    {
        auto* slotElement = [&]() -> juce::XmlElement*
        {
            for (auto* element : xml->getChildIterator())
                if (element->hasTagName("SampleSlot")
                    && element->getIntAttribute("index", -1) == i)
                    return element;
            return nullptr;
        }();
        const auto path = slotElement
            ? slotElement->getStringAttribute("path")
            : xml->getStringAttribute("slot" + juce::String(i));
        PhraserSlotSettings range;
        if (slotElement)
        {
            range.startMin = static_cast<float>(slotElement->getDoubleAttribute("startMin", 0.0));
            range.startMax = static_cast<float>(slotElement->getDoubleAttribute("startMax", 1.0));
            range.lengthMin = static_cast<float>(slotElement->getDoubleAttribute("lengthMin", 0.08));
            range.lengthMax = static_cast<float>(slotElement->getDoubleAttribute("lengthMax", 0.35));
            range.fadeAtNextStep = slotElement->getBoolAttribute("fadeAtNextStep", false);
            range.syncToHost = slotElement->getBoolAttribute("syncToHost", false);
            range.manualBpm = slotElement->getDoubleAttribute("manualBpm", 0.0);
            range.pitchSemitones = slotElement->getIntAttribute("pitchSemitones", 0);
        }
        setSlotSettings(i, range);

        const juce::File file(path);
        if (path.isNotEmpty() && file.existsAsFile()) loadWavFile(i, file);
        else std::atomic_store(&samples[static_cast<size_t>(i)],
                               std::shared_ptr<const PhraserSample> {});
    }

    if (auto* phrase = xml->getChildByName("Phrase"))
        std::atomic_store(&pattern,
            std::shared_ptr<const PhraserPattern>(std::make_shared<PhraserPattern>(readPattern(*phrase))));

    for (auto& keep : keeps)
        std::atomic_store(&keep, std::shared_ptr<const PhraserPattern> {});
    for (auto* child : xml->getChildIterator())
    {
        if (!child->hasTagName("Keep")) continue;
        const int index = child->getIntAttribute("index", -1);
        if (juce::isPositiveAndBelow(index, 2))
            std::atomic_store(&keeps[static_cast<size_t>(index)],
                std::shared_ptr<const PhraserPattern>(
                    std::make_shared<PhraserPattern>(readPattern(*child))));
    }

    // ValueTree owns only the parameter children, not our sample / phrase / keep XML.
    auto stateXml = juce::parseXML(xml->toString());
    if (!stateXml) return;
    for (int i = stateXml->getNumChildElements() - 1; i >= 0; --i)
    {
        auto* child = stateXml->getChildElement(i);
        if (child->hasTagName("Phrase") || child->hasTagName("Keep")
            || child->hasTagName("SampleSlot"))
            stateXml->removeChildElement(child, true);
    }
    parameters.replaceState(juce::ValueTree::fromXml(*stateXml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhraserAudioProcessor();
}
