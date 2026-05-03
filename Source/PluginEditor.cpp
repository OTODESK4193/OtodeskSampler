// ==========================================
// File: PluginEditor.cpp
// ==========================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

Otodesk_samplerAudioProcessorEditor::Otodesk_samplerAudioProcessorEditor(Otodesk_samplerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&abletonLnF);

    auto setupTerminalLabel = [this](juce::Label& lbl) {
        lbl.setJustificationType(juce::Justification::centredLeft);
        lbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        lbl.setColour(juce::Label::textColourId, VividPastelTheme::slotColors[0]);
        lbl.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(lbl);
        };

    setupTerminalLabel(statusLabelFile);
    setupTerminalLabel(statusLabelTask);
    setupTerminalLabel(statusLabelOrig);

    setupSlotSlider(attackLabel, attackSlider, "Attack");
    setupSlotSlider(releaseLabel, releaseSlider, "Release");
    setupSlotSlider(sampleStartLabel, sampleStartSlider, "Sample Start");
    setupSlotSlider(sampleEndLabel, sampleEndSlider, "Sample End");
    setupSlotSlider(loopStartLabel, loopStartSlider, "Loop Start");
    setupSlotSlider(loopLengthLabel, loopLengthSlider, "Loop Length");
    setupSlotSlider(crossfadeLabel, crossfadeSlider, "X-Fade");

    setupSlotSlider(panLabel, panSlider, "Slot Pan");
    setupSlotSlider(slotGainLabel, slotGainSlider, "Slot Gain");
    slotGainSlider.setTextValueSuffix(" dB");

    setupSlotSlider(octLabel, octSlider, "Octave");
    setupSlotSlider(semiLabel, semiSlider, "Semi (st)");
    setupSlotSlider(fineLabel, fineSlider, "FineTune");

    playModeLabel.setText("Mode:", juce::dontSendNotification);
    playModeLabel.setJustificationType(juce::Justification::centredRight);
    playModeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(playModeLabel);
    playModeCombo.addItemList({ "Loop", "One-Shot" }, 1);
    addAndMakeVisible(playModeCombo);

    rootKeyLabel.setText("RootKey:", juce::dontSendNotification);
    rootKeyLabel.setJustificationType(juce::Justification::centredRight);
    rootKeyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(rootKeyLabel);

    rootKeyCombo.addItem("Auto", 1);
    for (int i = 4; i <= 127; ++i) {
        const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::String noteName = juce::String(names[i % 12]) + juce::String((i / 12) - 2);
        rootKeyCombo.addItem(noteName, i - 4 + 2);
    }
    addAndMakeVisible(rootKeyCombo);

    addAndMakeVisible(reanalyzeButton);
    reanalyzeButton.onClick = [this] { audioProcessor.triggerReanalyze(); };

    setupGlobalSlider(gainLabel, gainSlider, gainAttach, "outGain", "Master Gain");
    gainSlider.setTextValueSuffix(" dB");

    for (int i = 0; i < 8; ++i) addAndMakeVisible(slotButtons[i]);

    randomSlotButton.setButtonText("Random Slot");
    addAndMakeVisible(randomSlotButton);
    randomSlotAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "randomSlotEnable", randomSlotButton);

    materialLabel.setText("Material:", juce::dontSendNotification);
    materialLabel.setJustificationType(juce::Justification::centredRight);
    materialLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(materialLabel);
    materialCombo.addItemList({ "Auto", "Crisp", "Smooth", "Formant" }, 1);
    addAndMakeVisible(materialCombo);
    materialAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "materialMode", materialCombo);

    materialCombo.onChange = [this] {
        int slot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
        if (audioProcessor.getFileName(slot) != "---") audioProcessor.triggerReanalyze();
        };

    // 【重要】拡張されたLFOターゲット13種類をUIに反映
    juce::StringArray lfoTargets{ "None", "Loop Start", "Loop Length", "X-Fade", "Pan", "Sample Start", "Pitch (Oct)", "Pitch (Semi)", "Pitch (Fine)", "FX 1 Amount", "FX 2 Amount", "FX 3 Amount", "FX 4 Amount" };
    juce::StringArray lfoWaves = { "Sine", "Saw", "Square", "Random(S&H)", "Noise" };
    juce::StringArray lfoRates = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };

    for (int i = 0; i < 3; ++i) {
        auto& ui = lfoUIs[i];
        juce::String idx = juce::String(i + 1);

        ui.enableBtn.setButtonText("LFO " + idx);
        addAndMakeVisible(ui.enableBtn);

        ui.targetCombo.addItemList(lfoTargets, 1);
        addAndMakeVisible(ui.targetCombo);

        ui.waveCombo.addItemList(lfoWaves, 1);
        addAndMakeVisible(ui.waveCombo);

        ui.syncBtn.setButtonText("Sync");
        addAndMakeVisible(ui.syncBtn);

        ui.freqFreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        ui.freqFreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
        addAndMakeVisible(ui.freqFreeSlider);

        ui.freqSyncCombo.addItemList(lfoRates, 1);
        addChildComponent(ui.freqSyncCombo);

        ui.minLabel.setText("Min", juce::dontSendNotification);
        ui.minLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(ui.minLabel);
        ui.minSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        ui.minSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
        ui.minSlider.setLookAndFeel(&noSliderLnF);
        ui.minSlider.setTextValueSuffix("%");
        addAndMakeVisible(ui.minSlider);

        ui.maxLabel.setText("Max", juce::dontSendNotification);
        ui.maxLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(ui.maxLabel);
        ui.maxSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        ui.maxSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
        ui.maxSlider.setLookAndFeel(&noSliderLnF);
        ui.maxSlider.setTextValueSuffix("%");
        addAndMakeVisible(ui.maxSlider);
    }

    juce::StringArray fxChoices{ "None", "Mutator (Ring/Comb)", "Phantom (Dly)", "Warp (Frz)", "Abyss (Rev)" };
    for (int i = 0; i < 4; ++i) {
        juce::String idx = juce::String(i + 1);
        fxSlots[i].typeCombo.addItemList(fxChoices, 1);
        addAndMakeVisible(fxSlots[i].typeCombo);
        fxSlots[i].typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "fx" + idx + "Type", fxSlots[i].typeCombo);

        fxSlots[i].amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        fxSlots[i].amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        fxSlots[i].amountKnob.setTextValueSuffix("%");
        addAndMakeVisible(fxSlots[i].amountKnob);
        fxSlots[i].amountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "fx" + idx + "Amount", fxSlots[i].amountKnob);
    }

    aegisLabel.setText("Aegis Limit", juce::dontSendNotification);
    aegisLabel.setJustificationType(juce::Justification::centred);
    aegisLabel.setColour(juce::Label::textColourId, VividPastelTheme::slotColors[0]);
    addAndMakeVisible(aegisLabel);

    aegisKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    aegisKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    aegisKnob.setTextValueSuffix(" dB");
    addAndMakeVisible(aegisKnob);
    aegisAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "fxAegisAmount", aegisKnob);

    updateSlotAttachments();
    updateThemeColor((int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load()));

    audioProcessor.addChangeListener(this);
    vblankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { vblankCallback(); });

    setSize(840, 820);
}

