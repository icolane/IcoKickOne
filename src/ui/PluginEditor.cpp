#include "PluginEditor.h"
#include "IcoKickOne/KickDsp.h"

#include <algorithm>
#include <cmath>

namespace
{
const juce::Colour background { 0xff11151b };
const juce::Colour panel { 0xff1a2029 };
const juce::Colour panelLight { 0xff242d38 };
const juce::Colour accent { 0xffff7043 };
const juce::Colour text { 0xfff1f3f5 };
const juce::Colour muted { 0xff98a5b3 };
}

IcoKickOneAudioProcessorEditor::WaveformPreview::WaveformPreview(IcoKickOneAudioProcessor& processorToUse)
    : processor(processorToUse)
{
}

void IcoKickOneAudioProcessorEditor::WaveformPreview::paint(juce::Graphics& graphics)
{
    auto area = getLocalBounds().toFloat();
    graphics.setColour(panel);
    graphics.fillRoundedRectangle(area, 12.0f);
    graphics.setColour(panelLight.withAlpha(0.8f));
    for (int line = 1; line < 4; ++line)
    {
        const auto y = area.getY() + area.getHeight() * static_cast<float>(line) / 4.0f;
        graphics.drawHorizontalLine(juce::roundToInt(y), area.getX() + 14.0f, area.getRight() - 14.0f);
    }

    auto params = processor.readParameters();
    kicklab::Voice voice;
    voice.active = true;
    voice.noiseState = 0x48151623u;
    const auto center = area.getCentreY();
    const auto amplitude = area.getHeight() * 0.40f;
    juce::Path waveformPath;
    waveformPath.startNewSubPath(area.getX() + 12.0f, center);
    const auto count = juce::jmax(2, getWidth() - 24);
    for (int i = 0; i < count; ++i)
    {
        float sample = 0.0f;
        for (int step = 0; step < 128; ++step)
            sample = kicklab::renderSample(voice, params, 44100.0);
        const auto x = area.getX() + 12.0f + static_cast<float>(i);
        waveformPath.lineTo(x, center - sample * amplitude);
    }
    graphics.setColour(accent);
    graphics.strokePath(waveformPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
    graphics.setColour(muted);
    graphics.setFont(11.0f);
    graphics.drawText("KICK SHAPE PREVIEW", 14, 8, getWidth() - 28, 16, juce::Justification::centredLeft);
}

IcoKickOneAudioProcessorEditor::IcoKickOneAudioProcessorEditor(IcoKickOneAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), waveform(p)
{
    setSize(900, 650);
    startTimerHz(24);

    mainTitle.setText("KICKLAB ONE", juce::dontSendNotification);
    mainTitle.setFont(juce::Font(25.0f, juce::Font::bold));
    mainTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(mainTitle);

    subtitle.setText("PROCEDURAL KICK INSTRUMENT  /  MIDI IN", juce::dontSendNotification);
    subtitle.setFont(juce::Font(11.0f, juce::Font::bold));
    subtitle.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitle);

    styleLabel.setText("CHARACTER", juce::dontSendNotification);
    styleLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    styleLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(styleLabel);

    const juce::StringArray styles {
        "Hardgroove", "Schranz", "Industrial", "Reverse Bass", "Rumble",
        "Clean Punch", "Zaag", "Newstyle", "Classic"
    };
    style.addItemList(styles, 1);
    style.setColour(juce::ComboBox::backgroundColourId, panel);
    style.setColour(juce::ComboBox::textColourId, text);
    style.setColour(juce::ComboBox::outlineColourId, panelLight);
    addAndMakeVisible(style);
    styleAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.parameters.getParameter("style"), style);

    randomizeButton.setColour(juce::TextButton::buttonColourId, accent);
    randomizeButton.setColour(juce::TextButton::textColourOffId, background);
    randomizeButton.onClick = [this] { processor.randomize(); };
    addAndMakeVisible(randomizeButton);

    previewButton.setColour(juce::TextButton::buttonColourId, panelLight);
    previewButton.setColour(juce::TextButton::textColourOffId, text);
    previewButton.onClick = [this] { processor.triggerPreview(); };
    addAndMakeVisible(previewButton);

    const auto addToggle = [this](juce::ToggleButton& button, const juce::String& id)
    {
        button.setColour(juce::ToggleButton::textColourId, muted);
        addAndMakeVisible(button);
        buttonAttachments.push_back(std::make_unique<juce::ButtonParameterAttachment>(
            *processor.parameters.getParameter(id), button));
    };
    addToggle(pitchTrack, "pitchTrack");
    addToggle(autoGain, "autoGain");

    const int knobWidth = 94;
    addKnob("root", "ROOT", { 26, 154, knobWidth, 100 });
    addKnob("sweep", "SWEEP", { 130, 154, knobWidth, 100 });
    addKnob("sweepTime", "TIME", { 234, 154, knobWidth, 100 });
    addKnob("decay", "DECAY", { 338, 154, knobWidth, 100 });
    addKnob("body", "BODY", { 442, 154, knobWidth, 100 });
    addKnob("click", "CLICK", { 546, 154, knobWidth, 100 });
    addKnob("rumble", "RUMBLE", { 650, 154, knobWidth, 100 });
    addKnob("drive", "DRIVE", { 754, 154, knobWidth, 100 });
    addKnob("output", "OUTPUT", { 26, 268, knobWidth, 100 });
    addKnob("moduleAmount", "FX AMOUNT", { 130, 268, knobWidth, 100 });

    const std::array<juce::String, 9> moduleNames {
        "PUNCH", "SATURATION", "RUMBLE", "HARMONICS", "SUB THUMP",
        "LOW PASS", "SOFT CLIP", "TRANSIENT AIR", "OUTPUT GUARD"
    };
    for (std::size_t i = 0; i < modules.size(); ++i)
    {
        modules[i].setButtonText(moduleNames[i]);
        modules[i].setColour(juce::ToggleButton::textColourId, text);
        modules[i].setColour(juce::ToggleButton::tickColourId, accent);
        addAndMakeVisible(modules[i]);
        buttonAttachments.push_back(std::make_unique<juce::ButtonParameterAttachment>(
            *processor.parameters.getParameter("fx" + juce::String(static_cast<int>(i + 1))), modules[i]));
    }

    addAndMakeVisible(waveform);
}

