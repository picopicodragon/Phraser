#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TransientDetection.h"
#include <cmath>
#include <limits>

juce::AudioProcessorValueTreeState::ParameterLayout PhraserAudioProcessor::makeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto add = [&params](const char* id, const char* title, float low, float high, float initial)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, title,
            juce::NormalisableRange<float> { low, high }, initial));
    };
    add("density", "Density", 0.0f, 1.0f, 0.8f);
    add("slice", "Slice Size", 0.02f, 1.0f, 0.25f);
    add("breaks", "Breaks", 0.0f, 1.0f, 0.2f);
    add("swing", "Swing", 0.0f, 0.75f, 0.0f);
    add("gain", "Gain", 0.0f, 1.0f, 0.7f);
    add("fade", "Fade", 0.001f, 0.03f, 0.006f);
    add("transient", "Transient Snap", 0.0f, 1.0f, 1.0f);
    add("variation", "Variation", 0.0f, 1.0f, 0.5f);
    add("pitch", "Pitch (semitones)", -12.0f, 12.0f, 0.0f);
    add("pitchspread", "Pitch Spread", 0.0f, 12.0f, 0.0f);
    return { params.begin(), params.end() };
}

PhraserAudioProcessor::PhraserAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", makeParameters())
{
    formatManager.registerBasicFormats();
    randomizePattern();
}

bool PhraserAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layout.getMainInputChannelSet().isDisabled();
}

void PhraserAudioProcessor::prepareToPlay(double newRate, int)
{
    engineSampleRate = newRate;
    previousStep = std::numeric_limits<int64_t>::min();
    for (auto& voice : voices) voice = {};
    nextVoice = 0;
    currentStep.store(-1, std::memory_order_relaxed);
}

bool PhraserAudioProcessor::loadWavFile(int slot, const juce::File& file)
{
    if (!juce::isPositiveAndBelow(slot, numSlots) || !file.existsAsFile()
        || !file.hasFileExtension("wav"))
        return false;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (!reader || reader->numChannels < 1 || reader->sampleRate <= 0.0
        || reader->lengthInSamples < 2
        || reader->lengthInSamples > 20000000)
        return false;

    auto loaded = std::make_shared<PhraserSample>();
    loaded->audio.setSize(juce::jmin(2, static_cast<int>(reader->numChannels)),
                          static_cast<int>(reader->lengthInSamples));
    if (!reader->read(&loaded->audio, 0, loaded->audio.getNumSamples(), 0, true, true))
        return false;
    loaded->sampleRate = reader->sampleRate;
    loaded->sourcePath = file.getFullPathName();
    loaded->transients = phraser::detectTransients(loaded->audio, loaded->sampleRate);
    std::atomic_store(&samples[static_cast<size_t>(slot)],
                      std::shared_ptr<const PhraserSample>(loaded));
    return true;
}

juce::String PhraserAudioProcessor::getSlotName(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots))
        return {};
    auto source = std::atomic_load(&samples[static_cast<size_t>(slot)]);
    return source ? juce::File(source->sourcePath).getFileName() : "(empty)";
}

int PhraserAudioProcessor::getTransientCount(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, numSlots)) return 0;
    const auto loaded = std::atomic_load(&samples[static_cast<size_t>(slot)]);
    return loaded ? static_cast<int>(loaded->transients.size()) : 0;
}

