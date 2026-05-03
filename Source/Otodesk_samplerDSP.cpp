// ==========================================
// File: Otodesk_samplerDSP.cpp
// ==========================================
#include "Otodesk_samplerDSP.h"

OtodeskVoice::OtodeskVoice() {}

void OtodeskVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    adsr.setSampleRate(sampleRate);
    voiceInternalBuffer.setSize(2, samplesPerBlock, false, true, true);
    random.setSeed((uint32_t)(reinterpret_cast<uintptr_t>(this) & 0xFFFFFFFF));
}

void OtodeskVoice::releaseResources() { voiceInternalBuffer.setSize(0, 0); }

void OtodeskVoice::startNote(int midiNote, float velocity, const OtodeskParams& p, const std::array<int, 8>& validSlots,
    const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* anchors,
    const std::array<std::array<std::atomic<bool>, 42>, 8>* anchorReady)
{
    currentMidiNote = midiNote;
    playing = true;
    isReleasing = false;
    currentVelocity = velocity;
    playheadPhase = 0.0;
    cachedLoopStart = -1.0f;
    cachedLoopEnd = -1.0f;

    currentSlot = 0;

    if (p.randomSlotEnable) {
        int activeSlotCount = 0;
        int availableSlots[8] = { 0 };
        for (int i = 0; i < 8; ++i) {
            if (validSlots[i] > 0) availableSlots[activeSlotCount++] = i;
        }
        if (activeSlotCount > 0) {
            currentSlot = availableSlots[random.nextInt(activeSlotCount)];
        }
        else {
            playing = false;
            return;
        }
    }
    else {
        int targetSlot = p.activeSlotOverride;
        if (targetSlot >= 0 && targetSlot < 8 && validSlots[targetSlot] > 0) {
            currentSlot = targetSlot;
        }
        else {
            playing = false;
            return;
        }
    }

    isOneShot = (p.playMode[currentSlot] == 1);
    int targetAnchorIndex = juce::jlimit(0, 41, (int)std::round(midiNote / 3.0));

    int bestAnchor = targetAnchorIndex;
    int minDistance = 999;
    if (anchors != nullptr && anchorReady != nullptr) {
        for (int i = 0; i < 42; ++i) {
            if ((*anchorReady)[currentSlot][i].load() == true && (*anchors)[currentSlot][i].getNumSamples() > 0) {
                int dist = std::abs(i - targetAnchorIndex);
                if (dist < minDistance) {
                    minDistance = dist;
                    bestAnchor = i;
                }
            }
        }
    }

    currentAnchorIndex = bestAnchor;
    int anchorMidi = currentAnchorIndex * 3;

    float diffSt = (float)(midiNote - anchorMidi)
        + (float)(p.octave[currentSlot] * 12) + (p.pitchOctMod[currentSlot] * 12.0f)
        + (float)(p.pitchSt[currentSlot]) + p.pitchStMod[currentSlot]
        + (p.fineTune[currentSlot] / 100.0f) + (p.pitchFineMod[currentSlot] / 100.0f);

    currentPitchRatio = std::pow(2.0f, diffSt / 12.0f);

    juce::ADSR::Parameters adsrParams;
    adsrParams.attack = p.attack[currentSlot];
    adsrParams.decay = 0.1f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = p.release[currentSlot];
    adsr.setParameters(adsrParams);

    adsr.noteOn();
}

void OtodeskVoice::stopNote(bool allowTailOff)
{
    if (isOneShot) return;
    if (allowTailOff) adsr.noteOff();
    else clearCurrentNote();
}

bool OtodeskVoice::isActive() const { return playing; }
void OtodeskVoice::clearCurrentNote() { playing = false; currentMidiNote = -1; adsr.reset(); }

void OtodeskVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
    const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* anchors,
    const OtodeskParams& p)
{
    if (!adsr.isActive() || anchors == nullptr) {
        if (playing) clearCurrentNote();
        return;
    }

    const auto& srcBuffer = (*anchors)[currentSlot][currentAnchorIndex];
    int srcSamples = srcBuffer.getNumSamples();
    if (srcSamples <= 10) return;

    int numSamples = outputBuffer.getNumSamples();
    voiceInternalBuffer.clear();

    const float* srcL = srcBuffer.getReadPointer(0);
    const float* srcR = srcBuffer.getNumChannels() > 1 ? srcBuffer.getReadPointer(1) : srcL;

    float totalSamps = (float)srcSamples;

    float sStartRaw = p.sampleStart[currentSlot] * totalSamps;
    float sEndRaw = p.sampleEnd[currentSlot] * totalSamps;
    if (sEndRaw > totalSamps - 2.0f) sEndRaw = totalSamps - 2.0f;
    if (sStartRaw >= sEndRaw) sStartRaw = sEndRaw - 1.0f;

    float lStartRaw = p.loopStart[currentSlot] * totalSamps;
    float lLengthRaw = p.loopLength[currentSlot] * totalSamps;
    float lEndRaw = lStartRaw + lLengthRaw;
    if (lEndRaw > sEndRaw) lEndRaw = sEndRaw;
    if (lStartRaw >= lEndRaw) lStartRaw = lEndRaw - 1.0f;

    float activeStart = isOneShot ? sStartRaw : lStartRaw;
    float activeEnd = isOneShot ? sEndRaw : lEndRaw;

    if (playheadPhase == 0.0) {
        int searchMax = 512;
        int startIdx = (int)(isOneShot ? sStartRaw : sStartRaw);
        for (int i = 0; i < searchMax && (startIdx + i + 1) < srcSamples; ++i) {
            if (srcL[startIdx + i] <= 0.0f && srcL[startIdx + i + 1] > 0.0f) {
                playheadPhase = (double)(startIdx + i);
                break;
            }
        }
        if (playheadPhase == 0.0) playheadPhase = activeStart;
    }

    float xFade = isOneShot ? 0.0f : p.crossfadeSize[currentSlot] * (activeEnd - activeStart);

    auto getInterpolated = [&](float pos, const float* data, int maxIdx) -> float {
        int idx1 = (int)pos;
        float frac = pos - (float)idx1;
        int idx0 = juce::jmax(0, idx1 - 1);
        int idx2 = juce::jmin(maxIdx, idx1 + 1);
        int idx3 = juce::jmin(maxIdx, idx1 + 2);
        return lagrange3rdInterpolate(frac, data[idx0], data[idx1], data[idx2], data[idx3]);
        };

    float panBase = 0.5f + (p.pan[currentSlot] * 0.5f);
    float pan = juce::jlimit(0.0f, 1.0f, panBase);
    float panL = std::sqrt(1.0f - pan) * currentVelocity;
    float panR = std::sqrt(pan) * currentVelocity;
    float slotGainLin = juce::Decibels::decibelsToGain(p.slotGain[currentSlot]);

    float* writeL = voiceInternalBuffer.getWritePointer(0);
    float* writeR = voiceInternalBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        if (isOneShot) {
            float totalPlaySamples = (activeEnd - activeStart) / currentPitchRatio;
            float releaseSamples = p.release[currentSlot] * currentSampleRate;

            if (releaseSamples > totalPlaySamples * 0.5f) {
                releaseSamples = totalPlaySamples * 0.5f;
            }

            float remainSamples = (activeEnd - (float)playheadPhase) / currentPitchRatio;

            if (remainSamples <= releaseSamples && !isReleasing) {
                adsr.noteOff();
                isReleasing = true;
            }
        }

        playheadPhase += currentPitchRatio;

        if (playheadPhase >= activeEnd) {
            if (isOneShot) {
                clearCurrentNote();
                break;
            }
            else {
                playheadPhase -= (activeEnd - activeStart);
                if (playheadPhase < activeStart) playheadPhase = activeStart;
            }
        }

        float pos = (float)playheadPhase;
        int maxIdx = srcSamples - 1;

        float outL = getInterpolated(pos, srcL, maxIdx);
        float outR = getInterpolated(pos, srcR, maxIdx);

        if (!isOneShot && pos > activeEnd - xFade && xFade > 0.1f) {
            float linearPhase = (pos - (activeEnd - xFade)) / xFade;
            float crossPos = pos - activeEnd + activeStart;

            if (crossPos < 0.0f) crossPos += (activeEnd - activeStart);
            if (crossPos >= srcSamples) crossPos = (float)(srcSamples - 1);

            float crossL = getInterpolated(crossPos, srcL, maxIdx);
            float crossR = getInterpolated(crossPos, srcR, maxIdx);

            float gainMain = std::sqrt(1.0f - linearPhase);
            float gainCross = std::sqrt(linearPhase);

            outL = outL * gainMain + crossL * gainCross;
            outR = outR * gainMain + crossR * gainCross;
        }

        writeL[i] = outL * panL * slotGainLin;
        writeR[i] = outR * panR * slotGainLin;
    }

    adsr.applyEnvelopeToBuffer(voiceInternalBuffer, 0, numSamples);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
        juce::FloatVectorOperations::add(outputBuffer.getWritePointer(ch),
            voiceInternalBuffer.getReadPointer(ch % 2),
            numSamples);
    }
}