Otodesk_samplerAudioProcessorEditor::~Otodesk_samplerAudioProcessorEditor() {
    removeAllChildren();
    for (auto& ui : lfoUIs) { ui.minSlider.setLookAndFeel(nullptr); ui.maxSlider.setLookAndFeel(nullptr); }
    setLookAndFeel(nullptr);
    audioProcessor.removeChangeListener(this);
}

void Otodesk_samplerAudioProcessorEditor::updateSlotAttachments()
{
    int slot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load()) + 1;
    juce::String s = juce::String(slot);

    attackAttach.reset(); releaseAttach.reset();
    sampleStartAttach.reset(); sampleEndAttach.reset();
    loopStartAttach.reset(); loopLengthAttach.reset();
    crossfadeAttach.reset(); playModeAttach.reset();
    panAttach.reset(); slotGainAttach.reset();
    octAttach.reset(); semiAttach.reset(); fineAttach.reset();

    attackSlider.setValue(audioProcessor.apvts.getRawParameterValue("attack_" + s)->load(), juce::dontSendNotification);
    releaseSlider.setValue(audioProcessor.apvts.getRawParameterValue("release_" + s)->load(), juce::dontSendNotification);
    sampleStartSlider.setValue(audioProcessor.apvts.getRawParameterValue("sampleStart_" + s)->load(), juce::dontSendNotification);
    sampleEndSlider.setValue(audioProcessor.apvts.getRawParameterValue("sampleEnd_" + s)->load(), juce::dontSendNotification);
    loopStartSlider.setValue(audioProcessor.apvts.getRawParameterValue("loopStart_" + s)->load(), juce::dontSendNotification);
    loopLengthSlider.setValue(audioProcessor.apvts.getRawParameterValue("loopLength_" + s)->load(), juce::dontSendNotification);
    crossfadeSlider.setValue(audioProcessor.apvts.getRawParameterValue("crossfade_" + s)->load(), juce::dontSendNotification);
    playModeCombo.setSelectedId((int)std::round(audioProcessor.apvts.getRawParameterValue("playMode_" + s)->load()) + 1, juce::dontSendNotification);
    panSlider.setValue(audioProcessor.apvts.getRawParameterValue("pan_" + s)->load(), juce::dontSendNotification);
    slotGainSlider.setValue(audioProcessor.apvts.getRawParameterValue("slotGain_" + s)->load(), juce::dontSendNotification);

    octSlider.setValue(audioProcessor.apvts.getRawParameterValue("octave_" + s)->load(), juce::dontSendNotification);
    semiSlider.setValue(audioProcessor.apvts.getRawParameterValue("pitchSt_" + s)->load(), juce::dontSendNotification);
    fineSlider.setValue(audioProcessor.apvts.getRawParameterValue("fineTune_" + s)->load(), juce::dontSendNotification);

    attackAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "attack_" + s, attackSlider));
    releaseAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "release_" + s, releaseSlider));
    sampleStartAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "sampleStart_" + s, sampleStartSlider));
    sampleEndAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "sampleEnd_" + s, sampleEndSlider));
    loopStartAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "loopStart_" + s, loopStartSlider));
    loopLengthAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "loopLength_" + s, loopLengthSlider));
    crossfadeAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "crossfade_" + s, crossfadeSlider));
    playModeAttach.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.apvts, "playMode_" + s, playModeCombo));
    panAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "pan_" + s, panSlider));
    slotGainAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "slotGain_" + s, slotGainSlider));

    octAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "octave_" + s, octSlider));
    semiAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "pitchSt_" + s, semiSlider));
    fineAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "fineTune_" + s, fineSlider));

    rootKeyCombo.onChange = nullptr;
    int rk = (int)std::round(audioProcessor.apvts.getRawParameterValue("rootKey_" + s)->load());
    if (rk == -1) rootKeyCombo.setSelectedId(1, juce::dontSendNotification);
    else if (rk >= 4 && rk <= 127) rootKeyCombo.setSelectedId(rk - 4 + 2, juce::dontSendNotification);

    rootKeyCombo.onChange = [this, s]() {
        int id = rootKeyCombo.getSelectedId();
        int val = (id == 1) ? -1 : (id - 2 + 4);
        audioProcessor.apvts.getParameterAsValue("rootKey_" + s).setValue(val);
        };

    for (int l = 0; l < 3; ++l) {
        juce::String lIdx = juce::String(l + 1) + "_" + s;
        auto& ui = lfoUIs[l];

        ui.enableAttach.reset(); ui.targetAttach.reset(); ui.waveAttach.reset();
        ui.syncAttach.reset(); ui.freqFreeAttach.reset(); ui.freqSyncAttach.reset();
        ui.minAttach.reset(); ui.maxAttach.reset();

        ui.enableBtn.setToggleState(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Enable")->load() > 0.5f, juce::dontSendNotification);
        ui.targetCombo.setSelectedId((int)std::round(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Target")->load()) + 1, juce::dontSendNotification);
        ui.waveCombo.setSelectedId((int)std::round(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Wave")->load()) + 1, juce::dontSendNotification);
        ui.syncBtn.setToggleState(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Sync")->load() > 0.5f, juce::dontSendNotification);
        ui.freqFreeSlider.setValue(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "FreqFree")->load(), juce::dontSendNotification);
        ui.freqSyncCombo.setSelectedId((int)std::round(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "FreqSync")->load()) + 1, juce::dontSendNotification);
        ui.minSlider.setValue(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Min")->load(), juce::dontSendNotification);
        ui.maxSlider.setValue(audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Max")->load(), juce::dontSendNotification);

        ui.enableAttach.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(audioProcessor.apvts, "lfo" + lIdx + "Enable", ui.enableBtn));
        ui.targetAttach.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.apvts, "lfo" + lIdx + "Target", ui.targetCombo));
        ui.waveAttach.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.apvts, "lfo" + lIdx + "Wave", ui.waveCombo));
        ui.syncAttach.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(audioProcessor.apvts, "lfo" + lIdx + "Sync", ui.syncBtn));
        ui.freqFreeAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "lfo" + lIdx + "FreqFree", ui.freqFreeSlider));
        ui.freqSyncAttach.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(audioProcessor.apvts, "lfo" + lIdx + "FreqSync", ui.freqSyncCombo));
        ui.minAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "lfo" + lIdx + "Min", ui.minSlider));
        ui.maxAttach.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "lfo" + lIdx + "Max", ui.maxSlider));
    }
}

