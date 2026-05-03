// ==========================================
// File: Otodesk_samplerDSP.h
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <random>
#include <atomic>

// ==========================================
// CPU Optimization: Fast Xorshift PRNG
// ==========================================
class FastRandom {
    uint32_t state = 123456789;
public:
    inline void setSeed(uint32_t seed) { state = seed ? seed : 123456789; }
    inline uint32_t nextInt() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    inline float nextFloat() {
        return (float)(nextInt() & 0x00FFFFFF) / (float)0x01000000;
    }
    inline int nextInt(int maxVal) {
        if (maxVal <= 0) return 0;
        return (int)(nextInt() % (uint32_t)maxVal);
    }
};

struct LFOParams {
    bool enable = false;
    int target = 0;
    int waveform = 0;
    bool sync = false;
    float freqFree = 1.0f;
    int freqSync = 2;
    float minVal = 0.0f;
    float maxVal = 100.0f;
};

// ==========================================
// Otodesk Sampler Parameters
// ==========================================
struct OtodeskParams {
    std::array<float, 8> attack = { 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f };
    std::array<float, 8> release = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    std::array<float, 8> sampleStart = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> sampleEnd = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 8> loopStart = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> loopLength = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 8> crossfadeSize = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
    std::array<int, 8> playMode = { 0, 0, 0, 0, 0, 0, 0, 0 };

    std::array<float, 8> pan = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> slotGain = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    std::array<int, 8> octave = { 0, 0, 0, 0, 0, 0, 0, 0 };
    std::array<int, 8> pitchSt = { 0, 0, 0, 0, 0, 0, 0, 0 };
    std::array<float, 8> fineTune = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    std::array<LFOParams, 8> lfo1;
    std::array<LFOParams, 8> lfo2;
    std::array<LFOParams, 8> lfo3;

    std::array<float, 8> pitchOctMod = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> pitchStMod = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> pitchFineMod = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    int activeSlotOverride = 0;
    bool randomSlotEnable = false;
    float outGainDb = 0.0f;

    float masterHPF = 20.0f;
    float masterLPF = 20000.0f;

    int fx1Type = 0; float fx1Amount = 0.0f;
    int fx2Type = 0; float fx2Amount = 0.0f;
    int fx3Type = 0; float fx3Amount = 0.0f;
    int fx4Type = 0; float fx4Amount = 0.0f;
    float fxAegisAmount = 0.0f;
};

// ==========================================
// DSP Utilities
// ==========================================
inline float antiDenormal(float x) {
    return (std::abs(x) < 1.0e-20f) ? 0.0f : x;
}

inline float safeLoopSaturate(float x) {
    if (x > 1.5f) return 1.5f + std::tanh(x - 1.5f) * 0.1f;
    if (x < -1.5f) return -1.5f + std::tanh(x + 1.5f) * 0.1f;
    return x;
}

inline int findNearestPrime(int n) {
    auto isPrime = [](int num) {
        if (num <= 1) return false;
        if (num <= 3) return true;
        if (num % 2 == 0 || num % 3 == 0) return false;
        for (int i = 5; i * i <= num; i += 6)
            if (num % i == 0 || num % (i + 2) == 0) return false;
        return true;
        };
    if (n <= 2) return 2;
    int up = n;
    while (!isPrime(up)) up++;
    int down = n;
    while (down > 2 && !isPrime(down)) down--;
    return ((up - n) < (n - down)) ? up : down;
}

inline float lagrange3rdInterpolate(float frac, float y0, float y1, float y2, float y3) {
    float f_minus_1 = frac + 1.0f;
    float f_minus_2 = frac - 1.0f;
    float f_minus_3 = frac - 2.0f;

    float l0 = -(frac * f_minus_2 * f_minus_3) / 6.0f;
    float l1 = (f_minus_1 * f_minus_2 * f_minus_3) / 2.0f;
    float l2 = -(f_minus_1 * frac * f_minus_3) / 2.0f;
    float l3 = (f_minus_1 * frac * f_minus_2) / 6.0f;

    return y0 * l0 + y1 * l1 + y2 * l2 + y3 * l3;
}

