#pragma once

#include <JuceHeader.h>
#include "IcoKickOne/KickDsp.h"

class IcoKickOneAudioProcessor final : public juce::AudioProcessor
{
public:
    using juce::AudioProcessor::processBlock;

    IcoKickOneAudioProcessor();
    ~IcoKickOneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    kicklab::Parameters readParameters() const;
    void randomize();
    void triggerPreview() noexcept { previewPending.store(true); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void triggerVoice(int noteNumber, const kicklab::Parameters& current);

    std::array<kicklab::Voice, 8> voices {};
    std::size_t nextVoice = 0;
    double currentSampleRate = 44100.0;
    std::atomic<bool> previewPending { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IcoKickOneAudioProcessor)
};
