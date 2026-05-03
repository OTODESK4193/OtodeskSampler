// ==========================================
// File: PluginProcessor.cpp
// ==========================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <regex>
#include <thread>
#include <algorithm>
#include "rubberband/RubberBandStretcher.h"

juce::AudioProcessorValueTreeState::ParameterLayout Otodesk_samplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 1; i <= 8; ++i) {
        juce::String idx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>("attack_" + idx, "Attack " + idx, juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.3f), 0.05f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("release_" + idx, "Release " + idx, juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("sampleStart_" + idx, "Sample Start " + idx, 0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("sampleEnd_" + idx, "Sample End " + idx, 0.0f, 1.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("loopStart_" + idx, "Loop Start " + idx, 0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("loopLength_" + idx, "Loop Length " + idx, 0.01f, 1.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("crossfade_" + idx, "X-Fade " + idx, 0.0f, 0.5f, 0.05f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("playMode_" + idx, "Play Mode " + idx, juce::StringArray{ "Loop", "One-Shot" }, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>("rootKey_" + idx, "Root Key " + idx, -1, 127, -1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>("pan_" + idx, "Pan " + idx, -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("slotGain_" + idx, "Slot Gain " + idx, -36.0f, 12.0f, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterInt>("octave_" + idx, "Octave " + idx, -3, 3, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>("pitchSt_" + idx, "Semi (st) " + idx, -24, 24, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("fineTune_" + idx, "Fine Tune " + idx, -100.0f, 100.0f, 0.0f));

        // 【拡張】LFOのターゲットにピッチやFXを追加
        juce::StringArray lfoTargets{ "None", "Loop Start", "Loop Length", "X-Fade", "Pan", "Sample Start", "Pitch (Oct)", "Pitch (Semi)", "Pitch (Fine)", "FX 1 Amount", "FX 2 Amount", "FX 3 Amount", "FX 4 Amount" };

        for (int l = 1; l <= 3; ++l) {
            juce::String lIdx = juce::String(l) + "_" + idx;
            params.push_back(std::make_unique<juce::AudioParameterBool>("lfo" + lIdx + "Enable", "LFO " + juce::String(l) + " Enable " + idx, false));
            params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + lIdx + "Target", "Target " + lIdx, lfoTargets, 0));
            params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + lIdx + "Wave", "Waveform " + lIdx, juce::StringArray{ "Sine", "Saw", "Square", "Random(S&H)", "Noise" }, 0));
            params.push_back(std::make_unique<juce::AudioParameterBool>("lfo" + lIdx + "Sync", "Sync " + lIdx, false));
            params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo" + lIdx + "FreqFree", "Freq " + lIdx, juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.4f), 1.0f));
            params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + lIdx + "FreqSync", "Rate " + lIdx, juce::StringArray{ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2));
            params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo" + lIdx + "Min", "Min " + lIdx, 0.0f, 100.0f, 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo" + lIdx + "Max", "Max " + lIdx, 0.0f, 100.0f, 100.0f));
        }
    }

    params.push_back(std::make_unique<juce::AudioParameterInt>("activeSlot", "Active Slot", 0, 7, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("randomSlotEnable", "Random Slot", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outGain", "Master Gain", -36.0f, 12.0f, -6.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("materialMode", "Material", juce::StringArray{ "Auto", "Crisp", "Smooth", "Formant" }, 0));

    juce::StringArray fxChoices{ "None", "Mutator (Ring/Comb)", "Phantom (Dly)", "Warp (Frz)", "Abyss (Rev)" };
    for (int i = 1; i <= 4; ++i) {
        juce::String idx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>("fx" + idx + "Type", "FX Slot " + idx, fxChoices, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("fx" + idx + "Amount", "FX " + idx + " Amount", -100.0f, 100.0f, 0.0f));
    }

    params.push_back(std::make_unique<juce::AudioParameterFloat>("fxAegisAmount", "Aegis Limiter", -24.0f, 0.0f, 0.0f));
    return { params.begin(), params.end() };
}

Otodesk_samplerAudioProcessor::Otodesk_samplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), juce::Thread("AudioIngestThread"),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    formatManager.registerBasicFormats();
    for (int s = 0; s < 8; ++s) {
        waveformReady[s].store(false);
        waveformPeaks[s].fill(0.0f);
        for (int a = 0; a < 42; ++a) {
            anchorReady[s][a].store(false);
        }
    }
    validSlots.fill(0);
    cachedDetectedRoots.fill(-1);
}

Otodesk_samplerAudioProcessor::~Otodesk_samplerAudioProcessor()
{
    stopThread(4000);
    for (auto& slot : sharedAnchorBuffers) {
        for (auto& buf : slot) {
            buf.setSize(0, 0);
        }
    }
}

const juce::String Otodesk_samplerAudioProcessor::getName() const { return juce::String(JucePlugin_Name); }
bool Otodesk_samplerAudioProcessor::acceptsMidi() const { return true; }
bool Otodesk_samplerAudioProcessor::producesMidi() const { return true; }
bool Otodesk_samplerAudioProcessor::isMidiEffect() const { return false; }
double Otodesk_samplerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int Otodesk_samplerAudioProcessor::getNumPrograms() { return 1; }
int Otodesk_samplerAudioProcessor::getCurrentProgram() { return 0; }
void Otodesk_samplerAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String Otodesk_samplerAudioProcessor::getProgramName(int index) { juce::ignoreUnused(index); return juce::String(); }
void Otodesk_samplerAudioProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void Otodesk_samplerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    synth.prepareToPlay(sampleRate, samplesPerBlock);
    previewNoteOffTimer.store(0);
}
void Otodesk_samplerAudioProcessor::releaseResources() { synth.releaseResources(); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool Otodesk_samplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}
#endif

void Otodesk_samplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    for (auto i = 0; i < getTotalNumOutputChannels(); ++i) buffer.clear(i, 0, buffer.getNumSamples());

    if (triggerPreview.exchange(false)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        previewNoteOffTimer.store(24000);
    }
    if (previewNoteOffTimer.load() > 0) {
        int currentTimer = previewNoteOffTimer.load();
        currentTimer -= buffer.getNumSamples();
        if (currentTimer <= 0) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
            previewNoteOffTimer.store(0);
        }
        else { previewNoteOffTimer.store(currentTimer); }
    }

    double bpm = 120.0;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (pos->getBpm()) bpm = *pos->getBpm();
        }
    }

    OtodeskParams params;
    for (int i = 0; i < 8; ++i) {
        juce::String idx = juce::String(i + 1);
        params.attack[i] = apvts.getRawParameterValue("attack_" + idx)->load();
        params.release[i] = apvts.getRawParameterValue("release_" + idx)->load();
        params.sampleStart[i] = apvts.getRawParameterValue("sampleStart_" + idx)->load();
        params.sampleEnd[i] = apvts.getRawParameterValue("sampleEnd_" + idx)->load();
        params.loopStart[i] = apvts.getRawParameterValue("loopStart_" + idx)->load();
        params.loopLength[i] = apvts.getRawParameterValue("loopLength_" + idx)->load();
        params.crossfadeSize[i] = apvts.getRawParameterValue("crossfade_" + idx)->load();
        params.playMode[i] = (int)std::round(apvts.getRawParameterValue("playMode_" + idx)->load());
        params.pan[i] = apvts.getRawParameterValue("pan_" + idx)->load();
        params.slotGain[i] = apvts.getRawParameterValue("slotGain_" + idx)->load();

        params.octave[i] = (int)std::round(apvts.getRawParameterValue("octave_" + idx)->load());
        params.pitchSt[i] = (int)std::round(apvts.getRawParameterValue("pitchSt_" + idx)->load());
        params.fineTune[i] = apvts.getRawParameterValue("fineTune_" + idx)->load();

        auto getLfoParams = [&](int lfoNum, LFOParams& lfoP) {
            juce::String lIdx = juce::String(lfoNum) + "_" + idx;
            lfoP.enable = apvts.getRawParameterValue("lfo" + lIdx + "Enable")->load() > 0.5f;
            lfoP.target = (int)std::round(apvts.getRawParameterValue("lfo" + lIdx + "Target")->load());
            lfoP.waveform = (int)std::round(apvts.getRawParameterValue("lfo" + lIdx + "Wave")->load());
            lfoP.sync = apvts.getRawParameterValue("lfo" + lIdx + "Sync")->load() > 0.5f;
            lfoP.freqFree = apvts.getRawParameterValue("lfo" + lIdx + "FreqFree")->load();
            lfoP.freqSync = (int)std::round(apvts.getRawParameterValue("lfo" + lIdx + "FreqSync")->load());
            lfoP.minVal = apvts.getRawParameterValue("lfo" + lIdx + "Min")->load();
            lfoP.maxVal = apvts.getRawParameterValue("lfo" + lIdx + "Max")->load();
            };
        getLfoParams(1, params.lfo1[i]);
        getLfoParams(2, params.lfo2[i]);
        getLfoParams(3, params.lfo3[i]);
    }

    params.activeSlotOverride = (int)std::round(apvts.getRawParameterValue("activeSlot")->load());
    params.randomSlotEnable = apvts.getRawParameterValue("randomSlotEnable")->load() > 0.5f;
    params.outGainDb = apvts.getRawParameterValue("outGain")->load();

    params.fx1Type = (int)std::round(apvts.getRawParameterValue("fx1Type")->load());
    params.fx1Amount = apvts.getRawParameterValue("fx1Amount")->load() / 100.0f;
    params.fx2Type = (int)std::round(apvts.getRawParameterValue("fx2Type")->load());
    params.fx2Amount = apvts.getRawParameterValue("fx2Amount")->load() / 100.0f;
    params.fx3Type = (int)std::round(apvts.getRawParameterValue("fx3Type")->load());
    params.fx3Amount = apvts.getRawParameterValue("fx3Amount")->load() / 100.0f;
    params.fx4Type = (int)std::round(apvts.getRawParameterValue("fx4Type")->load());
    params.fx4Amount = apvts.getRawParameterValue("fx4Amount")->load() / 100.0f;
    params.fxAegisAmount = apvts.getRawParameterValue("fxAegisAmount")->load();

    OtodeskParams finalModP;
    synth.processBlock(buffer, midiMessages, &sharedAnchorBuffers, validSlots, params, bpm, finalModP, &anchorReady);
}