class LFO {
public:
    void setSampleRate(double sr) { sampleRate = sr; }
    void advanceBlock(float freq, int waveType, int numSamples) {
        float step = (freq / (float)sampleRate) * (float)numSamples;
        phase += step;
        while (phase >= 1.0f) {
            phase -= 1.0f;
            shValue = random.nextFloat();
        }
    }
    float getValue(int waveType) {
        switch (waveType) {
        case 0: return 0.5f * (1.0f + std::sin((phase - 0.25f) * juce::MathConstants<float>::twoPi));
        case 1: return phase;
        case 2: return phase < 0.5f ? 1.0f : 0.0f;
        case 3: return shValue;
        case 4: return random.nextFloat();
        default: return 0.0f;
        }
    }
private:
    float phase = 0.0f;
    double sampleRate = 48000.0;
    float shValue = 0.0f;
    FastRandom random;
};

class ChaosLFO {
    float phase1 = 0.0f, phase2 = 0.0f, inc1 = 0.0f, inc2 = 0.0f;
public:
    void setFrequency(float freq, float sampleRate) {
        float safeFreq = (freq > 0.0f) ? freq : 0.0f;
        inc1 = safeFreq / sampleRate;
        inc2 = (safeFreq * 1.41421356f) / sampleRate;
    }
    void setPhase(float p) { phase1 = p; phase2 = p * 1.618f; }
    inline float process() {
        if (inc1 <= 0.0f) return 0.0f;
        phase1 += inc1; if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += inc2; if (phase2 >= 1.0f) phase2 -= 1.0f;
        return (std::sin(phase1 * juce::MathConstants<float>::twoPi) + std::sin(phase2 * juce::MathConstants<float>::twoPi)) * 0.5f;
    }
};

// ==========================================
// Effects Engine
// ==========================================
class OctaveShifter {
    std::array<float, 4096> buffer;
    int writeIdx = 0;
    float phase = 0.0f;
public:
    OctaveShifter() { buffer.fill(0.0f); }
    float process(float in) {
        buffer[(size_t)writeIdx] = in;
        float windowSize = 2048.0f;
        float phaseStep = 1.0f / windowSize;
        phase += phaseStep;
        if (phase >= 1.0f) phase -= 1.0f;
        float phaseB = phase + 0.5f;
        if (phaseB >= 1.0f) phaseB -= 1.0f;
        float delayA = (1.0f - phase) * windowSize;
        float delayB = (1.0f - phaseB) * windowSize;

        auto getInterpolated = [&](float d) {
            float readPos = (float)writeIdx - d;
            if (readPos < 0.0f) readPos += 4096.0f;
            int idx1 = (int)readPos;
            float frac = readPos - (float)idx1;
            int idx2 = (idx1 + 1) % 4096;
            return buffer[(size_t)idx1] * (1.0f - frac) + buffer[(size_t)idx2] * frac;
            };

        float outA = getInterpolated(delayA);
        float outB = getInterpolated(delayB);
        float winA = 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
        float winB = 0.5f * (1.0f - std::cos(phaseB * juce::MathConstants<float>::twoPi));
        writeIdx = (writeIdx + 1) % 4096;
        return outA * winA + outB * winB;
    }
};

struct VelvetNoiseDiffuser {
    std::vector<float> buffer;
    int writePos = 0;
    int bufferMask = 0;
    struct Tap { int delay; float gain; };
    std::vector<Tap> taps;
    float amount = 0.0f;

    void prepare(float fs) {
        int size = 16384;
        buffer.assign((size_t)size, 0.0f);
        bufferMask = size - 1;
        writePos = 0;
        taps.clear();
        int numTaps = 48;
        float durationMs = 50.0f;
        float totalSamples = durationMs * fs * 0.001f;
        float grid = totalSamples / (float)numTaps;
        std::mt19937 gen(12345);
        std::uniform_real_distribution<float> distOffset(0.0f, grid - 1.0f);
        std::uniform_int_distribution<> sign(0, 1);
        float normGain = 1.0f / std::sqrt((float)numTaps);
        for (int i = 0; i < numTaps; ++i) {
            Tap t;
            t.delay = (int)((float)i * grid + distOffset(gen));
            if (t.delay < 1) t.delay = 1;
            if (t.delay >= size) t.delay = size - 1;
            t.gain = (sign(gen) == 0 ? 1.0f : -1.0f) * normGain;
            taps.push_back(t);
        }
    }
    void setAmount(float a) { amount = a; }
    inline float process(float in) {
        if (amount < 0.01f || buffer.empty()) return in;
        buffer[(size_t)writePos] = in;
        float out = 0.0f;
        for (const auto& t : taps) {
            int r = (writePos - t.delay) & bufferMask;
            out += buffer[(size_t)r] * t.gain;
        }
        writePos = (writePos + 1) & bufferMask;
        return in * (1.0f - amount) + out * amount;
    }
};

