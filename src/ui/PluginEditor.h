#pragma once

#include <JuceHeader.h>
#include "../plugin/PluginProcessor.h"

class IcoKickOneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit IcoKickOneAudioProcessorEditor(IcoKickOneAudioProcessor& processor);
    ~IcoKickOneAudioProcessorEditor() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    class WaveformPreview final : public juce::Component
    {
    public:
        explicit WaveformPreview(IcoKickOneAudioProcessor& processorToUse);
        void paint(juce::Graphics& graphics) override;

    private:
        IcoKickOneAudioProcessor& processor;
    };

    void timerCallback() override;
    void addKnob(const juce::String& id, const juce::String& label, juce::Rectangle<int> bounds);
    void addModule(const juce::String& id, const juce::String& label, juce::Rectangle<int> bounds);

    IcoKickOneAudioProcessor& processor;
    juce::ComboBox style;
    juce::TextButton randomizeButton { "RANDOMIZE" };
    juce::TextButton previewButton { "PREVIEW C2" };
    juce::ToggleButton pitchTrack { "PITCH TRACK" };
    juce::ToggleButton autoGain { "AUTO GAIN" };
    juce::Slider root, sweep, sweepTime, decay, body, click, rumble, drive, output, moduleAmount;
    juce::Label styleLabel, mainTitle, subtitle;
    WaveformPreview waveform;
    std::array<juce::ToggleButton, 9> modules;
    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::Label>> controlLabels;
    std::unique_ptr<juce::ComboBoxParameterAttachment> styleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IcoKickOneAudioProcessorEditor)
};
