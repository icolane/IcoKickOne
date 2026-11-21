#include "PluginProcessor.h"
#include "../ui/PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;

void addFloat(APVTS::ParameterLayout& layout, const juce::String& id, const juce::String& name,
              float minimum, float maximum, float initial, const juce::String& unit = {})
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { id, 1 }, name,
        juce::NormalisableRange<float> { minimum, maximum, 0.001f }, initial,
        juce::AudioParameterFloatAttributes().withLabel(unit)));
}

float load(const APVTS& state, const juce::String& id, float fallback)
{
    if (const auto* parameter = state.getRawParameterValue(id))
        return parameter->load();
    return fallback;
}

void addToggle(APVTS::ParameterLayout& layout, const juce::String& id, const juce::String& name, bool initial)
{
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { id, 1 }, name, initial));
}
}

IcoKickOneAudioProcessor::IcoKickOneAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout IcoKickOneAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    const juce::StringArray styles {
        "Hardgroove", "Schranz", "Industrial", "Reverse Bass", "Rumble",
        "Clean Punch", "Zaag", "Newstyle", "Classic"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "style", 1 }, "Style", styles, 1));
    addFloat(layout, "root", "Root", 36.0f, 72.0f, 52.0f, "Hz");
    addFloat(layout, "sweep", "Pitch Sweep", 0.0f, 48.0f, 30.0f, "st");
    addFloat(layout, "sweepTime", "Sweep Time", 5.0f, 100.0f, 28.0f, "ms");
    addFloat(layout, "decay", "Decay", 0.12f, 1.6f, 0.68f, "s");
    addFloat(layout, "body", "Body", 0.0f, 1.0f, 0.88f);
    addFloat(layout, "click", "Click", 0.0f, 1.0f, 0.22f);
    addFloat(layout, "rumble", "Rumble", 0.0f, 1.0f, 0.25f);
    addFloat(layout, "drive", "Drive", 0.0f, 24.0f, 8.0f, "dB");
    addFloat(layout, "output", "Output", 0.0f, 1.5f, 0.78f);
    addFloat(layout, "moduleAmount", "Module Amount", 0.0f, 1.0f, 0.65f);
    addToggle(layout, "pitchTrack", "Pitch Track", true);
    addToggle(layout, "autoGain", "Auto Gain", true);

    const std::array<juce::String, 9> modules {
        "Punch", "Saturation", "Rumble Layer", "Harmonic Edge", "Sub Thump",
        "Low Pass", "Soft Clip", "Transient Air", "Output Guard"
    };
    for (std::size_t i = 0; i < modules.size(); ++i)
        addToggle(layout, "fx" + juce::String(static_cast<int>(i + 1)), modules[i], i != 7);
    return layout;
}

void IcoKickOneAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& voice : voices)
        voice = {};
}

void IcoKickOneAudioProcessor::releaseResources()
{
    for (auto& voice : voices)
        voice = {};
}

bool IcoKickOneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

kicklab::Parameters IcoKickOneAudioProcessor::readParameters() const
{
    kicklab::Parameters result;
    result.style = juce::jlimit(0, 8, juce::roundToInt(load(parameters, "style", 1.0f)));
    result.rootHz = load(parameters, "root", 52.0f);
    result.sweepSemitones = load(parameters, "sweep", 30.0f);
    result.sweepTimeMs = load(parameters, "sweepTime", 28.0f);
    result.decaySeconds = load(parameters, "decay", 0.68f);
    result.body = load(parameters, "body", 0.88f);
    result.click = load(parameters, "click", 0.22f);
    result.rumble = load(parameters, "rumble", 0.25f);
    result.driveDb = load(parameters, "drive", 8.0f);
    result.output = load(parameters, "output", 0.78f);
    result.moduleAmount = load(parameters, "moduleAmount", 0.65f);
    result.pitchTrack = load(parameters, "pitchTrack", 1.0f) >= 0.5f;
    result.autoGain = load(parameters, "autoGain", 1.0f) >= 0.5f;
    for (std::size_t i = 0; i < result.modules.size(); ++i)
        result.modules[i] = load(parameters, "fx" + juce::String(static_cast<int>(i + 1)), i == 7 ? 0.0f : 1.0f) >= 0.5f;
    return result;
}

void IcoKickOneAudioProcessor::triggerVoice(int noteNumber, const kicklab::Parameters& current)
{
    auto& voice = voices[nextVoice++ % voices.size()];
    voice = {};
    voice.active = true;
    voice.noiseState = 0x9e3779b9u ^ (static_cast<std::uint32_t>(noteNumber) * 2654435761u);
    const auto semitones = static_cast<float>(noteNumber - 36);
    voice.noteRatio = current.pitchTrack ? std::pow(2.0f, semitones / 12.0f) : 1.0f;
}

void IcoKickOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto current = readParameters();
    if (previewPending.exchange(false))
        triggerVoice(36, current);

    auto midiEvent = midiMessages.begin();
    const auto midiEnd = midiMessages.end();
    const auto sampleCount = buffer.getNumSamples();
    const auto channels = buffer.getNumChannels();

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        while (midiEvent != midiEnd && (*midiEvent).samplePosition <= sample)
        {
            const auto message = (*midiEvent).getMessage();
            if (message.isNoteOn())
                triggerVoice(message.getNoteNumber(), current);
            ++midiEvent;
        }

        float output = 0.0f;
        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;
            auto parametersForVoice = current;
            parametersForVoice.rootHz = std::clamp(current.rootHz * voice.noteRatio, 25.0f, 120.0f);
            output += kicklab::renderSample(voice, parametersForVoice, currentSampleRate);
        }
        output = std::tanh(output * 1.35f);
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel, sample, output);
    }
    midiMessages.clear();
}

void IcoKickOneAudioProcessor::randomize()
{
    auto& random = juce::Random::getSystemRandom();
    const std::array<std::pair<juce::String, float>, 8> values {{
        { "root", random.nextFloat() * 28.0f + 40.0f },
        { "sweep", random.nextFloat() * 38.0f + 10.0f },
        { "sweepTime", random.nextFloat() * 55.0f + 12.0f },
        { "decay", random.nextFloat() * 0.8f + 0.28f },
        { "body", random.nextFloat() * 0.48f + 0.5f },
        { "click", random.nextFloat() * 0.55f + 0.08f },
        { "rumble", random.nextFloat() * 0.52f + 0.04f },
        { "drive", random.nextFloat() * 15.0f + 3.0f }
    }};
    for (const auto& item : values)
    {
        if (auto* parameter = parameters.getParameter(item.first))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(item.second));
            parameter->endChangeGesture();
        }
    }

    if (auto* parameter = parameters.getParameter("style"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(random.nextInt(9))));
        parameter->endChangeGesture();
    }
}

juce::AudioProcessorEditor* IcoKickOneAudioProcessor::createEditor()
{
    return new IcoKickOneAudioProcessorEditor(*this);
}

void IcoKickOneAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    if (const auto state = parameters.copyState(); state.isValid())
    {
        if (const auto xml = state.createXml())
            copyXmlToBinary(*xml, destinationData);
    }
}

void IcoKickOneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IcoKickOneAudioProcessor();
}
