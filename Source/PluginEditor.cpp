#include "PluginProcessor.h"
#include "PluginEditor.h"

PhatringAudioProcessorEditor::PhatringAudioProcessorEditor(PhatringAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), resizer(this, &constrainer)
{
    // ---------- Colors ----------
    customLookAndFeel.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    customLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colour(0xffcccccc));
    customLookAndFeel.setColour(juce::Slider::trackColourId, juce::Colour(0xff444444));
    customLookAndFeel.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff888888));
    customLookAndFeel.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333));
    customLookAndFeel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff444444));
    customLookAndFeel.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff5a5a));
    customLookAndFeel.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    customLookAndFeel.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    customLookAndFeel.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3a3a3a));
    customLookAndFeel.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
    customLookAndFeel.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff555555));
    setLookAndFeel(&customLookAndFeel);

    // ---------- Helper for vertical sliders ----------
    auto makeVerticalSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text)
        {
            s.setSliderStyle(juce::Slider::LinearVertical);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
            s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffaaaaaa));
            s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1e1e1e));
            addAndMakeVisible(s);
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
            l.setFont(sliderLabelFont);
            l.attachToComponent(&s, false);
            addAndMakeVisible(l);
        };

    // ---------- Helper for rotary knobs (clipper) ----------
    auto makeRotarySlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text)
        {
            s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 60);
            s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffaaaaaa));
            s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1e1e1e));
            addAndMakeVisible(s);
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
            l.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            l.setFont(sliderLabelFont);
            addAndMakeVisible(l);
        };

    makeVerticalSlider(freqSlider, freqLabel, "Input LP");
    makeVerticalSlider(noiseSlider, noiseLabel, "Noise");
    makeVerticalSlider(amountSlider, amountLabel, "AM");

    makeRotarySlider(clipThreshKnob, clipThreshLabel, "Thresh");
    makeRotarySlider(clipCeilKnob, clipCeilLabel, "Ceil");
    makeRotarySlider(clipSoftKnob, clipSoftLabel, "Soft");

    // ---------- Buttons ----------
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setSize(68, 24);
    addAndMakeVisible(bypassButton);

    filterToggle.setButtonText("FILTER");
    filterToggle.setClickingTogglesState(true);
    filterToggle.setSize(68, 24);
    filterToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    filterToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff888888));
    addAndMakeVisible(filterToggle);

    clipToggle.setButtonText("CLIP");
    clipToggle.setClickingTogglesState(true);
    clipToggle.setSize(68, 24);
    clipToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    clipToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff888888));
    addAndMakeVisible(clipToggle);

    // ---------- ComboBox ----------
    modSourceBox.addItemList({ "Self", "External", "Both" }, 1);
    modSourceBox.setSelectedItemIndex(0);
    modSourceBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modSourceBox);

    // ---------- Attachments ----------
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "lpfreq", freqSlider);
    noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "noise_mix", noiseSlider);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "amount", amountSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", bypassButton);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "filter_on", filterToggle);
    sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "mod_source", modSourceBox);
    clipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "clip_on", clipToggle);
    clipThreshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "clip_threshold", clipThreshKnob);
    clipCeilAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "clip_ceiling", clipCeilKnob);
    clipSoftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "clip_soft", clipSoftKnob);

    // Resize constraints
    constrainer.setMinimumSize(400, 450);
    constrainer.setMaximumSize(1000, 1125);
    addAndMakeVisible(resizer);
    resizer.setAlwaysOnTop(true);

    setSize(refWidth, refHeight);
}

PhatringAudioProcessorEditor::~PhatringAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PhatringAudioProcessorEditor::paint(juce::Graphics& g)
{

    g.fillAll(juce::Colour(0xff1c1c1c));
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds().toFloat(), 1.5f);

    g.setColour(juce::Colour(0xffcccccc));
    g.setFont(juce::Font(22.0f * currentScale, juce::Font::bold));
    g.drawText("PHATring",
        juce::roundToInt(24 * currentScale),
        juce::roundToInt(13 * currentScale),
        juce::roundToInt(200 * currentScale),
        juce::roundToInt(28 * currentScale),
        juce::Justification::left, false);

    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::Font(11.0f * currentScale));
    g.drawText("R1C1N buffering \\>_<\\",
    juce::roundToInt(24 * currentScale),
    getHeight() - juce::roundToInt(25 * currentScale),
    juce::roundToInt(200 * currentScale),
    juce::roundToInt(14 * currentScale),
    juce::Justification::left, false);
}