void Otodesk_samplerAudioProcessorEditor::updateThemeColor(int slotIndex)
{
    juce::Colour c = VividPastelTheme::slotColors[slotIndex];
    abletonLnF.currentAccentColor = c;

    statusLabelFile.setColour(juce::Label::textColourId, c);
    attackLabel.setColour(juce::Label::textColourId, c);
    releaseLabel.setColour(juce::Label::textColourId, c);
    sampleStartLabel.setColour(juce::Label::textColourId, c);
    sampleEndLabel.setColour(juce::Label::textColourId, c);
    loopStartLabel.setColour(juce::Label::textColourId, c);
    loopLengthLabel.setColour(juce::Label::textColourId, c);
    crossfadeLabel.setColour(juce::Label::textColourId, c);
    aegisLabel.setColour(juce::Label::textColourId, c);
    panLabel.setColour(juce::Label::textColourId, c);
    slotGainLabel.setColour(juce::Label::textColourId, c);
    octLabel.setColour(juce::Label::textColourId, c);
    semiLabel.setColour(juce::Label::textColourId, c);
    fineLabel.setColour(juce::Label::textColourId, c);

    repaint();
}

void Otodesk_samplerAudioProcessorEditor::setupGlobalSlider(juce::Label& lbl, juce::Slider& sld, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att, const juce::String& id, const juce::String& name)
{
    lbl.setText(name, juce::dontSendNotification);
    lbl.setJustificationType(juce::Justification::centredRight);
    lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(lbl);
    sld.setSliderStyle(juce::Slider::LinearHorizontal);
    sld.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 55, 20);
    addAndMakeVisible(sld);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id, sld);
}