static inline void matrixHadamard(float* x) {
    for (int i = 0; i < 16; i += 2) { float a = x[i]; float b = x[i + 1]; x[i] = a + b; x[i + 1] = a - b; }
    for (int i = 0; i < 16; i += 4) {
        float a0 = x[i]; float b0 = x[i + 2]; x[i] = a0 + b0; x[i + 2] = a0 - b0;
        float a1 = x[i + 1]; float b1 = x[i + 3]; x[i + 1] = a1 + b1; x[i + 3] = a1 - b1;
    }
    for (int i = 0; i < 16; i += 8) {
        for (int j = 0; j < 4; ++j) {
            float a = x[i + j]; float b = x[i + j + 4]; x[i + j] = a + b; x[i + j + 4] = a - b;
        }
    }
    for (int i = 0; i < 8; ++i) { float a = x[i]; float b = x[i + 8]; x[i] = a + b; x[i + 8] = a - b; }
    for (int i = 0; i < 16; ++i) x[i] *= 0.25f;
}

class AbyssReverb {
    static constexpr int NUM_CHANNELS = 16;
    std::vector<std::vector<float>> delayBuffers;
    std::array<int, NUM_CHANNELS> writePos;
    std::array<int, NUM_CHANNELS> delayLengths;
    std::array<float, NUM_CHANNELS> dampStates;
    std::array<ChaosLFO, NUM_CHANNELS> lfos;
    VelvetNoiseDiffuser velvetL, velvetR;
    OctaveShifter shifter;
    double sampleRate = 48000.0;
public:
    AbyssReverb() {
        delayBuffers.resize(NUM_CHANNELS);
        writePos.fill(0);
        delayLengths.fill(0);
        dampStates.fill(0.0f);
    }
    void prepareToPlay(double sr) {
        sampleRate = sr;
        velvetL.prepare((float)sr);
        velvetR.prepare((float)sr);
        int maxDelaySamples = (int)(sr * 2.0);
        const float LFO_RATIOS[16] = {
            1.000f, 0.618f, 1.272f, 0.786f, 1.618f, 0.382f, 1.414f, 0.528f,
            1.175f, 0.854f, 1.324f, 0.472f, 1.089f, 0.927f, 1.236f, 0.691f
        };
        const float baseDelaysMs[16] = {
            31.0f, 37.0f, 41.0f, 43.0f, 47.0f, 53.0f, 59.0f, 61.0f,
            67.0f, 71.0f, 73.0f, 79.0f, 83.0f, 89.0f, 97.0f, 101.0f
        };
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            delayBuffers[i].assign((size_t)maxDelaySamples, 0.0f);
            float targetSamps = baseDelaysMs[i] * 0.001f * (float)sr;
            delayLengths[i] = findNearestPrime((int)targetSamps);
            if (delayLengths[i] > maxDelaySamples - 100) delayLengths[i] = maxDelaySamples - 100;
            writePos[i] = 0;
            dampStates[i] = 0.0f;
            lfos[i].setFrequency(0.5f * LFO_RATIOS[i], (float)sr);
            lfos[i].setPhase((float)i / (float)NUM_CHANNELS);
        }
    }
    void process(float& inOutL, float& inOutR, float amount) {
        if (amount <= 0.0f) return;
        velvetL.setAmount(amount * 0.8f);
        velvetR.setAmount(amount * 0.8f);
        float vL = velvetL.process(inOutL);
        float vR = velvetR.process(inOutR);
        float monoIn = (vL + vR) * 0.5f;
        float shimmerSig = shifter.process(monoIn);
        float shimmerMix = amount * 0.5f;
        float injectL = vL + shimmerSig * shimmerMix;
        float injectR = vR + shimmerSig * shimmerMix;
        float delayOutputs[16] = { 0.0f };
        float feedbackInputs[16] = { 0.0f };
        float lfoDepth = amount * 12.0f;
        float dampFactor = 0.1f + (1.0f - amount) * 0.4f;
        float feedback = std::min(0.98f, 0.5f + amount * 0.48f);
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            float mod = lfos[i].process() * lfoDepth;
            float readPos = (float)writePos[i] - ((float)delayLengths[i] + mod);
            int bufSize = (int)delayBuffers[i].size();
            if (bufSize == 0) continue;
            while (readPos < 0.0f) readPos += (float)bufSize;
            while (readPos >= (float)bufSize) readPos -= (float)bufSize;
            int idx1 = (int)readPos;
            float frac = readPos - (float)idx1;
            int idx2 = idx1 + 1;
            if (idx2 >= bufSize) idx2 = 0;
            float rawRead = delayBuffers[i][(size_t)idx1] * (1.0f - frac) + delayBuffers[i][(size_t)idx2] * frac;
            dampStates[i] = rawRead * (1.0f - dampFactor) + dampStates[i] * dampFactor;
            delayOutputs[i] = dampStates[i];
            feedbackInputs[i] = delayOutputs[i];
        }
        matrixHadamard(feedbackInputs);
        float sumL = 0.0f, sumR = 0.0f;
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            int bufSize = (int)delayBuffers[i].size();
            if (bufSize == 0) continue;
            float inSig = (i % 2 == 0) ? injectL : injectR;
            float v_n = inSig + feedbackInputs[i] * feedback;
            v_n = safeLoopSaturate(v_n);
            v_n = antiDenormal(v_n);
            delayBuffers[i][(size_t)writePos[i]] = v_n;
            writePos[i]++;
            if (writePos[i] >= bufSize) writePos[i] = 0;
            if (i < 8) sumL += delayOutputs[i];
            else       sumR += delayOutputs[i];
        }
        sumL *= 0.25f; sumR *= 0.25f;
        auto softClipOutput = [](float x) {
            float ax = std::abs(x);
            if (ax > 1.0f) return x > 0.0f ? 1.0f : -1.0f;
            return x * (1.5f - 0.5f * x * x);
            };
        inOutL = inOutL * (1.0f - amount) + softClipOutput(sumL) * amount * 1.5f;
        inOutR = inOutR * (1.0f - amount) + softClipOutput(sumR) * amount * 1.5f;
    }
};