PhraserPattern PhraserAudioProcessor::getPatternSnapshot() const
{
    auto p = std::atomic_load(&pattern);
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
    const auto spread = parameters.getRawParameterValue("pitchspread")->load();
    std::array<int, numSlots> loaded {};
    int count = 0;
    for (int i = 0; i < numSlots; ++i)
        if (std::atomic_load(&samples[static_cast<size_t>(i)]))
            loaded[static_cast<size_t>(count++)] = i;

    const int anchor = count > 0 ? loaded[static_cast<size_t>(random.nextInt(count))] : 0;
    float anchorOffset = random.nextFloat() * 0.9f;
    const int motifPeriod = variation < 0.33f ? 4 : (variation < 0.67f ? 8 : 16);
    for (int i = 0; i < numSteps; ++i)
    {
        auto& e = next->events[static_cast<size_t>(i)];
        if (old && old->events[static_cast<size_t>(i)].locked)
        {
            e = old->events[static_cast<size_t>(i)];
            continue;
        }

        const bool rhythmicBreak = (i % 4 >= 2 && random.nextFloat() < breaks);
        e.enabled = random.nextFloat() < density && !rhythmicBreak;
        const bool repeatMotif = i >= motifPeriod && random.nextFloat() > variation;
        if (repeatMotif)
        {
            e = next->events[static_cast<size_t>(i - motifPeriod)];
            e.locked = false;
            continue;
        }

        e.slot = count > 0 ? loaded[static_cast<size_t>(random.nextInt(count))] : anchor;
        e.length = size;
        const int maxSpread = juce::jlimit(0, 12, static_cast<int>(std::round(spread)));
        e.semitones = maxSpread > 0 ? random.nextInt(2 * maxSpread + 1) - maxSpread : 0;
        e.offset = (variation < 0.25f && (i % 4) != 0)
            ? anchorOffset : random.nextFloat() * 0.94f;
        if (i % 4 == 0) anchorOffset = e.offset;

        const auto source = std::atomic_load(&samples[static_cast<size_t>(e.slot)]);
        if (source && !source->transients.empty() && random.nextFloat() < snap)
        {
            const auto& onsets = source->transients;
            const auto it = std::lower_bound(onsets.begin(), onsets.end(), e.offset);
            if (it == onsets.end()) e.offset = onsets.back();
            else if (it == onsets.begin()) e.offset = *it;
            else
            {
                const auto earlier = *(it - 1);
                e.offset = (e.offset - earlier < *it - e.offset) ? earlier : *it;
            }
        }
    }
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::toggleStep(int index)
{
    if (!juce::isPositiveAndBelow(index, numSteps)) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    auto& e = next->events[static_cast<size_t>(index)];
    e.enabled = !e.enabled;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::toggleLock(int index)
{
    if (!juce::isPositiveAndBelow(index, numSteps)) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    auto& e = next->events[static_cast<size_t>(index)];
    e.locked = !e.locked;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::setStepSlot(int step, int slot)
{
    if (!juce::isPositiveAndBelow(step, numSteps) || !juce::isPositiveAndBelow(slot, numSlots)) return;
    auto next = std::make_shared<PhraserPattern>(getPatternSnapshot());
    next->events[static_cast<size_t>(step)].slot = slot;
    next->events[static_cast<size_t>(step)].enabled = true;
    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::saveKeep(int index)
{
    if (!juce::isPositiveAndBelow(index, 2)) return;
    auto snapshot = std::atomic_load(&pattern);
    std::atomic_store(&keeps[static_cast<size_t>(index)], snapshot);
}

void PhraserAudioProcessor::recallKeep(int index)
{
    if (!juce::isPositiveAndBelow(index, 2)) return;
    auto remembered = std::atomic_load(&keeps[static_cast<size_t>(index)]);
    if (remembered) std::atomic_store(&pattern, remembered);
}

void PhraserAudioProcessor::startStep(
    const SliceEvent& event,
    const std::array<std::shared_ptr<const PhraserSample>, numSlots>& available,
    double releaseFrames)
{
    // Existing voices fade out when the next step begins, including rests.
    for (auto& voice : voices)
        if (voice.source && voice.releaseStart < 0.0)
        {
            voice.releaseStart = voice.rendered;
            voice.releaseFrames = releaseFrames;
        }

    if (!event.enabled || !juce::isPositiveAndBelow(event.slot, numSlots))
        return;

    const auto source = available[static_cast<size_t>(event.slot)];
    if (!source || source->audio.getNumSamples() < 2)
        return;

    // Prefer a free voice; otherwise replace the oldest voice in the ring.
    int selected = -1;
    for (int i = 0; i < maxVoices; ++i)
        if (!voices[static_cast<size_t>(i)].source)
        {
            selected = i;
            break;
        }
    if (selected < 0)
    {
        selected = nextVoice;
        nextVoice = (nextVoice + 1) % maxVoices;
    }

    Voice v;
    v.source = source;
    const double last = static_cast<double>(source->audio.getNumSamples() - 1);
    v.position = juce::jlimit(0.0, last - 1.0, event.offset * last);
    v.end = juce::jmin(last, v.position + event.length * (last + 1.0));
    const auto globalPitch = parameters.getRawParameterValue("pitch")->load();
    const auto semitones = juce::jlimit(-24.0, 24.0,
        static_cast<double>(event.semitones) + static_cast<double>(globalPitch));
    // Resampling pitch changes playback speed/duration; not a time-stretch algorithm.
    v.increment = source->sampleRate / engineSampleRate * std::pow(2.0, semitones / 12.0);
    v.duration = juce::jmax(1.0, (v.end - v.position) / v.increment);
    voices[static_cast<size_t>(selected)] = std::move(v);
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
    if (!host) return;
    const auto position = host->getPosition();
    if (!position || !position->getIsPlaying())
    {
        previousStep = std::numeric_limits<int64_t>::min();
        currentStep.store(-1, std::memory_order_relaxed);
        for (auto& voice : voices) voice = {};
        return;
    }

    const auto ppq = position->getPpqPosition();
    const auto bpm = position->getBpm().orFallback(120.0);
    if (!ppq || bpm <= 0.0) return;

    const double stepAdvance = (bpm / 60.0 * 4.0) / engineSampleRate;
    const double baseStep = *ppq * 4.0;
    const double swing = parameters.getRawParameterValue("swing")->load();
    const float fade = parameters.getRawParameterValue("fade")->load();
    const float gain = parameters.getRawParameterValue("gain")->load();
    const double fadeFrames = juce::jmax(1.0, static_cast<double>(fade) * engineSampleRate);
    const double crossfadeFrames = juce::jmax(1.0, static_cast<double>(0.005) * engineSampleRate);

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const double positionInSteps = baseStep + sampleIndex * stepAdvance;
        const auto whole = static_cast<int64_t>(std::floor(positionInSteps));
        const double fraction = positionInSteps - static_cast<double>(whole);
        const int64_t step = ((whole & 1LL) != 0 && fraction < swing)
            ? whole - 1 : whole;

        if (step != previousStep)
        {
            previousStep = step;
            currentStep.store(static_cast<int>((step % numSteps + numSteps) % numSteps), std::memory_order_relaxed);
            const int index = static_cast<int>((step % numSteps + numSteps) % numSteps);
            startStep(localPattern->events[static_cast<size_t>(index)],
                      available, crossfadeFrames);
        }

        for (auto& voice : voices)
        {
            if (!voice.source) continue;
            const auto& audio = voice.source->audio;
            const int frame = static_cast<int>(voice.position);
            if (voice.position >= voice.end || frame + 1 >= audio.getNumSamples())
            {
                voice = {};
                continue;
            }

            // Natural start/end fades plus short release on the next trigger.
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

            const float fractionInSample = static_cast<float>(voice.position - frame);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                const auto sourceChannel = juce::jmin(channel, audio.getNumChannels() - 1);
                const auto* data = audio.getReadPointer(sourceChannel);
                const float value = data[frame] + (data[frame + 1] - data[frame]) * fractionInSample;
                buffer.addSample(channel, sampleIndex,
                                 value * static_cast<float>(envelope) * gain);
            }
            voice.position += voice.increment;
            voice.rendered += 1.0;
        }
    }
}

juce::AudioProcessorEditor* PhraserAudioProcessor::createEditor()
{
    return new PhraserAudioProcessorEditor(*this);
}

void PhraserAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto xml = parameters.copyState().createXml();
    if (!xml) return;
    xml->setAttribute("phraserVersion", 4);
    for (int i = 0; i < numSlots; ++i)
    {
        auto loaded = std::atomic_load(&samples[static_cast<size_t>(i)]);
        xml->setAttribute("slot" + juce::String(i),
                          loaded ? loaded->sourcePath : juce::String());
    }
    auto* phrase = xml->createNewChildElement("Phrase");
    const auto snapshot = getPatternSnapshot();
    for (int i = 0; i < numSteps; ++i)
    {
        const auto& e = snapshot.events[static_cast<size_t>(i)];
        auto* element = phrase->createNewChildElement("Step");
        element->setAttribute("on", e.enabled);
        element->setAttribute("hold", e.locked);
        element->setAttribute("slot", e.slot);
        element->setAttribute("offset", e.offset);
        element->setAttribute("length", e.length);
        element->setAttribute("pitch", e.semitones);
    }
    for (int k = 0; k < 2; ++k)
    {
        const auto kept = std::atomic_load(&keeps[static_cast<size_t>(k)]);
        if (!kept) continue;
        auto* memory = xml->createNewChildElement("Keep");
        memory->setAttribute("index", k);
        for (const auto& e : kept->events)
        {
            auto* el = memory->createNewChildElement("Step");
            el->setAttribute("on", e.enabled);
            el->setAttribute("hold", e.locked);
            el->setAttribute("slot", e.slot);
            el->setAttribute("offset", e.offset);
            el->setAttribute("length", e.length);
            el->setAttribute("pitch", e.semitones);
        }
    }
    copyXmlToBinary(*xml, destination);
}

void PhraserAudioProcessor::setStateInformation(const void* data, int size)
{
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(parameters.state.getType())) return;

    // Sample paths are saved rather than embedding potentially large WAV files.
    // Note: synchronous disk reload on host state restore is a prototype limitation.
    for (int i = 0; i < numSlots; ++i)
    {
        const juce::File file(xml->getStringAttribute("slot" + juce::String(i)));
        if (file.existsAsFile())
            loadWavFile(i, file);
        else
            std::atomic_store(&samples[static_cast<size_t>(i)],
                              std::shared_ptr<const PhraserSample> {});
    }

    if (auto* phrase = xml->getChildByName("Phrase"))
    {
        if (phrase->getNumChildElements() == numSteps)
        {
            auto restored = std::make_shared<PhraserPattern>();
            for (int i = 0; i < numSteps; ++i)
            {
                auto* element = phrase->getChildElement(i);
                auto& e = restored->events[static_cast<size_t>(i)];
                e.enabled = element->getBoolAttribute("on", true);
                e.locked = element->getBoolAttribute("hold", false);
                e.slot = juce::jlimit(0, numSlots - 1, element->getIntAttribute("slot", 0));
                e.offset = juce::jlimit(0.0f, 0.99f,
                                      static_cast<float>(element->getDoubleAttribute("offset", 0)));
                e.length = juce::jlimit(0.02f, 1.0f,
                                      static_cast<float>(element->getDoubleAttribute("length", 0.25)));
                e.semitones = juce::jlimit(-12, 12, element->getIntAttribute("pitch", 0));
            }
            std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(restored));
        }
    }

    for (auto* keep : xml->getChildIterator())
    {
        if (!keep->hasTagName("Keep")) continue;
        const int k = keep->getIntAttribute("index", -1);
        if (!juce::isPositiveAndBelow(k, 2) || keep->getNumChildElements() != numSteps) continue;
        auto remembered = std::make_shared<PhraserPattern>();
        for (int i = 0; i < numSteps; ++i)
        {
            const auto* el = keep->getChildElement(i);
            auto& e = remembered->events[static_cast<size_t>(i)];
            e.enabled = el->getBoolAttribute("on", true);
            e.locked = el->getBoolAttribute("hold", false);
            e.slot = juce::jlimit(0, numSlots - 1, el->getIntAttribute("slot", 0));
            e.offset = juce::jlimit(0.0f, 0.99f, static_cast<float>(el->getDoubleAttribute("offset", 0)));
            e.length = juce::jlimit(0.02f, 1.0f, static_cast<float>(el->getDoubleAttribute("length", 0.25)));
            e.semitones = juce::jlimit(-12, 12, el->getIntAttribute("pitch", 0));
        }
        std::atomic_store(&keeps[static_cast<size_t>(k)], std::shared_ptr<const PhraserPattern>(remembered));
    }

    // Remove our custom children before passing the ValueTree to APVTS.
    auto stateXml = juce::parseXML(xml->toString());
    if (stateXml)
    {
        if (auto* phrase = stateXml->getChildByName("Phrase"))
            stateXml->removeChildElement(phrase, true);
        for (int i = stateXml->getNumChildElements() - 1; i >= 0; --i)
            if (stateXml->getChildElement(i)->hasTagName("Keep"))
                stateXml->removeChildElement(stateXml->getChildElement(i), true);
        parameters.replaceState(juce::ValueTree::fromXml(*stateXml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhraserAudioProcessor();
}
