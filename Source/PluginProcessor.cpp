#include "PluginProcessor.h"
#include "PluginEditor.h"

PhatringAudioProcessor::PhatringAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",       juce::AudioChannelSet::stereo(), true)
          .withInput  ("Modulation",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output",      juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

PhatringAudioProcessor::~PhatringAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout PhatringAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input LP (log scale)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lpfreq", "Input LP",
        juce::NormalisableRange<float> (5.0f, 500.0f, 0.0f, 0.5f), 500.0f));

    // Noise mix
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "noise_mix", "Input Noise", 0.0f, 100.0f, 0.0f));

    // AM amount
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "amount", "AM", 0.0f, 100.0f, 100.0f));

    // Filter enable toggle
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "filter_on", "Filter On", true));

    // Modulation source
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mod_source", "Mod Source",
        juce::StringArray ("Self", "External", "Both"), 0));

    // Bypass
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "bypass", "Bypass", false));

    // ---- Clipper section ----
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "clip_on", "Clip On", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "clip_threshold", "Threshold (dB)",
        -30.0f, 0.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "clip_ceiling", "Ceiling (dB)",
        -20.0f, 0.0f, -0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "clip_soft", "Soft Clip (dB)",
        0.0f, 6.0f, 2.0f));

    return { params.begin(), params.end() };
}

void PhatringAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, 1, static_cast<juce::uint32> (samplesPerBlock) };
    filterL1.prepare (spec);
    filterL2.prepare (spec);
    filterR1.prepare (spec);
    filterR2.prepare (spec);

    lastLowpassFreq = -1.0f;
    smoothedAmount.setCurrentAndTargetValue (1.0f);
}

void PhatringAudioProcessor::releaseResources() {}

void PhatringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalIn  = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();

    for (int i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalIn < 2)
        return;

    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f)
        return;

    // Read parameters
    const float lpfreq      = apvts.getRawParameterValue ("lpfreq")     ->load();
    const float noiseMix    = apvts.getRawParameterValue ("noise_mix")  ->load();
    const float amtRaw      = apvts.getRawParameterValue ("amount")     ->load() * 0.01f;
    const bool  filterOn    = apvts.getRawParameterValue ("filter_on")  ->load() > 0.5f;
    const int   modSource   = juce::roundToInt (apvts.getRawParameterValue ("mod_source")->load());
    const bool  clipOn      = apvts.getRawParameterValue ("clip_on")    ->load() > 0.5f;
    const float clThreshDB  = apvts.getRawParameterValue ("clip_threshold")->load();
    const float clCeilingDB = apvts.getRawParameterValue ("clip_ceiling")->load();
    const float clSoftDB    = apvts.getRawParameterValue ("clip_soft")   ->load();

    smoothedAmount.setTargetValue (amtRaw);

    // Update filter coefficients
    if (lpfreq != lastLowpassFreq)
    {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (getSampleRate(), lpfreq, 0.7071f);
        *filterL1.coefficients = *coeffs;
        *filterL2.coefficients = *coeffs;
        *filterR1.coefficients = *coeffs;
        *filterR2.coefficients = *coeffs;
        lastLowpassFreq = lpfreq;
    }

    // Noise / original modulation gains
    float noiseGain, modGain;
    if (noiseMix <= 50.0f)
    {
        noiseGain = noiseMix / 50.0f;
        modGain   = 1.0f;
    }
    else
    {
        noiseGain = 1.0f;
        modGain   = 1.0f - (noiseMix - 50.0f) / 50.0f;
    }

    // Pre‑compute clipper constants
    const float makeup    = juce::Decibels::decibelsToGain (clCeilingDB - clThreshDB);
    const float ceiling   = juce::Decibels::decibelsToGain (clCeilingDB);
    const float scv       = juce::Decibels::decibelsToGain (-clSoftDB);       // soft clip threshold (linear)
    const float peakDB    = clCeilingDB + 25.0f;
    const float scmult    = std::abs ((clCeilingDB + clSoftDB) / (peakDB + clSoftDB));

    const int numSamples = buffer.getNumSamples();
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputL = outL[s];
        const float inputR = outR[s];

        // Build modulation signal
        float modL = 0.0f, modR = 0.0f;
        if (modSource == 0)        // Self
        {
            modL = inputL;
            modR = inputR;
        }
        else if (modSource == 1)   // External (pins 3‑4)
        {
            if (totalIn >= 4)
            {
                modL = buffer.getSample (2, s);
                modR = buffer.getSample (3, s);
            }
        }
        else                       // Both (‑6 dB mix)
        {
            modL = inputL * 0.5f;
            modR = inputR * 0.5f;
            if (totalIn >= 4)
            {
                modL += buffer.getSample (2, s) * 0.5f;
                modR += buffer.getSample (3, s) * 0.5f;
            }
        }

        // Low‑pass filter
        if (filterOn)
        {
            modL = filterL1.processSample (modL);
            modL = filterL2.processSample (modL);
            modR = filterR1.processSample (modR);
            modR = filterR2.processSample (modR);
        }

        // Add white noise
        const float noiseL = rng.nextFloat() * 2.0f - 1.0f;
        const float noiseR = rng.nextFloat() * 2.0f - 1.0f;
        modL = modL * modGain + noiseL * noiseGain;
        modR = modR * modGain + noiseR * noiseGain;

        // Ring modulation with dry/wet mix
        const float amt = smoothedAmount.getNextValue();
        const float dry = 1.0f - amt;
        const float wet = amt;

        float outL_sample = inputL * dry + (inputL * modL) * wet;
        float outR_sample = inputR * dry + (inputR * modR) * wet;

        // ---- Event Horizon Clipper ----
        if (clipOn)
        {
            auto clip = [&](float sample) -> float
            {
                float sign = (sample >= 0.0f) ? 1.0f : -1.0f;
                float absSample = std::abs (sample) * makeup;    // apply makeup gain

                float overDB = 20.0f * std::log10 (std::max (absSample, 1e-15f)) - clCeilingDB;

                float outAbs = absSample;
                if (absSample > scv)
                {
                    float compressed = scv + std::pow (10.0f, overDB * scmult / 20.0f);
                    outAbs = compressed;
                }
                outAbs = std::min (outAbs, ceiling);            // hard limit to ceiling
                return outAbs * sign;
            };

            outL_sample = clip (outL_sample);
            outR_sample = clip (outR_sample);
        }

        outL[s] = outL_sample;
        outR[s] = outR_sample;
    }
}

void PhatringAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PhatringAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* PhatringAudioProcessor::createEditor()
{
    return new PhatringAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhatringAudioProcessor();
}