void IcoKickOneAudioProcessorEditor::addKnob(const juce::String& id, const juce::String& label,
                                             juce::Rectangle<int> bounds)
{
    juce::Slider* slider = nullptr;
    if (id == "root") slider = &root;
    else if (id == "sweep") slider = &sweep;
    else if (id == "sweepTime") slider = &sweepTime;
    else if (id == "decay") slider = &decay;
    else if (id == "body") slider = &body;
    else if (id == "click") slider = &click;
    else if (id == "rumble") slider = &rumble;
    else if (id == "drive") slider = &drive;
    else if (id == "output") slider = &output;
    else if (id == "moduleAmount") slider = &moduleAmount;
    jassert(slider != nullptr);

    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    slider->setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider->setColour(juce::Slider::rotarySliderOutlineColourId, panelLight);
    slider->setColour(juce::Slider::thumbColourId, text);
    slider->setColour(juce::Slider::textBoxTextColourId, text);
    slider->setColour(juce::Slider::textBoxBackgroundColourId, panel);
    slider->setColour(juce::Slider::textBoxOutlineColourId, panel);
    addAndMakeVisible(*slider);

    auto labelComponent = std::make_unique<juce::Label>();
    labelComponent->setText(label, juce::dontSendNotification);
    labelComponent->setFont(juce::Font(10.0f, juce::Font::bold));
    labelComponent->setColour(juce::Label::textColourId, muted);
    labelComponent->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*labelComponent);
    labelComponent->setBounds(bounds.getX(), bounds.getY() + 77, bounds.getWidth(), 18);
    controlLabels.push_back(std::move(labelComponent));
    slider->setBounds(bounds.getX() + 10, bounds.getY(), bounds.getWidth() - 20, 72);
    sliderAttachments.push_back(std::make_unique<juce::SliderParameterAttachment>(
        *processor.parameters.getParameter(id), *slider));
}

void IcoKickOneAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);
    graphics.setColour(panel);
    graphics.fillRoundedRectangle(16.0f, 16.0f, 868.0f, 84.0f, 12.0f);
    graphics.setColour(accent);
    graphics.fillRoundedRectangle(16.0f, 16.0f, 4.0f, 84.0f, 2.0f);

    graphics.setColour(panel);
    graphics.fillRoundedRectangle(16.0f, 128.0f, 868.0f, 256.0f, 12.0f);
    graphics.fillRoundedRectangle(16.0f, 396.0f, 868.0f, 112.0f, 12.0f);
    graphics.fillRoundedRectangle(16.0f, 520.0f, 868.0f, 112.0f, 12.0f);

    graphics.setColour(muted);
    graphics.setFont(11.0f);
    graphics.drawText("KICK ENGINE", 30, 134, 200, 17, juce::Justification::centredLeft);
    graphics.drawText("PROCESSING MODULES  /  ENABLE OR BYPASS EACH STAGE", 30, 402, 500, 17,
                      juce::Justification::centredLeft);
    graphics.drawText("OUTPUT", 30, 526, 200, 17, juce::Justification::centredLeft);
}

void IcoKickOneAudioProcessorEditor::resized()
{
    mainTitle.setBounds(34, 26, 270, 34);
    subtitle.setBounds(35, 62, 300, 20);
    styleLabel.setBounds(352, 32, 90, 17);
    style.setBounds(350, 52, 175, 30);
    randomizeButton.setBounds(545, 37, 150, 38);
    previewButton.setBounds(708, 37, 150, 38);

    for (std::size_t i = 0; i < modules.size(); ++i)
    {
        const auto column = static_cast<int>(i % 5);
        const auto row = static_cast<int>(i / 5);
        modules[i].setBounds(28 + column * 168, 430 + row * 34, 160, 28);
    }

    pitchTrack.setBounds(238, 538, 150, 28);
    autoGain.setBounds(390, 538, 150, 28);
    waveform.setBounds(30, 564, 570, 54);
    previewButton.setBounds(708, 37, 150, 38);
}

void IcoKickOneAudioProcessorEditor::timerCallback()
{
    waveform.repaint();
}