OtodeskSynth::OtodeskSynth() {}

void OtodeskSynth::prepareToPlay(double sr, int spb) {
    currentSampleRate = sr;
    for (int i = 0; i < 8; ++i) {
        lfo1Gen[i].setSampleRate(sr);
        lfo2Gen[i].setSampleRate(sr);
        lfo3Gen[i].setSampleRate(sr);
    }
    for (int ch = 0; ch < 2; ++ch) { filterHPF[ch].reset(); filterLPF[ch].reset(); }

    for (int i = 0; i < 4; ++i) {
        mutators[i].prepareToPlay(sr);
        phantoms[i].prepareToPlay(sr);
        warps[i].prepareToPlay(sr);
        abysses[i].prepareToPlay(sr);
    }
    for (auto& v : voices) v.prepareToPlay(sr, spb);
}

void OtodeskSynth::releaseResources() { for (auto& v : voices) v.releaseResources(); }

void OtodeskSynth::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi,
    const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* sharedAnchorBuffers,
    const std::array<int, 8>& validSlots,
    const OtodeskParams& p, double bpm, OtodeskParams& outModulatedParams,
    const std::array<std::array<std::atomic<bool>, 42>, 8>* anchorReady)
{
    OtodeskParams modP = p;

    float globalFx1Mod = 0.0f;
    float globalFx2Mod = 0.0f;
    float globalFx3Mod = 0.0f;
    float globalFx4Mod = 0.0f;

    for (int s = 0; s < 8; ++s) {
        auto applyLFO = [&](const LFOParams& lfoP, LFO& lfoGen, int currentSlotIndex) {
            float lfoFreq = lfoP.freqFree;
            if (lfoP.sync && bpm > 0.0) {
                const float multipliers[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
                int idx = juce::jlimit(0, 5, lfoP.freqSync);
                lfoFreq = (float)(bpm / 60.0) * multipliers[idx];
            }
            lfoGen.advanceBlock(lfoFreq, lfoP.waveform, buf.getNumSamples());
            float lfoVal = lfoGen.getValue(lfoP.waveform);

            if (lfoP.enable && lfoP.target > 0) {
                float minNorm = lfoP.minVal / 100.0f;
                float maxNorm = lfoP.maxVal / 100.0f;
                float modAmount = minNorm + (maxNorm - minNorm) * lfoVal;

                switch (lfoP.target) {
                case 1: modP.loopStart[currentSlotIndex] = modAmount; break;
                case 2: modP.loopLength[currentSlotIndex] = modAmount; break;
                case 3: modP.crossfadeSize[currentSlotIndex] = modAmount; break;
                case 4: modP.pan[currentSlotIndex] = (modAmount * 2.0f) - 1.0f; break;
                case 5: modP.sampleStart[currentSlotIndex] = modAmount; break;
                case 6: modP.pitchOctMod[currentSlotIndex] = std::round((modAmount * 4.0f) - 2.0f); break;
                case 7: modP.pitchStMod[currentSlotIndex] = std::round((modAmount * 48.0f) - 24.0f); break;
                case 8: modP.pitchFineMod[currentSlotIndex] = (modAmount * 200.0f) - 100.0f; break;
                case 9:  globalFx1Mod += (modAmount * 2.0f) - 1.0f; break;
                case 10: globalFx2Mod += (modAmount * 2.0f) - 1.0f; break;
                case 11: globalFx3Mod += (modAmount * 2.0f) - 1.0f; break;
                case 12: globalFx4Mod += (modAmount * 2.0f) - 1.0f; break;
                }
            }
            };

        applyLFO(p.lfo1[s], lfo1Gen[s], s);
        applyLFO(p.lfo2[s], lfo2Gen[s], s);
        applyLFO(p.lfo3[s], lfo3Gen[s], s);
    }

    modP.fx1Amount = juce::jlimit(-1.0f, 1.0f, modP.fx1Amount + globalFx1Mod);
    modP.fx2Amount = juce::jlimit(-1.0f, 1.0f, modP.fx2Amount + globalFx2Mod);
    modP.fx3Amount = juce::jlimit(-1.0f, 1.0f, modP.fx3Amount + globalFx3Mod);
    modP.fx4Amount = juce::jlimit(-1.0f, 1.0f, modP.fx4Amount + globalFx4Mod);

    outModulatedParams = modP;

    for (const auto& m : midi) {
        auto msg = m.getMessage();
        if (msg.isNoteOn()) {
            bool voiceAllocated = false;

            // 1. まず空いている（発音していない）ボイスを探す
            for (auto& v : voices) {
                if (!v.isActive()) {
                    v.startNote(msg.getNoteNumber(), msg.getFloatVelocity(), modP, validSlots, sharedAnchorBuffers, anchorReady);
                    voiceAllocated = true;
                    break;
                }
            }

            // 2. 【究極の追加】ボイス・スティーリング（16音全て埋まっている場合の強制割り当て）
            if (!voiceAllocated) {
                // ラウンドロビン（順番）で古い音を強制的に奪う
                static int stealIndex = 0;
                voices[stealIndex].clearCurrentNote();
                voices[stealIndex].startNote(msg.getNoteNumber(), msg.getFloatVelocity(), modP, validSlots, sharedAnchorBuffers, anchorReady);

                stealIndex = (stealIndex + 1) % NUM_VOICES;
            }
        }
        else if (msg.isNoteOff()) {
            int n = msg.getNoteNumber();
            for (auto& v : voices) {
                if (v.isActive() && v.getCurrentlyPlayingNote() == n) {
                    v.stopNote(true);
                }
            }
        }
    }

    for (auto& v : voices) if (v.isActive()) v.renderNextBlock(buf, sharedAnchorBuffers, modP);

    for (int ch = 0; ch < 2; ++ch) {
        filterHPF[ch].setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, modP.masterHPF));
        filterLPF[ch].setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, modP.masterLPF));
    }

    for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
        float* channelData = buf.getWritePointer(ch);
        filterHPF[ch].processSamples(channelData, buf.getNumSamples());
        filterLPF[ch].processSamples(channelData, buf.getNumSamples());
    }

    buf.applyGain(juce::Decibels::decibelsToGain(modP.outGainDb));

    int channels = buf.getNumChannels();
    for (int i = 0; i < buf.getNumSamples(); ++i) {
        float sampleL = buf.getSample(0, i);
        float sampleR = channels > 1 ? buf.getSample(1, i) : sampleL;

        auto applyFXSlot = [&](int slotIndex, int type, float amount) {
            switch (type) {
            case 1: mutators[slotIndex].process(sampleL, sampleR, amount); break;
            case 2: phantoms[slotIndex].process(sampleL, sampleR, std::abs(amount), bpm); break;
            case 3: warps[slotIndex].process(sampleL, sampleR, std::abs(amount)); break;
            case 4: abysses[slotIndex].process(sampleL, sampleR, std::abs(amount)); break;
            }
            };

        applyFXSlot(0, modP.fx1Type, modP.fx1Amount);
        applyFXSlot(1, modP.fx2Type, modP.fx2Amount);
        applyFXSlot(2, modP.fx3Type, modP.fx3Amount);
        applyFXSlot(3, modP.fx4Type, modP.fx4Amount);

        aegis.process(sampleL, sampleR, modP.fxAegisAmount);

        buf.setSample(0, i, sampleL);
        if (channels > 1) buf.setSample(1, i, sampleR);
    }
}