// ==========================================
// File: PluginProcessor.h
// ==========================================
#pragma once

#define HAVE_KISSFFT 1
#define USE_KISSFFT 1
#define HAVE_SPEEXDSP 1
#define HAVE_SPEEX 1
#define USE_SPEEX 1
#define NO_TIMING 1
#define NO_THREAD_CHECKS 1
#define NOMINMAX 1

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include "Otodesk_samplerDSP.h"

enum class IngestState {
    Empty, Loading, Normalizing, Analyzing, Stretching, Ready, Failed
};

struct PitchInfo {
    double frequency = 0.0;
    juce::String noteName = "---";
    float centsOffset = 0.0f;
    int midiNote = 60;
    bool isDetected = false;
};

class Otodesk_samplerAudioProcessor : public juce::AudioProcessor,
    public juce::Thread,
    public juce::ChangeBroadcaster
{
public:
    Otodesk_samplerAudioProcessor();
    ~Otodesk_samplerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void loadFileForIngest(const juce::File& file, int targetSlot);
    void reingestCurrentFile();
    void triggerReanalyze();
    juce::String getFileName(int slotIndex) const;
    void clearSlot(int slotIndex);
    void run() override;

    juce::AudioFormatManager formatManager;

    std::array<std::array<juce::AudioBuffer<float>, 42>, 8> sharedAnchorBuffers;
    std::array<std::array<std::atomic<bool>, 42>, 8> anchorReady;
    std::array<int, 8> validSlots;
    std::array<juce::File, 8> slotFiles;
    std::array<int, 8> cachedDetectedRoots;
    std::array<std::array<float, 1000>, 8> waveformPeaks;
    std::array<std::atomic<bool>, 8> waveformReady;

    std::atomic<IngestState> ingestState{ IngestState::Empty };
    std::atomic<float> ingestProgress{ 0.0f };
    std::atomic<int> validSampleCount{ 0 };
    std::atomic<bool> triggerPreview{ false };
    std::atomic<int> previewNoteOffTimer{ 0 };

    juce::CriticalSection stateLock;
    juce::String hudLineFile = "File: ---";
    juce::String hudLineTask = "Task: Ready";
    juce::String hudLineOrig = "Original: ---";

    juce::CriticalSection queueLock;
    std::vector<int> ingestQueue;
    std::atomic<int> anchorsCompleted{ 0 };
    juce::CriticalSection etaLock;

private:
    double currentSampleRate = 48000.0;
    bool isReanalyzing = false;
    bool isRestoringFromSave = false;
    OtodeskSynth synth;

    PitchInfo analyzePitch(const float* audioData, int length, double sampleRate, float minFreq, float corrThreshold);
    PitchInfo analyzePitchCepstrum(const float* audioData, int length, double sampleRate);
    PitchInfo analyzePitchOmni(const float* audioData, int length, double sampleRate, const juce::String& filename, int materialMode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Otodesk_samplerAudioProcessor)
};