void Otodesk_samplerAudioProcessorEditor::setupSlotSlider(juce::Label& lbl, juce::Slider& sld, const juce::String& name)
{
    lbl.setText(name, juce::dontSendNotification);
    lbl.setJustificationType(juce::Justification::centredRight);
    lbl.setColour(juce::Label::textColourId, VividPastelTheme::slotColors[0]);
    addAndMakeVisible(lbl);
    sld.setSliderStyle(juce::Slider::LinearHorizontal);
    sld.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 55, 20);
    addAndMakeVisible(sld);
}

float Otodesk_samplerAudioProcessorEditor::getZeroCrossedPosition(float normalizedPos, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8 || audioProcessor.validSlots[slotIndex] == 0) return normalizedPos;

    int rk = (int)std::round(audioProcessor.apvts.getRawParameterValue("rootKey_" + juce::String(slotIndex + 1))->load());
    if (rk == -1) rk = audioProcessor.cachedDetectedRoots[slotIndex];
    if (rk == -1) rk = 60;

    int centerAnchor = juce::jlimit(0, 41, (int)std::round(rk / 3.0));
    auto& buf = audioProcessor.sharedAnchorBuffers[slotIndex][centerAnchor];
    int numSamples = buf.getNumSamples();
    if (numSamples <= 0) return normalizedPos;

    int targetIdx = (int)(normalizedPos * numSamples);
    targetIdx = juce::jlimit(0, numSamples - 2, targetIdx);
    const float* data = buf.getReadPointer(0);

    int searchRange = 512;
    int bestIdx = targetIdx;
    int minDistance = searchRange + 1;

    for (int i = -searchRange; i <= searchRange; ++i) {
        int idx = targetIdx + i;
        if (idx >= 0 && idx < numSamples - 1) {
            if (data[idx] <= 0.0f && data[idx + 1] > 0.0f) {
                if (std::abs(i) < minDistance) {
                    minDistance = std::abs(i);
                    bestIdx = idx;
                }
            }
        }
    }
    return (float)bestIdx / (float)numSamples;
}

void Otodesk_samplerAudioProcessorEditor::vblankCallback()
{
    blinkCounter = (blinkCounter + 1) % 60;
    bool showCursor = blinkCounter < 30;

    {
        juce::ScopedLock sl(audioProcessor.stateLock);
        statusLabelFile.setText("> " + audioProcessor.hudLineFile, juce::dontSendNotification);
        juce::String taskTxt = audioProcessor.hudLineTask;
        if (audioProcessor.ingestState.load() != IngestState::Ready && audioProcessor.ingestState.load() != IngestState::Empty) {
            taskTxt += (showCursor ? "_" : " ");
        }
        statusLabelTask.setText("> " + taskTxt, juce::dontSendNotification);
        statusLabelOrig.setText("> " + audioProcessor.hudLineOrig, juce::dontSendNotification);
    }
    currentProgress = audioProcessor.ingestProgress.load();

    int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load()) + 1;
    juce::String s = juce::String(activeSlot);
    for (int i = 0; i < 3; ++i) {
        juce::String lIdx = juce::String(i + 1) + "_" + s;
        bool isSync = audioProcessor.apvts.getRawParameterValue("lfo" + lIdx + "Sync")->load() > 0.5f;
        lfoUIs[i].freqFreeSlider.setVisible(!isSync);
        lfoUIs[i].freqSyncCombo.setVisible(isSync);
    }
    repaint(waveArea);
    repaint(terminalArea);
    repaint(slotsArea);
}

void Otodesk_samplerAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster*) { repaint(); }