class MutatorFX {
public:
    void prepareToPlay(double sr) {
        sampleRate = sr;
        delayBuffer.fill(0.0f);
        writeIdx = 0;
        lfoPhase = 0.0f;
        lastL = 0.0f;
        lastR = 0.0f;
    }
    void process(float& inOutL, float& inOutR, float amount) {
        if (std::abs(amount) < 0.01f) return;

        if (amount > 0.0f) {
            // 【完全修正】サチュレーションを撤去した純粋なAM（Ring Modulator）
            float freq = 20.0f + amount * 1980.0f;
            lfoPhase += freq / (float)sampleRate;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            float mod = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
            float rmOutL = inOutL * mod;
            float rmOutR = inOutR * mod;

            inOutL = inOutL * (1.0f - amount) + rmOutL * amount;
            inOutR = inOutR * (1.0f - amount) + rmOutR * amount;
        }
        else {
            float absAmt = -amount;
            float freq = 50.0f + absAmt * 950.0f;
            float delaySamps = (float)sampleRate / freq;

            int halfSize = (int)(delayBuffer.size() / 2);
            float readPos = (float)writeIdx - delaySamps;
            if (readPos < 0.0f) readPos += (float)halfSize;

            int idx1 = (int)readPos;
            if (idx1 >= halfSize) idx1 = 0;
            float frac = readPos - (float)idx1;
            int idx2 = idx1 + 1;
            if (idx2 >= halfSize) idx2 -= halfSize;

            float dL = delayBuffer[(size_t)idx1 * 2] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2] * frac;
            float dR = delayBuffer[(size_t)idx1 * 2 + 1] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2 + 1] * frac;