bool Otodesk_samplerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* Otodesk_samplerAudioProcessor::createEditor() { return new Otodesk_samplerAudioProcessorEditor(*this); }

// ==========================================
// プロジェクト保存時のキャッシュ記憶ロジック
// ==========================================
void Otodesk_samplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    juce::ScopedLock sl(stateLock);
    for (int i = 0; i < 8; ++i) {
        xml->setAttribute("AudioFilePath_" + juce::String(i), slotFiles[i].getFullPathName());
        xml->setAttribute("CachedRoot_" + juce::String(i), cachedDetectedRoots[i]);
    }
    copyXmlToBinary(*xml, destData);
}

void Otodesk_samplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        juce::ScopedLock sl(stateLock);
        isRestoringFromSave = true;
        for (int i = 0; i < 8; ++i) {
            cachedDetectedRoots[i] = xmlState->getIntAttribute("CachedRoot_" + juce::String(i), -1);
            juce::String savedFilePath = xmlState->getStringAttribute("AudioFilePath_" + juce::String(i), "");
            if (savedFilePath.isNotEmpty()) {
                juce::File savedFile(savedFilePath);
                if (savedFile.existsAsFile()) loadFileForIngest(savedFile, i);
            }
        }
    }
}

void Otodesk_samplerAudioProcessor::loadFileForIngest(const juce::File& file, int targetSlot)
{
    slotFiles[targetSlot] = file;
    if (!isRestoringFromSave) cachedDetectedRoots[targetSlot] = -1;
    {
        juce::ScopedLock sl(queueLock);
        if (std::find(ingestQueue.begin(), ingestQueue.end(), targetSlot) == ingestQueue.end()) {
            ingestQueue.push_back(targetSlot);
        }
    }
    if (!isThreadRunning()) startThread();
}