void Otodesk_samplerAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
    int playMode = (int)std::round(audioProcessor.apvts.getRawParameterValue("playMode_" + juce::String(activeSlot + 1))->load());

    if (waveArea.contains(e.getPosition())) {
        float relX = (float)(e.getPosition().x - waveArea.getX()) / (float)waveArea.getWidth();
        float relY = (float)(e.getPosition().y - waveArea.getY()) / (float)waveArea.getHeight();

        float sStart = audioProcessor.apvts.getRawParameterValue("sampleStart_" + juce::String(activeSlot + 1))->load();
        float sEnd = audioProcessor.apvts.getRawParameterValue("sampleEnd_" + juce::String(activeSlot + 1))->load();
        float lStart = audioProcessor.apvts.getRawParameterValue("loopStart_" + juce::String(activeSlot + 1))->load();
        float lLen = audioProcessor.apvts.getRawParameterValue("loopLength_" + juce::String(activeSlot + 1))->load();
        float lEnd = std::min(1.0f, lStart + lLen);

        if (relY < 0.5f) {
            if (std::abs(relX - sStart) < 0.03f) dragMode = DragMode::SampleStart;
            else if (std::abs(relX - sEnd) < 0.03f) dragMode = DragMode::SampleEnd;
            else dragMode = DragMode::None;
        }
        else {
            if (playMode == 0) {
                if (std::abs(relX - lStart) < 0.03f) dragMode = DragMode::LoopStart;
                else if (std::abs(relX - lEnd) < 0.03f) dragMode = DragMode::LoopEnd;
                else dragMode = DragMode::None;
            }
            else {
                dragMode = DragMode::None;
            }
        }
    }
    else if (slotsArea.contains(e.getPosition())) {
        int numSlots = 8; float padding = 4.0f;
        float boxW = ((float)slotsArea.getWidth() - padding * (numSlots - 1)) / (float)numSlots;
        for (int i = 0; i < numSlots; ++i) {
            float x = slotsArea.getX() + i * (boxW + padding);
            juce::Rectangle<float> rect(x, (float)slotsArea.getY(), boxW, (float)slotsArea.getHeight());

            if (rect.contains(e.getPosition().toFloat())) {
                if (e.mods.isRightButtonDown()) {
                    juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::QuestionIcon)
                        .withTitle("Clear Slot")
                        .withMessage("Are you sure you want to completely clear Slot " + juce::String(i + 1) + "?")
                        .withButton("Yes")
                        .withButton("No"),
                        [this, i](int result) {
                            if (result == 1) audioProcessor.clearSlot(i);
                        });
                    return;
                }

                if (auto* param = audioProcessor.apvts.getParameter("activeSlot")) {
                    param->setValueNotifyingHost(i / 7.0f);
                    updateSlotAttachments();
                    updateThemeColor(i);
                }
                audioProcessor.triggerPreview.store(true);
                repaint(slotsArea); repaint(waveArea);
                break;
            }
        }
    }
}

void Otodesk_samplerAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragMode != DragMode::None) {
        float ratio = (float)(e.getPosition().x - waveArea.getX()) / (float)waveArea.getWidth();
        ratio = juce::jlimit(0.0f, 1.0f, ratio);

        int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
        float snappedRatio = getZeroCrossedPosition(ratio, activeSlot);
        juce::String s = juce::String(activeSlot + 1);

        if (dragMode == DragMode::SampleStart) {
            float sEnd = audioProcessor.apvts.getRawParameterValue("sampleEnd_" + s)->load();
            float newStart = juce::jlimit(0.0f, sEnd - 0.01f, snappedRatio);
            sampleStartSlider.setValue(newStart, juce::sendNotificationSync);
        }
        else if (dragMode == DragMode::SampleEnd) {
            float sStart = audioProcessor.apvts.getRawParameterValue("sampleStart_" + s)->load();
            float newEnd = juce::jlimit(sStart + 0.01f, 1.0f, snappedRatio);
            sampleEndSlider.setValue(newEnd, juce::sendNotificationSync);
        }
        else if (dragMode == DragMode::LoopStart) {
            float currentLen = audioProcessor.apvts.getRawParameterValue("loopLength_" + s)->load();
            float newStart = juce::jlimit(0.0f, 1.0f - currentLen, snappedRatio);
            loopStartSlider.setValue(newStart, juce::sendNotificationSync);
        }
        else if (dragMode == DragMode::LoopEnd) {
            float currentStart = audioProcessor.apvts.getRawParameterValue("loopStart_" + s)->load();
            float newLen = juce::jlimit(0.0f, 1.0f - currentStart, snappedRatio - currentStart);
            loopLengthSlider.setValue(newLen, juce::sendNotificationSync);
        }
    }
}

void Otodesk_samplerAudioProcessorEditor::mouseUp(const juce::MouseEvent&) { dragMode = DragMode::None; }

