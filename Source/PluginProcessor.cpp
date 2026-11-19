#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout PhraserAudioProcessor::makeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 }, "Density",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "slice", 1 }, "Slice Size",
        juce::NormalisableRange<float> { 0.02f, 1.0f }, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> { 0.0f, 1.5f }, 0.8f));
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

void PhraserAudioProcessor::prepareToPlay(double newSampleRate, int)
{
    engineSampleRate = newSampleRate;
    previousStep = -1;
    activeSample.reset();
    voicePosition = voiceEnd = 0.0;
}

bool PhraserAudioProcessor::loadWavFile(const juce::File& file)
{
    if (!file.hasFileExtension("wav"))
        return false;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples < 1 || reader->numChannels < 1
        || reader->sampleRate <= 0.0 || reader->lengthInSamples > 44100LL * 60LL * 10LL)
        return false;

    auto loaded = std::make_shared<PhraserSample>();
    loaded->audio.setSize(juce::jmin(2, static_cast<int>(reader->numChannels)),
                          static_cast<int>(reader->lengthInSamples));
    if (!reader->read(&loaded->audio, 0, loaded->audio.getNumSamples(), 0, true, true))
        return false;
    loaded->sampleRate = reader->sampleRate;
    std::atomic_store(&sample, std::shared_ptr<const PhraserSample>(loaded));
    return true;
}

void PhraserAudioProcessor::randomizePattern()
{
    auto next = std::make_shared<PhraserPattern>();
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto slice = parameters.getRawParameterValue("slice")->load();

    for (auto& event : next->events)
    {
        event.enabled = random.nextFloat() < density;
        event.offset = random.nextFloat() * 0.92f;
        event.length = slice;
    }

    std::atomic_store(&pattern, std::shared_ptr<const PhraserPattern>(next));
}

void PhraserAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto localSample = std::atomic_load(&sample);
    const auto localPattern = std::atomic_load(&pattern);
    if (!localSample || !localPattern || engineSampleRate <= 0.0)
        return;

    const auto* hostPlayHead = getPlayHead();
    if (hostPlayHead == nullptr)
        return;

    const auto position = hostPlayHead->getPosition();
    if (!position || !position->getIsPlaying())
    {
        previousStep = -1;
        voicePosition = voiceEnd = 0.0;
        return;
    }
    const auto bpm = position->getBpm().orFallback(120.0);
    const auto ppq = position->getPpqPosition();
    if (!ppq || bpm <= 0.0)
        return;

    const auto& audio = localSample->audio;
    const int total = audio.getNumSamples();
    if (total < 2)
        return;

    const auto gain = parameters.getRawParameterValue("gain")->load();
    const double stepsPerSecond = bpm / 60.0 * 4.0;
    const double baseStep = *ppq * 4.0;
    const int outputSamples = buffer.getNumSamples();
    for (int i = 0; i < outputSamples; ++i)
    {
        const double phase = baseStep + (static_cast<double>(i) / engineSampleRate) * stepsPerSecond;
        const auto step = static_cast<int64_t>(std::floor(phase));
        if (step != previousStep)
        {
            previousStep = step;
            // 16-step wrap, including when the host loop restarts.
            const auto eventIndex = static_cast<size_t>((step % 16 + 16) % 16);
            const auto& event = localPattern->events[eventIndex];
            if (event.enabled)
            {
                activeSample = localSample;
                voicePosition = static_cast<double>(event.offset) * (total - 1);
                const double requested = static_cast<double>(event.length) * total;
                voiceEnd = juce::jmin(static_cast<double>(total - 1), voicePosition + requested);
                voiceIncrement = localSample->sampleRate / engineSampleRate;
            }
            else
            {
                voicePosition = voiceEnd;
            }
        }

        if (activeSample && voicePosition < voiceEnd)
        {
            const auto& source = activeSample->audio;
            const auto index = static_cast<int>(voicePosition);
            if (index + 1 < source.getNumSamples())
            {
                const auto fraction = static_cast<float>(voicePosition - index);
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    const auto sourceChannel = juce::jmin(channel, source.getNumChannels() - 1);
                    const auto* data = source.getReadPointer(sourceChannel);
                    const auto value = data[index] + (data[index + 1] - data[index]) * fraction;
                    buffer.setSample(channel, i, value * gain);
                }
            }
            voicePosition += voiceIncrement;
        }
    }
}

juce::AudioProcessorEditor* PhraserAudioProcessor::createEditor()
{
    return new PhraserAudioProcessorEditor(*this);
}

void PhraserAudioProcessor::getStateInformation(juce::MemoryBlock& output)
{
    auto tree = parameters.copyState();
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, output);
}

void PhraserAudioProcessor::setStateInformation(const void* data, int length)
{
    if (auto xml = getXmlFromBinary(data, length))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhraserAudioProcessor();
}