void Otodesk_samplerAudioProcessor::reingestCurrentFile() { triggerReanalyze(); }

void Otodesk_samplerAudioProcessor::triggerReanalyze()
{
    int targetSlot = (int)std::round(apvts.getRawParameterValue("activeSlot")->load());
    if (!slotFiles[targetSlot].existsAsFile()) return;
    isReanalyzing = true;
    isRestoringFromSave = false;
    {
        juce::ScopedLock sl(queueLock);
        ingestQueue.clear();
        ingestQueue.push_back(targetSlot);
    }
    if (isThreadRunning()) stopThread(2000);
    startThread();
}

juce::String Otodesk_samplerAudioProcessor::getFileName(int slotIndex) const {
    if (slotIndex >= 0 && slotIndex < 8) return slotFiles[slotIndex].getFileName();
    return "---";
}

void Otodesk_samplerAudioProcessor::clearSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;

    {
        juce::ScopedLock sl(queueLock);
        ingestQueue.erase(std::remove(ingestQueue.begin(), ingestQueue.end(), slotIndex), ingestQueue.end());
    }

    validSlots[slotIndex] = 0;
    slotFiles[slotIndex] = juce::File();
    cachedDetectedRoots[slotIndex] = -1;
    waveformPeaks[slotIndex].fill(0.0f);
    waveformReady[slotIndex].store(false);

    for (int i = 0; i < 42; ++i) {
        anchorReady[slotIndex][i].store(false);
        sharedAnchorBuffers[slotIndex][i].setSize(0, 0);
    }

    if (slotIndex == (int)std::round(apvts.getRawParameterValue("activeSlot")->load())) {
        juce::ScopedLock sl(stateLock);
        hudLineFile = "File: ---";
        hudLineTask = "Task: Empty";
        hudLineOrig = "Original: ---";
        sendChangeMessage();
    }
}