void Otodesk_samplerAudioProcessorEditor::drawWaveform(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(VividPastelTheme::track);
    g.fillRoundedRectangle(area.toFloat(), 6.0f);

    int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
    if (!audioProcessor.waveformReady[activeSlot].load() || audioProcessor.validSlots[activeSlot] == 0) return;

    const auto& peaks = audioProcessor.waveformPeaks[activeSlot];
    float midY = (float)area.getCentreY();
    float height = (float)area.getHeight() * 0.8f;

    juce::Colour slotColor = VividPastelTheme::slotColors[activeSlot];
    juce::ColourGradient waveGrad(slotColor, area.getTopLeft().toFloat(),
        slotColor.withMultipliedBrightness(0.6f), area.getBottomRight().toFloat(), false);
    g.setGradientFill(waveGrad);

    juce::Path wavePath;
    wavePath.startNewSubPath((float)area.getX(), midY);
    for (size_t i = 0; i < (size_t)area.getWidth(); ++i) {
        size_t peakIdx = (size_t)juce::jlimit(0, 999, (int)((i * 1000) / (size_t)area.getWidth()));
        float val = peaks[peakIdx] * height;
        wavePath.lineTo((float)(area.getX() + (int)i), midY - val * 0.5f);
    }
    for (int i = area.getWidth() - 1; i >= 0; --i) {
        size_t peakIdx = (size_t)juce::jlimit(0, 999, (int)((i * 1000) / (size_t)area.getWidth()));
        float val = peaks[peakIdx] * height;
        wavePath.lineTo((float)(area.getX() + i), midY + val * 0.5f);
    }
    wavePath.closeSubPath();
    g.fillPath(wavePath);

    juce::String s = juce::String(activeSlot + 1);
    float sStart = audioProcessor.apvts.getRawParameterValue("sampleStart_" + s)->load();
    float sEnd = audioProcessor.apvts.getRawParameterValue("sampleEnd_" + s)->load();
    int playMode = (int)std::round(audioProcessor.apvts.getRawParameterValue("playMode_" + s)->load());

    float sStartX = area.getX() + sStart * area.getWidth();
    float sEndX = area.getX() + sEnd * area.getWidth();

    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRect(area.getX(), area.getY(), (int)(sStartX - area.getX()), area.getHeight());
    g.fillRect((int)sEndX, area.getY(), area.getRight() - (int)sEndX, area.getHeight());

    g.setColour(juce::Colours::white);
    g.drawVerticalLine((int)sStartX, (float)area.getY(), (float)area.getBottom());
    g.drawVerticalLine((int)sEndX, (float)area.getY(), (float)area.getBottom());

    juce::Path triStart, triEnd;
    triStart.addTriangle(sStartX - 6, area.getY(), sStartX + 6, area.getY(), sStartX, area.getY() + 10);
    triEnd.addTriangle(sEndX - 6, area.getY(), sEndX + 6, area.getY(), sEndX, area.getY() + 10);
    g.fillPath(triStart); g.fillPath(triEnd);

    if (playMode == 0) {
        float lStart = audioProcessor.apvts.getRawParameterValue("loopStart_" + s)->load();
        float lLen = audioProcessor.apvts.getRawParameterValue("loopLength_" + s)->load();
        float xFade = audioProcessor.apvts.getRawParameterValue("crossfade_" + s)->load();
        float lEnd = std::min(1.0f, lStart + lLen);

        float lStartX = area.getX() + lStart * area.getWidth();
        float lEndX = area.getX() + lEnd * area.getWidth();
        float fadeW = xFade * (lEndX - lStartX);

        g.setColour(slotColor.withAlpha(0.2f));
        g.fillRect(lStartX, (float)area.getY(), lEndX - lStartX, (float)area.getHeight());

        if (fadeW > 1.0f) {
            float px1 = lEndX;
            float px0 = lEndX - fadeW;

            juce::Path pOut, pInEnd, pInStart;
            pOut.addTriangle(px0, midY, px1, (float)area.getBottom(), px1, (float)area.getY());
            pInEnd.addTriangle(px0, (float)area.getBottom(), px0, (float)area.getY(), px1, midY);
            pInStart.addTriangle(lStartX, (float)area.getBottom(), lStartX, (float)area.getY(), lStartX + fadeW, midY);

            g.setColour(juce::Colour(0xFFFF4081).withAlpha(0.4f));
            g.fillPath(pOut);
            g.setColour(juce::Colour(0xFF00E676).withAlpha(0.4f));
            g.fillPath(pInEnd);
            g.setColour(juce::Colour(0xFF00E676).withAlpha(0.25f));
            g.fillPath(pInStart);
        }

        g.setColour(slotColor.brighter(0.5f));
        g.drawVerticalLine((int)lStartX, midY, (float)area.getBottom());
        g.drawVerticalLine((int)lEndX, midY, (float)area.getBottom());

        juce::Path loopTri1, loopTri2;
        loopTri1.addTriangle(lStartX - 6, midY, lStartX + 6, midY, lStartX, midY + 10);
        loopTri2.addTriangle(lEndX - 6, midY, lEndX + 6, midY, lEndX, midY + 10);
        g.strokePath(loopTri1, juce::PathStrokeType(1.5f));
        g.strokePath(loopTri2, juce::PathStrokeType(1.5f));
    }
}