            float feedback = 0.90f + absAmt * 0.08f;
            float filterCoef = 0.3f;

            float fbL = dL * (1.0f - filterCoef) + lastL * filterCoef;
            float fbR = dR * (1.0f - filterCoef) + lastR * filterCoef;
            lastL = fbL; lastR = fbR;

            delayBuffer[(size_t)writeIdx * 2] = inOutL + fbL * feedback;
            delayBuffer[(size_t)writeIdx * 2 + 1] = inOutR + fbR * feedback;

            writeIdx++;
            if (writeIdx >= halfSize) writeIdx = 0;

            inOutL = inOutL * (1.0f - absAmt) + dL * absAmt;
            inOutR = inOutR * (1.0f - absAmt) + dR * absAmt;
        }
    }
private:
    double sampleRate = 48000.0;
    std::array<float, 96000 * 2> delayBuffer;
    int writeIdx = 0;
    float lfoPhase = 0.0f;
    float lastL = 0.0f, lastR = 0.0f;
};

class PhantomDelay {
public:
    void prepareToPlay(double sr) {
        sampleRate = sr;
        delayBuffer.fill(0.0f);
        writeIndex = 0;
        envelope = 0.0f;
        currentDelaySamps = 0.0f;
    }
    void process(float& inOutL, float& inOutR, float amount, double bpm) {
        if (amount <= 0.0f) return;
        float inSum = std::abs(inOutL) + std::abs(inOutR);
        float attack = 0.001f;
        float release = 0.0002f;
        if (inSum > envelope) envelope += attack * (inSum - envelope);
        else envelope += release * (inSum - envelope);
        float duckingGain = 1.0f - juce::jlimit(0.0f, 0.7f, envelope * (1.0f + amount * 4.0f));
        float feedback = 0.3f + (amount * 0.5f);
        double effectiveBpm = bpm > 0.0 ? bpm : 120.0;
        double beatSec = 60.0 / effectiveBpm;
        int halfSize = (int)(delayBuffer.size() / 2);
        float targetDelaySamps = (float)(sampleRate * beatSec * 0.75);
        if (targetDelaySamps > (float)(halfSize - 2)) targetDelaySamps = (float)(halfSize - 2);
        if (currentDelaySamps == 0.0f) currentDelaySamps = targetDelaySamps;
        currentDelaySamps += 0.002f * (targetDelaySamps - currentDelaySamps);
        float readPos = (float)writeIndex - currentDelaySamps;
        if (readPos < 0.0f) readPos += (float)halfSize;
        int idx1 = (int)readPos;
        if (idx1 >= halfSize) idx1 = 0;
        if (idx1 < 0) idx1 += halfSize;
        float frac = readPos - (float)idx1;
        int idx2 = idx1 + 1;
        if (idx2 >= halfSize) idx2 -= halfSize;
        float dL = delayBuffer[(size_t)idx1 * 2] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2] * frac;
        float dR = delayBuffer[(size_t)idx1 * 2 + 1] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2 + 1] * frac;
        delayBuffer[(size_t)writeIndex * 2] = (inOutL * duckingGain) + (dL * feedback);
        delayBuffer[(size_t)writeIndex * 2 + 1] = (inOutR * duckingGain) + (dR * feedback);
        writeIndex++;
        if (writeIndex >= halfSize) writeIndex = 0;
        inOutL += dL * amount;
        inOutR += dR * amount;
    }
private:
    double sampleRate = 48000.0;
    std::array<float, 192000 * 2> delayBuffer;
    int writeIndex = 0;
    float envelope = 0.0f;
    float currentDelaySamps = 0.0f;
};