void Otodesk_samplerAudioProcessor::run()
{
    while (!threadShouldExit())
    {
        int processingSlot = -1;
        {
            juce::ScopedLock sl(queueLock);
            if (!ingestQueue.empty()) {
                processingSlot = ingestQueue.front();
                ingestQueue.erase(ingestQueue.begin());
            }
        }
        if (processingSlot == -1) break;

        juce::File targetFile = slotFiles[processingSlot];
        int activeSlot = (int)std::round(apvts.getRawParameterValue("activeSlot")->load());

        waveformReady[processingSlot].store(false);

        if (processingSlot == activeSlot) {
            ingestState.store(IngestState::Loading);
            ingestProgress.store(0.05f);
            juce::ScopedLock sl(stateLock);
            hudLineFile = "Slot " + juce::String(processingSlot + 1) + ": " + targetFile.getFileName();
            hudLineTask = "Task: Decoding Audio...";
        }

        std::unique_ptr<juce::AudioFormatReader> reader;
        auto stream = targetFile.createInputStream();
        if (stream != nullptr) {
            reader.reset(formatManager.createReaderFor(std::move(stream)));
        }

        if (!reader || reader->lengthInSamples == 0) {
            if (processingSlot == activeSlot) {
                ingestState.store(IngestState::Failed);
                juce::ScopedLock sl(stateLock);
                hudLineTask = "Task: Failed (Invalid Format)";
            }
            continue;
        }

        double fileSampleRate = reader->sampleRate;
        juce::AudioBuffer<float> tempRaw((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&tempRaw, 0, (int)reader->lengthInSamples, 0, true, true);

        if (fileSampleRate != currentSampleRate && fileSampleRate > 0.0) {
            double ratio = fileSampleRate / currentSampleRate;
            int newLength = (int)((double)tempRaw.getNumSamples() / ratio);
            juce::AudioBuffer<float> resampled(tempRaw.getNumChannels(), newLength);

            juce::AudioBuffer<float> paddedTemp(tempRaw.getNumChannels(), tempRaw.getNumSamples() + 8);
            paddedTemp.clear();
            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch) {
                paddedTemp.copyFrom(ch, 0, tempRaw, ch, 0, tempRaw.getNumSamples());
            }

            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch) {
                juce::LagrangeInterpolator interpolator;
                interpolator.process(ratio, paddedTemp.getReadPointer(ch), resampled.getWritePointer(ch), newLength);
            }
            tempRaw = resampled;
        }

        if (!isRestoringFromSave) {
            int totalSamps = tempRaw.getNumSamples();
            if (totalSamps > currentSampleRate * 0.1) {
                int startTarget = (int)(totalSamps * 0.15f);
                int endTarget = (int)(totalSamps * 0.65f);

                auto findZeroCross = [&](int start, int dir) -> int {
                    for (int i = 0; i < currentSampleRate && (start + i * dir) >= 0 && (start + i * dir + 1) < totalSamps; ++i) {
                        int idx = start + i * dir;
                        if (tempRaw.getSample(0, idx) <= 0.0f && tempRaw.getSample(0, idx + 1) > 0.0f) return idx;
                    }
                    return start;
                    };

                int zStart = findZeroCross(startTarget, 1);
                int zEnd = findZeroCross(endTarget, -1);
                if (zEnd <= zStart) { zStart = startTarget; zEnd = endTarget; }

                float normStart = (float)zStart / (float)totalSamps;
                float normLen = (float)(zEnd - zStart) / (float)totalSamps;

                juce::String slotStr = juce::String(processingSlot + 1);
                if (auto* pS = apvts.getParameter("loopStart_" + slotStr)) {
                    pS->setValueNotifyingHost(pS->convertTo0to1(normStart));
                }
                if (auto* pL = apvts.getParameter("loopLength_" + slotStr)) {
                    pL->setValueNotifyingHost(pL->convertTo0to1(normLen));
                }
            }
        }

        if (processingSlot == activeSlot) {
            ingestProgress.store(0.20f);
            ingestState.store(IngestState::Analyzing);
            juce::ScopedLock sl(stateLock); hudLineTask = "Task: Analyzing Root Key...";
        }

        double rootFreq = 261.625565;
        int finalMidi = 60;
        juce::String detectMethod = "Default";

        int manualRootKey = (int)std::round(apvts.getRawParameterValue("rootKey_" + juce::String(processingSlot + 1))->load());

        if (manualRootKey != -1) {
            finalMidi = manualRootKey;
            rootFreq = 440.0 * std::pow(2.0, (finalMidi - 69) / 12.0);
            detectMethod = "Manual";
            cachedDetectedRoots[processingSlot] = finalMidi;
        }
        else if (isRestoringFromSave && cachedDetectedRoots[processingSlot] != -1) {
            finalMidi = cachedDetectedRoots[processingSlot];
            rootFreq = 440.0 * std::pow(2.0, (finalMidi - 69) / 12.0);
            detectMethod = "Cache";
        }
        else {
            juce::String fn = targetFile.getFileNameWithoutExtension();
            fn = fn.replace("♯", "#").replace("♭", "b").replace(" ", "_");
            std::string fnStr = fn.toStdString();

            int exactMidi = -1;
            int hintPC = -1;

            std::regex exactRegex(R"((?:^|[^a-zA-Z0-9])([A-G])([#bB]?)(-?[0-9])(?:$|[^a-zA-Z0-9]))", std::regex_constants::icase);
            auto words_begin = std::sregex_iterator(fnStr.begin(), fnStr.end(), exactRegex);
            auto words_end = std::sregex_iterator();
            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                std::string note = match[1].str();
                std::string acc = match[2].str();
                int oct = std::stoi(match[3].str());

                char n = toupper(note[0]);
                int pc = 0;
                if (n == 'C') pc = 0; else if (n == 'D') pc = 2; else if (n == 'E') pc = 4;
                else if (n == 'F') pc = 5; else if (n == 'G') pc = 7; else if (n == 'A') pc = 9; else if (n == 'B') pc = 11;

                if (!acc.empty()) {
                    char a = tolower(acc[0]);
                    if (a == '#') pc++; else if (a == 'b') pc--;
                }
                if (pc < 0) pc += 12; if (pc > 11) pc -= 12;
                exactMidi = (oct + 2) * 12 + pc;
            }

            if (exactMidi == -1) {
                std::regex hintRegex(R"((?:^|[^a-zA-Z0-9])([A-G])([#bB]?)(?:$|[^a-zA-Z0-9]))", std::regex_constants::icase);
                words_begin = std::sregex_iterator(fnStr.begin(), fnStr.end(), hintRegex);
                for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                    std::smatch match = *i;
                    std::string note = match[1].str();
                    std::string acc = match[2].str();

                    char n = toupper(note[0]);
                    int pc = 0;
                    if (n == 'C') pc = 0; else if (n == 'D') pc = 2; else if (n == 'E') pc = 4;
                    else if (n == 'F') pc = 5; else if (n == 'G') pc = 7; else if (n == 'A') pc = 9; else if (n == 'B') pc = 11;

                    if (!acc.empty()) {
                        char a = tolower(acc[0]);
                        if (a == '#') pc++; else if (a == 'b') pc--;
                    }
                    if (pc < 0) pc += 12; if (pc > 11) pc -= 12;
                    hintPC = pc;
                }
            }

            int materialMode = (int)std::round(apvts.getRawParameterValue("materialMode")->load());
            PitchInfo info = analyzePitchOmni(tempRaw.getReadPointer(0), tempRaw.getNumSamples(), currentSampleRate, targetFile.getFileName(), materialMode);

            if (exactMidi != -1) {
                finalMidi = exactMidi;
                if (info.isDetected && std::abs(info.midiNote - exactMidi) <= 12) {
                    double semitoneDiff = exactMidi - info.midiNote;
                    rootFreq = info.frequency * std::pow(2.0, semitoneDiff / 12.0);
                    detectMethod = "Exact + DSP Tune";
                }
                else {
                    rootFreq = 440.0 * std::pow(2.0, (finalMidi - 69) / 12.0);
                    detectMethod = "File Name (Exact)";
                }
            }
            else if (hintPC != -1) {
                if (info.isDetected) {
                    int detectedOctave = info.midiNote / 12;
                    finalMidi = detectedOctave * 12 + hintPC;
                    if (finalMidi < info.midiNote - 6) finalMidi += 12;
                    else if (finalMidi > info.midiNote + 6) finalMidi -= 12;

                    double semitoneDiff = finalMidi - info.midiNote;
                    rootFreq = info.frequency * std::pow(2.0, semitoneDiff / 12.0);
                    detectMethod = "Hint + DSP Tune";
                }
                else {
                    finalMidi = 60 + hintPC;
                    if (hintPC > 6) finalMidi -= 12;
                    rootFreq = 440.0 * std::pow(2.0, (finalMidi - 69) / 12.0);
                    detectMethod = "File Name (Hint)";
                }
            }
            else {
                if (info.isDetected) {
                    finalMidi = info.midiNote;
                    rootFreq = info.frequency;
                    detectMethod = "DSP Analysis";
                }
            }
            cachedDetectedRoots[processingSlot] = finalMidi;
        }

        const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::String dispOrig = juce::String(noteNames[juce::jmax(0, finalMidi) % 12]) + juce::String((finalMidi / 12) - 2);

        int rootIdxForDisplay = juce::jlimit(0, 41, (int)std::round(finalMidi / 3.0));
        int minRbMidi = juce::jmax(0, rootIdxForDisplay - 16) * 3;
        int maxRbMidi = juce::jmin(41, rootIdxForDisplay + 16) * 3;
        juce::String minN = juce::String(noteNames[minRbMidi % 12]) + juce::String((minRbMidi / 12) - 2);
        juce::String maxN = juce::String(noteNames[maxRbMidi % 12]) + juce::String((maxRbMidi / 12) - 2);

        if (processingSlot == activeSlot) {
            ingestState.store(IngestState::Stretching);
            juce::ScopedLock sl(stateLock);
            hudLineOrig = "Original: " + dispOrig + " (" + minN + " ~ " + maxN + ") / " + juce::String(rootFreq, 1) + " Hz [" + detectMethod + "]";
            hudLineTask = "Task: Generating Anchors (Hybrid)...";
        }

        int rootIdx = rootIdxForDisplay;
        std::vector<int> processOrder;
        processOrder.push_back(rootIdx);

        std::vector<int> remaining;
        for (int i = 0; i < 42; ++i) {
            if (i != rootIdx) remaining.push_back(i);
        }
        std::sort(remaining.begin(), remaining.end(), [&](int a, int b) {
            return std::abs(a - rootIdx) < std::abs(b - rootIdx);
            });
        processOrder.insert(processOrder.end(), remaining.begin(), remaining.end());

        int rbOptions = RubberBand::RubberBandStretcher::OptionProcessOffline
            | RubberBand::RubberBandStretcher::OptionPitchHighQuality
            | RubberBand::RubberBandStretcher::OptionWindowShort;

        anchorsCompleted.store(0);
        juce::Time startTime = juce::Time::getCurrentTime();

        for (int i = 0; i < 42; ++i) {
            anchorReady[processingSlot][i].store(false);
            sharedAnchorBuffers[processingSlot][i].setSize(0, 0);
        }

        std::vector<std::thread> workers;
        std::atomic<int> nextQueueIdx{ 0 };
        int numCores = juce::jmax(1, juce::SystemStats::getNumCpus() - 1);

        for (int t = 0; t < numCores; ++t) {
            workers.emplace_back([&, this]() {
                while (!threadShouldExit()) {
                    int qIdx = nextQueueIdx.fetch_add(1);
                    if (qIdx >= 42) break;

                    int anchorIdx = processOrder[qIdx];
                    double targetFreq = 440.0 * std::pow(2.0, ((anchorIdx * 3) - 69) / 12.0);
                    double pitchScale = targetFreq / rootFreq;
                    auto& destBuffer = sharedAnchorBuffers[processingSlot][anchorIdx];

                    if (std::abs(pitchScale - 1.0) < 0.001) {
                        int avail = tempRaw.getNumSamples();
                        if (avail > 0) {
                            destBuffer.setSize(2, avail, false, true, true);
                            destBuffer.clear();
                            destBuffer.copyFrom(0, 0, tempRaw, 0, 0, avail);
                            destBuffer.copyFrom(1, 0, tempRaw, tempRaw.getNumChannels() > 1 ? 1 : 0, 0, avail);
                        }
                    }
                    else if (std::abs(anchorIdx - rootIdx) > 16) {
                        int newLen = (int)((double)tempRaw.getNumSamples() / pitchScale);
                        if (newLen > 0) {
                            destBuffer.setSize(2, newLen, false, true, true);

                            juce::AudioBuffer<float> paddedIn(tempRaw.getNumChannels(), tempRaw.getNumSamples() + 8);
                            paddedIn.clear();
                            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch) {
                                paddedIn.copyFrom(ch, 0, tempRaw, ch, 0, tempRaw.getNumSamples());
                            }

                            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch) {
                                juce::LagrangeInterpolator li;
                                li.process(pitchScale, paddedIn.getReadPointer(ch), destBuffer.getWritePointer(ch % 2), newLen);
                            }
                            if (tempRaw.getNumChannels() == 1) {
                                destBuffer.copyFrom(1, 0, destBuffer, 0, 0, newLen);
                            }
                        }
                    }
                    else {
                        RubberBand::RubberBandStretcher stretcher((size_t)currentSampleRate, (size_t)tempRaw.getNumChannels(), rbOptions);
                        stretcher.setPitchScale(pitchScale);
                        stretcher.process(tempRaw.getArrayOfReadPointers(), (size_t)tempRaw.getNumSamples(), true);
                        int avail = (int)stretcher.available();
                        if (avail > 0) {
                            destBuffer.setSize(2, avail, false, true, true);
                            destBuffer.clear();
                            if (tempRaw.getNumChannels() == 1) {
                                juce::AudioBuffer<float> mono(1, avail);
                                stretcher.retrieve(mono.getArrayOfWritePointers(), (size_t)avail);
                                destBuffer.copyFrom(0, 0, mono, 0, 0, avail);
                                destBuffer.copyFrom(1, 0, mono, 0, 0, avail);
                            }
                            else {
                                stretcher.retrieve(destBuffer.getArrayOfWritePointers(), (size_t)avail);
                            }
                        }
                    }

                    anchorReady[processingSlot][anchorIdx].store(true);
                    anchorsCompleted++;

                    if (anchorIdx == rootIdx) {
                        int avail = destBuffer.getNumSamples();
                        validSampleCount.store(avail);
                        if (avail > 0) {
                            waveformPeaks[processingSlot].fill(0.0f);
                            size_t step = (size_t)juce::jmax(1, avail / 1000);
                            for (size_t i = 0; i < 1000; ++i) {
                                size_t len = (size_t)juce::jmin((int)step, avail - (int)(i * step));
                                if (len > 0) {
                                    waveformPeaks[processingSlot][i] = destBuffer.getMagnitude((int)(i * step), (int)len);
                                }
                            }
                            waveformReady[processingSlot].store(true);
                            validSlots[processingSlot] = 1;
                        }
                    }
                }
                });
        }

        while (anchorsCompleted.load() < 42) {
            if (threadShouldExit()) break;

            int completed = anchorsCompleted.load();

            if (processingSlot == (int)std::round(apvts.getRawParameterValue("activeSlot")->load())) {
                ingestProgress.store(0.2f + 0.8f * ((float)completed / 42.0f));

                if (completed > 2) {
                    juce::ScopedLock sl(etaLock);
                    double elapsedMs = (juce::Time::getCurrentTime() - startTime).inMilliseconds();
                    int eta = (int)(((elapsedMs / completed) * (42 - completed)) / 1000.0);

                    juce::ScopedLock slState(stateLock);
                    hudLineTask = "Task: Optimizing " + juce::String(completed) + "/42 (ETA: " + juce::String(eta) + "s)";
                }
            }
            juce::Thread::sleep(50);
        }

        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }

        if (processingSlot == (int)std::round(apvts.getRawParameterValue("activeSlot")->load())) {
            ingestProgress.store(1.0f);
            ingestState.store(IngestState::Ready);
            {
                juce::ScopedLock sl(stateLock);
                hudLineTask = "Task: High-Fidelity Ready";
            }
            sendChangeMessage();
        }
    }
    isRestoringFromSave = false;
}