void Otodesk_samplerAudioProcessorEditor::drawTerminalHUD(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xFF060606));
    g.fillRoundedRectangle(terminalArea.toFloat(), 4.0f);
    int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
    juce::Colour slotColor = VividPastelTheme::slotColors[activeSlot];
    g.setColour(slotColor.withAlpha(0.3f));
    g.drawRoundedRectangle(terminalArea.toFloat(), 4.0f, 1.0f);

    if (audioProcessor.ingestState.load() == IngestState::Ready || audioProcessor.ingestState.load() == IngestState::Empty) return;

    g.setColour(juce::Colour(0xFF0F261B));
    g.fillRoundedRectangle(progressBarRect.toFloat(), 2.0f);
    g.setColour(slotColor);
    auto filledRect = progressBarRect.withWidth((int)((float)progressBarRect.getWidth() * currentProgress));
    g.fillRoundedRectangle(filledRect.toFloat(), 2.0f);
    g.setColour(juce::Colours::black);
    g.drawText(juce::String(juce::roundToInt(currentProgress * 100.0f)) + "%", progressBarRect, juce::Justification::centred);
}

void Otodesk_samplerAudioProcessorEditor::drawSlotsIndicator(juce::Graphics& g)
{
    if (slotsArea.isEmpty()) return;
    int numSlots = 8; float padding = 4.0f;
    float boxW = ((float)slotsArea.getWidth() - padding * (numSlots - 1)) / (float)numSlots;
    float boxH = slotsArea.getHeight();
    int activeSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());

    for (int i = 0; i < numSlots; ++i) {
        float x = slotsArea.getX() + i * (boxW + padding);
        juce::Rectangle<float> rect(x, slotsArea.getY(), boxW, boxH);
        bool isLoaded = audioProcessor.validSlots[i] > 0;
        bool isActive = (i == activeSlot);
        juce::Colour slotColor = isActive ? juce::Colours::white : (isLoaded ? VividPastelTheme::slotColors[i].withMultipliedBrightness(0.6f) : VividPastelTheme::inactiveSlot);
        g.setColour(slotColor);
        g.fillRoundedRectangle(rect, 3.0f);
        g.setColour(isActive ? juce::Colours::black : juce::Colours::black.withAlpha(0.5f));
        g.drawText(juce::String(i + 1), rect, juce::Justification::centred, false);
    }
}

void Otodesk_samplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(VividPastelTheme::bg);
    drawTerminalHUD(g); drawSlotsIndicator(g);
    if (!waveArea.isEmpty()) drawWaveform(g, waveArea);

    if (!modMatrixArea.isEmpty()) {
        g.setColour(VividPastelTheme::panel);
        g.fillRoundedRectangle(modMatrixArea.toFloat(), 6.0f);
    }
    if (!fxRackArea.isEmpty()) {
        g.setColour(VividPastelTheme::panel);
        g.fillRoundedRectangle(fxRackArea.toFloat(), 6.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(fxRackArea.toFloat(), 6.0f, 1.0f);
    }
}