class WarpSmear {
public:
    void prepareToPlay(double sr) {
        sampleRate = sr;
        buffer.fill(0.0f);
        writeIndex = 0;
    }
    void process(float& inOutL, float& inOutR, float amount) {
        if (amount <= 0.0f) return;
        int halfSize = (int)(buffer.size() / 2);
        int delaySamps = (int)(sampleRate * 0.1);
        int readIndex = writeIndex - delaySamps;
        if (readIndex < 0) readIndex += halfSize;
        float smL = buffer[(size_t)readIndex * 2];
        float smR = buffer[(size_t)readIndex * 2 + 1];
        float freezeFeedback = amount * 0.98f;
        buffer[(size_t)writeIndex * 2] = inOutL + smL * freezeFeedback;
        buffer[(size_t)writeIndex * 2 + 1] = inOutR + smR * freezeFeedback;
        writeIndex++;
        if (writeIndex >= halfSize) writeIndex = 0;
        inOutL = inOutL * (1.0f - amount) + smL * amount;
        inOutR = inOutR * (1.0f - amount) + smR * amount;
    }
private:
    double sampleRate = 48000.0;
    std::array<float, 96000 * 2> buffer;
    int writeIndex = 0;
};

class AegisLimiter {
public:
    void process(float& inOutL, float& inOutR, float amountDB) {
        if (amountDB >= -0.01f) return;
        float thresh = std::pow(10.0f, amountDB / 20.0f);
        auto softClip = [](float x, float t) {
            float absX = std::abs(x);
            if (absX <= t) return x;
            if (absX >= t + 0.5f) return (x > 0 ? 1.0f : -1.0f) * (t + 0.25f);
            float diff = absX - t;
            float val = t + diff - (diff * diff);
            return (x > 0 ? 1.0f : -1.0f) * val;
            };
        inOutL = softClip(inOutL, thresh);
        inOutR = softClip(inOutR, thresh);
    }
};

// ==========================================
// Pure Sampler Voice Engine
// ==========================================
class OtodeskVoice
{
public:
    OtodeskVoice();
    ~OtodeskVoice() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void startNote(int midiNote, float velocity, const OtodeskParams& p, const std::array<int, 8>& validSlots,
        const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* anchors,
        const std::array<std::array<std::atomic<bool>, 42>, 8>* anchorReady);
    void stopNote(bool allowTailOff);
    bool isActive() const;
    void clearCurrentNote();

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
        const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* anchors,
        const OtodeskParams& p);

    int getCurrentlyPlayingNote() const { return currentMidiNote; }

private:
    double currentSampleRate = 48000.0;
    int currentMidiNote = -1;
    bool playing = false;
    bool isOneShot = false;
    bool isReleasing = false;

    int currentSlot = 0;
    int currentAnchorIndex = 0;
    double playheadPhase = 0.0;
    float currentPitchRatio = 1.0f;
    float currentVelocity = 1.0f;

    float cachedLoopStart = -1.0f;
    float cachedLoopEnd = -1.0f;
    float zeroCrossedLoopStart = 0.0f;
    float zeroCrossedLoopEnd = 0.0f;

    juce::ADSR adsr;
    FastRandom random;
    juce::AudioBuffer<float> voiceInternalBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OtodeskVoice)
};

class OtodeskSynth
{
public:
    OtodeskSynth();
    ~OtodeskSynth() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float>& outputBuffer,
        juce::MidiBuffer& midiMessages,
        const std::array<std::array<juce::AudioBuffer<float>, 42>, 8>* sharedAnchorBuffers,
        const std::array<int, 8>& validSlots,
        const OtodeskParams& p,
        double bpm,
        OtodeskParams& outModulatedParams,
        const std::array<std::array<std::atomic<bool>, 42>, 8>* anchorReady);

private:
    static constexpr int NUM_VOICES = 16;
    std::array<OtodeskVoice, NUM_VOICES> voices;

    std::array<LFO, 8> lfo1Gen;
    std::array<LFO, 8> lfo2Gen;
    std::array<LFO, 8> lfo3Gen;

    juce::IIRFilter filterHPF[2];
    juce::IIRFilter filterLPF[2];
    double currentSampleRate = 48000.0;

    // 【完全修正】各FXをスロットごとに4インスタンス持つ配列に変更（共有バグの根絶）
    std::array<MutatorFX, 4> mutators;
    std::array<PhantomDelay, 4> phantoms;
    std::array<WarpSmear, 4> warps;
    std::array<AbyssReverb, 4> abysses;
    AegisLimiter aegis;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OtodeskSynth)
};