PitchInfo Otodesk_samplerAudioProcessor::analyzePitch(const float* audioData, int length, double sampleRate, float minFreq, float corrThreshold)
{
    PitchInfo info;
    if (length < 1024) return info;

    int minLag = (int)(sampleRate / 1200.0);
    int maxLag = (int)(sampleRate / (double)minFreq);
    maxLag = juce::jmin(maxLag, length / 2);

    std::vector<float> corr((size_t)maxLag, 0.0f);
    float maxCorr = 0.0f;
    int bestLag = -1;

    for (int lag = minLag; lag < maxLag; ++lag) {
        float sum = 0.0f;
        for (size_t i = 0; i < (size_t)(length / 2); ++i) sum += audioData[i] * audioData[i + (size_t)lag];
        corr[(size_t)lag] = sum;
        if (sum > maxCorr) { maxCorr = sum; bestLag = lag; }
    }

    float energy = 0.0f;
    for (size_t i = 0; i < (size_t)(length / 2); ++i) energy += audioData[i] * audioData[i];

    if (energy > 0.0001f && bestLag > minLag && bestLag < maxLag - 1 && (maxCorr / energy) > corrThreshold) {
        float alpha = corr[(size_t)bestLag - 1];
        float beta = corr[(size_t)bestLag];
        float gamma = corr[(size_t)bestLag + 1];
        float peakOffset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);

        double refinedLag = (double)bestLag + (double)peakOffset;
        info.frequency = sampleRate / refinedLag;

        if (info.frequency > 15.0 && info.frequency < 5000.0) {
            double midiFloat = 69.0 + 12.0 * std::log2(info.frequency / 440.0);
            info.midiNote = juce::roundToInt(midiFloat);
            info.isDetected = true;
        }
    }
    return info;
}