void Otodesk_samplerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20, 15);

    terminalArea = area.removeFromTop(90);
    auto termInner = terminalArea.reduced(10, 5);
    statusLabelFile.setBounds(termInner.removeFromTop(20));
    statusLabelTask.setBounds(termInner.removeFromTop(20));
    progressBarRect = termInner.removeFromTop(10).reduced(5, 2);
    statusLabelOrig.setBounds(termInner.removeFromTop(20));

    area.removeFromTop(10);
    slotsArea = area.removeFromTop(16).reduced(20, 0);

    area.removeFromTop(10);
    waveArea = area.removeFromTop(150);
    area.removeFromTop(10);

    auto toolBar1 = area.removeFromTop(35).withTrimmedLeft(5);
    randomSlotButton.setBounds(toolBar1.removeFromLeft(100)); toolBar1.removeFromLeft(10);
    materialLabel.setBounds(toolBar1.removeFromLeft(55)); materialCombo.setBounds(toolBar1.removeFromLeft(90)); toolBar1.removeFromLeft(15);
    rootKeyLabel.setBounds(toolBar1.removeFromLeft(60)); rootKeyCombo.setBounds(toolBar1.removeFromLeft(70)); toolBar1.removeFromLeft(15);
    reanalyzeButton.setBounds(toolBar1.removeFromLeft(90).reduced(0, 4)); toolBar1.removeFromLeft(15);
    playModeLabel.setBounds(toolBar1.removeFromLeft(45)); playModeCombo.setBounds(toolBar1.removeFromLeft(90)); toolBar1.removeFromLeft(15);

    slotGainLabel.setBounds(toolBar1.removeFromLeft(65));
    slotGainSlider.setBounds(toolBar1.removeFromLeft(80));

    area.removeFromTop(15);

    int colW = area.getWidth() / 3;

    auto row0 = area.removeFromTop(35);
    auto cell0_0 = row0.removeFromLeft(colW).reduced(5, 2); attackLabel.setBounds(cell0_0.removeFromLeft(70)); attackSlider.setBounds(cell0_0);
    auto cell0_1 = row0.removeFromLeft(colW).reduced(5, 2); loopStartLabel.setBounds(cell0_1.removeFromLeft(70)); loopStartSlider.setBounds(cell0_1);
    auto cell0_2 = row0.removeFromLeft(colW).reduced(5, 2); octLabel.setBounds(cell0_2.removeFromLeft(70)); octSlider.setBounds(cell0_2);

    auto row1 = area.removeFromTop(35);
    auto cell1_0 = row1.removeFromLeft(colW).reduced(5, 2); releaseLabel.setBounds(cell1_0.removeFromLeft(70)); releaseSlider.setBounds(cell1_0);
    auto cell1_1 = row1.removeFromLeft(colW).reduced(5, 2); loopLengthLabel.setBounds(cell1_1.removeFromLeft(70)); loopLengthSlider.setBounds(cell1_1);
    auto cell1_2 = row1.removeFromLeft(colW).reduced(5, 2); semiLabel.setBounds(cell1_2.removeFromLeft(70)); semiSlider.setBounds(cell1_2);

    auto row2 = area.removeFromTop(35);
    auto cell2_0 = row2.removeFromLeft(colW).reduced(5, 2); sampleStartLabel.setBounds(cell2_0.removeFromLeft(70)); sampleStartSlider.setBounds(cell2_0);
    auto cell2_1 = row2.removeFromLeft(colW).reduced(5, 2); crossfadeLabel.setBounds(cell2_1.removeFromLeft(70)); crossfadeSlider.setBounds(cell2_1);
    auto cell2_2 = row2.removeFromLeft(colW).reduced(5, 2); fineLabel.setBounds(cell2_2.removeFromLeft(70)); fineSlider.setBounds(cell2_2);

    auto row3 = area.removeFromTop(35);
    auto cell3_0 = row3.removeFromLeft(colW).reduced(5, 2); sampleEndLabel.setBounds(cell3_0.removeFromLeft(70)); sampleEndSlider.setBounds(cell3_0);
    auto cell3_1 = row3.removeFromLeft(colW).reduced(5, 2); panLabel.setBounds(cell3_1.removeFromLeft(60)); panSlider.setBounds(cell3_1);
    auto cell3_2 = row3.removeFromLeft(colW).reduced(5, 2); gainLabel.setBounds(cell3_2.removeFromLeft(75)); gainSlider.setBounds(cell3_2);

    area.removeFromTop(10);
    modMatrixArea = area.removeFromTop(120);

    for (int i = 0; i < 3; ++i) {
        auto lfoRow = modMatrixArea.removeFromTop(40).reduced(10, 8); auto& ui = lfoUIs[i];
        ui.enableBtn.setBounds(lfoRow.removeFromLeft(60)); lfoRow.removeFromLeft(15);
        ui.targetCombo.setBounds(lfoRow.removeFromLeft(100)); lfoRow.removeFromLeft(10);
        ui.waveCombo.setBounds(lfoRow.removeFromLeft(100)); lfoRow.removeFromLeft(15);
        ui.syncBtn.setBounds(lfoRow.removeFromLeft(60)); lfoRow.removeFromLeft(10);
        auto freqArea = lfoRow.removeFromLeft(80); ui.freqFreeSlider.setBounds(freqArea); ui.freqSyncCombo.setBounds(freqArea); lfoRow.removeFromLeft(15);
        ui.minLabel.setBounds(lfoRow.removeFromLeft(30)); ui.minSlider.setBounds(lfoRow.removeFromLeft(80)); lfoRow.removeFromLeft(10);
        ui.maxLabel.setBounds(lfoRow.removeFromLeft(30)); ui.maxSlider.setBounds(lfoRow.removeFromLeft(80));
    }

    area.removeFromTop(10);
    fxRackArea = area;
    auto innerFxArea = fxRackArea.reduced(5); int slotWidth = innerFxArea.getWidth() / 5;
    for (int i = 0; i < 4; ++i) {
        auto slotArea = innerFxArea.removeFromLeft(slotWidth).reduced(8, 0);
        fxSlots[i].typeCombo.setBounds(slotArea.removeFromTop(24)); slotArea.removeFromTop(8);
        fxSlots[i].amountKnob.setBounds(slotArea);
    }
    auto aegisArea = innerFxArea.reduced(8, 0); aegisLabel.setBounds(aegisArea.removeFromTop(24)); aegisArea.removeFromTop(8); aegisKnob.setBounds(aegisArea);
}

bool Otodesk_samplerAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files) if (f.endsWith(".wav") || f.endsWith(".aif") || f.endsWith(".mp3")) return true;
    return false;
}

void Otodesk_samplerAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    for (const auto& f : files) {
        if (f.endsWith(".wav") || f.endsWith(".aif") || f.endsWith(".mp3")) {
            int currentSlot = (int)std::round(audioProcessor.apvts.getRawParameterValue("activeSlot")->load());
            audioProcessor.loadFileForIngest(juce::File(f), currentSlot);
            break;
        }
    }
}