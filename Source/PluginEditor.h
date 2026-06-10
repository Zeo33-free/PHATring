#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PhatringAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PhatringAudioProcessorEditor (PhatringAudioProcessor&);
    ~PhatringAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PhatringAudioProcessor& audioProcessor;

    // Original controls
    juce::Slider freqSlider, noiseSlider, amountSlider;
    juce::Label  freqLabel,  noiseLabel,  amountLabel;

    juce::TextButton bypassButton, filterToggle;
    juce::ComboBox   modSourceBox;

    // Clipper controls (rotary knobs)
    juce::TextButton clipToggle;
    juce::Slider     clipThreshKnob, clipCeilKnob, clipSoftKnob;
    juce::Label      clipThreshLabel, clipCeilLabel, clipSoftLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clipAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipThreshAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipCeilAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipSoftAttachment;

    juce::LookAndFeel_V4 customLookAndFeel;

    juce::ComponentBoundsConstrainer constrainer;
    juce::ResizableCornerComponent resizer;

    float currentScale = 1.0f;
    const float refWidth  = 480.0f;
    const float refHeight = 540.0f;

    juce::Font sliderLabelFont { 12.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhatringAudioProcessorEditor)
};