PitchInfo Otodesk_samplerAudioProcessor::analyzePitchCepstrum(const float* audioData, int length, double sampleRate)
{
    PitchInfo info;
    int dftSize = 8192;
    int windowSize = juce::jmin(length, 4096);

    if (length < 2048) return info;

    std::vector<float> windowed(dftSize, 0.0f);
    for (int i = 0; i < windowSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(windowSize - 1)));
        windowed[i] = audioData[i] * window;
    }

    std::vector<float> mag(dftSize, 0.0f);
    for (int k = 0; k < dftSize / 2; ++k) {
        float re = 0.0f, im = 0.0f;
        float phaseInc = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
        for (int n = 0; n < dftSize; ++n) {
            re += windowed[n] * std::cos(phaseInc * (float)n);
            im -= windowed[n] * std::sin(phaseInc * (float)n);
        }
        mag[k] = std::log(std::sqrt(re * re + im * im) + 1e-6f);
    }

    for (int k = 1; k < dftSize / 2; ++k) {
        mag[dftSize - k] = mag[k];
    }

    std::vector<float> cepstrum(dftSize, 0.0f);
    for (int n = 0; n < dftSize; ++n) {
        float re = 0.0f;
        float phaseInc = juce::MathConstants<float>::twoPi * (float)n / (float)dftSize;
        for (int k = 0; k < dftSize; ++k) {
            re += mag[k] * std::cos(phaseInc * (float)k);
        }
        cepstrum[n] = re / (float)dftSize;
    }

    int minQ = (int)(sampleRate / 2000.0);
    int maxQ = (int)(sampleRate / 20.0);
    maxQ = juce::jmin(maxQ, dftSize / 2);

    float maxCep = 0.0f;
    int bestQ = -1;
    for (int q = minQ; q <= maxQ; ++q) {
        float weightedCep = cepstrum[q] * std::sqrt((float)q);
        if (weightedCep > maxCep) {
            maxCep = weightedCep;
            bestQ = q;
        }
    }

    if (bestQ > minQ && bestQ < maxQ) {
        float alpha = cepstrum[bestQ - 1];
        float beta = cepstrum[bestQ];
        float gamma = cepstrum[bestQ + 1];

        float denom = alpha - 2.0f * beta + gamma;
        float peakOffset = (std::abs(denom) > 1e-6f) ? (0.5f * (alpha - gamma) / denom) : 0.0f;

        double refinedQ = (double)bestQ + (double)peakOffset;
        info.frequency = sampleRate / refinedQ;

        if (info.frequency >= 20.0 && info.frequency <= 2000.0) {
            info.midiNote = juce::roundToInt(69.0 + 12.0 * std::log2(info.frequency / 440.0));
            info.isDetected = true;
        }
    }
    return info;
}

