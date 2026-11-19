#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    activeSample.reset();
    voicePosition = voiceEnd = 0.0;
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

PhraserPattern PhraserAudioProcessor::getPatternSnapshot() const
{
    auto p = std::atomic_load(&pattern);
    return p ? *p : PhraserPattern {};
}

void PhraserAudioProcessor::randomizePattern()
{
    auto next = std::make_shared<PhraserPattern>();
    auto old = std::atomic_load(&pattern);
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto breaks = parameters.getRawParameterValue("breaks")->load();
    const auto size = parameters.getRawParameterValue("slice")->load();
    std::array<int, numSlots> loaded {};
    int count = 0;
    for (int i = 0; i < numSlots; ++i)
        if (std::atomic_load(&samples[static_cast<size_t>(i)]))
            loaded[static_cast<size_t>(count++)] = i;

    for (int i = 0; i < numSteps; ++i)
    {
        auto& e = next->events[static_cast<size_t>(i)];
        if (old && old->events[static_cast<size_t>(i)].locked)
        {
            e = old->events[static_cast<size_t>(i)];
            continue;
        }
        // Consecutive pairs create occasional breaks, while density handles individual rests.
        const bool rest = random.nextFloat() >= density
            || ((i % 4) >= 2 && random.nextFloat() < breaks);
        e.enabled = !rest;
        e.slot = count > 0 ? loaded[static_cast<size_t>(random.nextInt(count))] : 0;
        e.offset = random.nextFloat() * 0.95f;
        e.length = size;
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

void PhraserAudioProcessor::startStep(
    const SliceEvent& event,
    const std::array<std::shared_ptr<const PhraserSample>, numSlots>& available)
{
    activeSample.reset();
    if (!event.enabled || !juce::isPositiveAndBelow(event.slot, numSlots))
        return;

    activeSample = available[static_cast<size_t>(event.slot)];
    if (!activeSample) return;
    const auto total = activeSample->audio.getNumSamples();
    if (total < 2) { activeSample.reset(); return; }

    voiceStart = juce::jlimit(0.0, static_cast<double>(total - 2),
                              static_cast<double>(event.offset) * (total - 1));
    voicePosition = voiceStart;
    voiceEnd = juce::jmin(static_cast<double>(total - 1),
                         voiceStart + static_cast<double>(event.length) * total);
    voiceIncrement = activeSample->sampleRate / engineSampleRate;
    voiceRendered = 0.0;
    voiceDuration = juce::jmax(1.0, (voiceEnd - voiceStart) / voiceIncrement);
}

void PhraserAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    buffer.clear();
    if (engineSampleRate <= 0.0) return;
    auto localPattern = std::atomic_load(&pattern);
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
        activeSample.reset();
        return;
    }

    const auto ppq = position->getPpqPosition();
    const auto bpm = position->getBpm().orFallback(120.0);
    if (!ppq || bpm <= 0.0) return;

    const double stepsPerSample = (bpm / 60.0 * 4.0) / engineSampleRate;
    const double baseStep = *ppq * 4.0;
    const double swing = parameters.getRawParameterValue("swing")->load();
    const float fadeSeconds = parameters.getRawParameterValue("fade")->load();
    const float gain = parameters.getRawParameterValue("gain")->load();
    const double fadeFrames = juce::jmax(1.0, static_cast<double>(fadeSeconds) * engineSampleRate);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const double straight = baseStep + i * stepsPerSample;
        // Swing delays odd sixteenth notes; event threshold is at n + swing for odd n.
        const int64_t integer = static_cast<int64_t>(std::floor(straight));
        const double fraction = straight - static_cast<double>(integer);
        const int64_t currentStep = ((integer & 1LL) != 0 && fraction < swing)
            ? integer - 1 : integer;

        if (currentStep != previousStep)
        {
            previousStep = currentStep;
            const int slot = static_cast<int>((currentStep % numSteps + numSteps) % numSteps);
            startStep(localPattern->events[static_cast<size_t>(slot)], available);
        }

        if (!activeSample || voicePosition >= voiceEnd) continue;
        const auto& audio = activeSample->audio;
        const int index = static_cast<int>(voicePosition);
        if (index + 1 >= audio.getNumSamples()) continue;
        const float frac = static_cast<float>(voicePosition - index);
        const double fade = juce::jmin(1.0,
            juce::jmin(voiceRendered / fadeFrames,
                       (voiceDuration - voiceRendered) / fadeFrames));
        const auto envelope = static_cast<float>(juce::jmax(0.0, fade)) * gain;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const int sc = juce::jmin(channel, audio.getNumChannels() - 1);
            const float* data = audio.getReadPointer(sc);
            const float value = data[index] + (data[index + 1] - data[index]) * frac;
            buffer.setSample(channel, i, value * envelope);
        }
        voicePosition += voiceIncrement;
        voiceRendered += 1.0;
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
    xml->setAttribute("phraserVersion", 2);
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
            }
            std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(restored));
        }
    }

    // Remove our custom child before passing the ValueTree to APVTS.
    auto stateXml = juce::parseXML(xml->toString());
    if (stateXml)
    {
        if (auto* phrase = stateXml->getChildByName("Phrase"))
            stateXml->removeChildElement(phrase, true);
        parameters.replaceState(juce::ValueTree::fromXml(*stateXml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhraserAudioProcessor();
}