void PhatringAudioProcessorEditor::resized()
{
    const float scaleX = getWidth() / refWidth;
    const float scaleY = getHeight() / refHeight;
    currentScale = juce::jmin(scaleX, scaleY);

    auto sc = [&](int v) { return juce::roundToInt(v * currentScale); };

    auto area = getLocalBounds().reduced(sc(20));

    // ---------- Top bar ----------
    auto topBar = area.removeFromTop(sc(36));
    bypassButton.setBounds(topBar.removeFromRight(sc(68))
        .withTrimmedTop(sc(-4))
        .withTrimmedBottom(sc(16)));
    modSourceBox.setBounds(bypassButton.getX() - sc(130), bypassButton.getY(),
        sc(120), bypassButton.getHeight());

    // ---------- Three equal vertical slider columns ----------
    const int numSliders = 3;
    const int sliderSpacing = sc(20);
    const int sliderTotalWidth = area.getWidth() - sc(80); // leave room for left buttons
    const int sliderWidth = (sliderTotalWidth - (numSliders - 1) * sliderSpacing) / numSliders;
    const int sliderHeight = area.getHeight() * 0.55f;

    auto sliderArea = area.removeFromTop(sliderHeight);
    int xPos = sliderArea.getX() + sc(80); // start after left button column

    freqSlider.setBounds(xPos, sliderArea.getY(), sliderWidth, sliderHeight);
    xPos += sliderWidth + sliderSpacing;
    noiseSlider.setBounds(xPos, sliderArea.getY(), sliderWidth, sliderHeight);
    xPos += sliderWidth + sliderSpacing;
    amountSlider.setBounds(xPos, sliderArea.getY(), sliderWidth, sliderHeight);

    // Store slider centers for alignment
    const float freqCenterX = freqSlider.getBounds().getCentreX();
    const float noiseCenterX = noiseSlider.getBounds().getCentreX();
    const float amountCenterX = amountSlider.getBounds().getCentreX();
    const float sliderMidY = sliderArea.getCentreY();

    // ---------- FILTER button (left side, vertically centered on sliders) ----------
    filterToggle.setBounds(sliderArea.getX(),
        (int)(sliderMidY - sc(12)),
        sc(68), sc(24));

    // ---------- Gap before clip section ----------
    area.removeFromTop(sc(12));

    // ---------- Clip section ----------
    auto clipSection = area;

    // CLIP button (left side, vertically centered on clip section)
    clipToggle.setBounds(clipSection.getX(),
        clipSection.getY() + (clipSection.getHeight() - sc(24)) / 2,
        sc(68), sc(24));

    // Three rotary knobs – centers aligned with the three sliders above
    const int knobWidth = sc(90);
    const int knobHeight = clipSection.getHeight();

    clipThreshKnob.setBounds(juce::roundToInt(freqCenterX - knobWidth / 2), clipSection.getY(), knobWidth, knobHeight);
    clipCeilKnob.setBounds(juce::roundToInt(noiseCenterX - knobWidth / 2), clipSection.getY(), knobWidth, knobHeight);
    clipSoftKnob.setBounds(juce::roundToInt(amountCenterX - knobWidth / 2), clipSection.getY(), knobWidth, knobHeight);

    // ----- 标签（Thresh/Ceil/Soft）固定在旋钮正中心 -----
    auto placeLabelInCentre = [&](juce::Label& label, const juce::Slider& knob)
        {
            const int labelHeight = sc(14);
            // 宽度与旋钮相同，高度固定，然后垂直居中放置在旋钮里
            label.setBounds(knob.getX(),
                knob.getY() + (knob.getHeight() - labelHeight) / 2 + sc(20),
                knob.getWidth(),
                labelHeight);
        };

    placeLabelInCentre(clipThreshLabel, clipThreshKnob);
    placeLabelInCentre(clipCeilLabel, clipCeilKnob);
    placeLabelInCentre(clipSoftLabel, clipSoftKnob);

    // 确保标签显示在最前面，不会被旋钮图形遮挡
    clipThreshLabel.toFront(false);
    clipCeilLabel.toFront(false);
    clipSoftLabel.toFront(false);

    // 上方三个 slider 的数值也要置顶
    freqLabel.toFront(false);
    noiseLabel.toFront(false);
    amountLabel.toFront(false);

    // Resize corner
    const int resizerSize = sc(16);
    resizer.setBounds(getWidth() - resizerSize, getHeight() - resizerSize, resizerSize, resizerSize);

    // Refresh label fonts
    sliderLabelFont = juce::Font(12.0f * currentScale);
    for (auto* lbl : { &freqLabel, &noiseLabel, &amountLabel,
                       &clipThreshLabel, &clipCeilLabel, &clipSoftLabel })
        lbl->setFont(sliderLabelFont);
}