PitchInfo Otodesk_samplerAudioProcessor::analyzePitchOmni(const float* audioData, int length, double sampleRate, const juce::String& filename, int materialMode)
{
    enum Route { Standard, KickFFT, HatCym, Loop, Micro, Cepstrum };
    std::vector<Route> plan;

    if (materialMode == 1) plan = { KickFFT, HatCym, Standard };
    else if (materialMode == 2) plan = { Loop, Standard };
    else if (materialMode == 3) plan = { Standard, Micro };
    else {
        juce::String fn = filename.toLowerCase();
        if (length < 2048) plan = { Micro, Standard };
        else if (fn.contains("bass") || fn.contains("sub") || fn.contains("808") || fn.contains("koto") || fn.contains("piano") || fn.contains("string")) plan = { Cepstrum, KickFFT, Standard };
        else if (fn.contains("kick") || fn.contains("drum")) plan = { KickFFT, Cepstrum, Standard };
        else if (fn.contains("hat") || fn.contains("cym") || fn.contains("snare") || fn.contains("perc")) plan = { HatCym, Standard };
        else if (fn.contains("loop") || length > (int)(sampleRate * 2.0)) plan = { Loop, Standard };
        else plan = { Cepstrum, Standard, KickFFT };
    }

    PitchInfo finalResult;
    for (Route r : plan) {
        PitchInfo result;
        if (r == Standard) {
            result = analyzePitch(audioData, length, sampleRate, 40.0f, 0.35f);
        }
        else if (r == Cepstrum) {
            result = analyzePitchCepstrum(audioData, length, sampleRate);
        }
        else if (r == KickFFT) {
            int skipSamples = (int)(sampleRate * 0.04);
            if (length - skipSamples > 1024) {
                int dftSize = juce::jmin(length - skipSamples, 8192);
                const float* tail = audioData + skipSamples;
                float maxMag = 0.0f;
                int peakK = -1;
                int kMin = juce::jmax(1, (int)(20.0 * (double)dftSize / sampleRate));
                int kMax = (int)(150.0 * (double)dftSize / sampleRate);
                std::vector<float> magnitudes((size_t)kMax + 2, 0.0f);

                for (int k = kMin; k <= kMax; ++k) {
                    float re = 0.0f, im = 0.0f;
                    float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                    for (int n = 0; n < dftSize; ++n) {
                        float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)n / (float)(dftSize - 1)));
                        float sample = tail[n] * window;
                        re += sample * std::cos(freqPhase * (float)n);
                        im -= sample * std::sin(freqPhase * (float)n);
                    }
                    float mag = re * re + im * im;
                    magnitudes[(size_t)k] = mag;
                    if (mag > maxMag) { maxMag = mag; peakK = k; }
                }

                if (peakK > kMin && peakK < kMax) {
                    float alpha = magnitudes[(size_t)peakK - 1];
                    float beta = magnitudes[(size_t)peakK];
                    float gamma = magnitudes[(size_t)peakK + 1];

                    float denom = alpha - 2.0f * beta + gamma;
                    float peakOffset = (std::abs(denom) > 1e-6f) ? (0.5f * (alpha - gamma) / denom) : 0.0f;

                    double refinedK = (double)peakK + (double)peakOffset;
                    result.frequency = refinedK * sampleRate / (double)dftSize;

                    if (result.frequency >= 20.0 && result.frequency <= 150.0) {
                        result.midiNote = juce::roundToInt(69.0 + 12.0 * std::log2(result.frequency / 440.0));
                        result.isDetected = true;
                    }
                }
            }
        }
        else if (r == HatCym) {
            size_t downFactor = 4;
            size_t downLen = (size_t)length / downFactor;
            if (downLen > 512) {
                std::vector<float> down(downLen);
                for (size_t i = 0; i < downLen; ++i) down[i] = audioData[i * downFactor];
                int dftSize = juce::jmin((int)downLen, 4096);
                float maxMagnitude = 0.0f;
                int peakBin = -1;
                for (int k = 1; k < dftSize / 2; ++k) {
                    float re = 0.0f, im = 0.0f;
                    float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                    for (int n = 0; n < dftSize; ++n) {
                        re += down[(size_t)n] * std::cos(freqPhase * (float)n);
                        im -= down[(size_t)n] * std::sin(freqPhase * (float)n);
                    }
                    float mag = re * re + im * im;
                    if (mag > maxMagnitude) { maxMagnitude = mag; peakBin = k; }
                }
                if (peakBin > 0) {
                    double downFreq = (double)peakBin * (sampleRate / (double)downFactor) / (double)dftSize;
                    result.frequency = downFreq * (double)downFactor;
                    if (result.frequency > 20.0 && result.frequency < 10000.0) {
                        result.midiNote = juce::roundToInt(69.0 + 12.0 * std::log2(result.frequency / 440.0));
                        result.isDetected = true;
                    }
                }
            }
        }
        else if (r == Loop || r == Micro) {
            int cutLen = (r == Loop) ? juce::jmin(length, (int)(sampleRate * 0.3)) : length;
            result = analyzePitch(audioData, cutLen, sampleRate, 30.0f, 0.3f);
        }

        if (result.isDetected) {
            finalResult = result;
            break;
        }
    }
    return finalResult;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Otodesk_samplerAudioProcessor(); }