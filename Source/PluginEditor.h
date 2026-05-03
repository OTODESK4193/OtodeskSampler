// ==========================================
// File: PluginEditor.h
// ==========================================
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum class DragMode { None, SampleStart, SampleEnd, LoopStart, LoopEnd };

namespace VividPastelTheme {
    const juce::Colour bg = juce::Colour(0xFF16181A);
    const juce::Colour panel = juce::Colour(0xFF222528);
    const juce::Colour track = juce::Colour(0xFF0C0C0C);
    const juce::Colour gold = juce::Colour(0xFFE5C07B);
    const juce::Colour inactiveSlot = juce::Colour(0xFF333333);

    const juce::Colour slotColors[8] = {
        juce::Colour(0xFF4EE6C5), // 1: Mint
        juce::Colour(0xFFFF5C8D), // 2: Pink
        juce::Colour(0xFF4CB1FF), // 3: Blue
        juce::Colour(0xFFFFAD5C), // 4: Peach
        juce::Colour(0xFFC07BFF), // 5: Lilac
        juce::Colour(0xFFFFD166), // 6: Yellow
        juce::Colour(0xFF06D6A0), // 7: Emerald
        juce::Colour(0xFFEF476F)  // 8: Rose
    };
}

class SlotButton : public juce::TextButton {
public:
    std::function<void()> onRightClick;
    SlotButton(const juce::String& name) : juce::TextButton(name) {}

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown()) {
            if (onRightClick) onRightClick();
        }
        else {
            juce::TextButton::mouseDown(e);
        }
    }
};

class AbletonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour currentAccentColor = VividPastelTheme::slotColors[0];

    AbletonLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::white.withAlpha(0.3f));
        setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
        setColour(juce::ComboBox::backgroundColourId, VividPastelTheme::panel);
        setColour(juce::ComboBox::outlineColourId, VividPastelTheme::track);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        bool isOn = button.getToggleState();

        g.setColour(isOn ? currentAccentColor : juce::Colour(0xFF33363A));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isOn ? juce::Colours::black : juce::Colours::lightgrey);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, true);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto radius = (float)std::min(width, height) / 2.0f - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(VividPastelTheme::track);
        g.fillEllipse(rx, ry, rw, rw);

        juce::Path arc;
        arc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xFF3A3A3A));
        g.strokePath(arc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(currentAccentColor);
        g.strokePath(valArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path p;
        auto pointerLength = radius * 0.7f;
        auto pointerThickness = 3.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius + 2.0f, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        float trackHeight = 4.0f;
        juce::Rectangle<float> track((float)x, (float)y + ((float)height - trackHeight) * 0.5f, (float)width, trackHeight);
        g.setColour(VividPastelTheme::track);
        g.fillRoundedRectangle(track, 2.0f);

        juce::Rectangle<float> fill((float)x, track.getY(), std::max(0.0f, sliderPos - (float)x), trackHeight);
        g.setColour(currentAccentColor);
        g.fillRoundedRectangle(fill, 2.0f);

        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 6.0f, (float)y + (float)height * 0.5f - 6.0f, 12.0f, 12.0f);
    }
};

class NoSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, const juce::Slider::SliderStyle, juce::Slider&) override {}
};

struct LfoUI {
    juce::ToggleButton enableBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    juce::ComboBox targetCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;
    juce::ComboBox waveCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttach;
    juce::ToggleButton syncBtn{ "Sync" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach;
    juce::Slider freqFreeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqFreeAttach;
    juce::ComboBox freqSyncCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> freqSyncAttach;
    juce::Label minLabel;
    juce::Slider minSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minAttach;
    juce::Label maxLabel;
    juce::Slider maxSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxAttach;
};

struct FxSlotUI {
    juce::ComboBox typeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
    juce::Slider amountKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttach;
};

class Otodesk_samplerAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::FileDragAndDropTarget,
    public juce::ChangeListener
{
public:
    Otodesk_samplerAudioProcessorEditor(Otodesk_samplerAudioProcessor&);
    ~Otodesk_samplerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void vblankCallback();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    Otodesk_samplerAudioProcessor& audioProcessor;
    AbletonLookAndFeel abletonLnF;
    NoSliderLookAndFeel noSliderLnF;

    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;

    juce::Label statusLabelFile;
    juce::Label statusLabelTask;
    juce::Label statusLabelOrig;
    juce::Rectangle<int> progressBarRect;
    float currentProgress = 0.0f;
    int blinkCounter = 0;

    juce::Label attackLabel; juce::Slider attackSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttach;
    juce::Label releaseLabel; juce::Slider releaseSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;

    juce::Label sampleStartLabel; juce::Slider sampleStartSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleStartAttach;
    juce::Label sampleEndLabel; juce::Slider sampleEndSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleEndAttach;

    juce::Label loopStartLabel; juce::Slider loopStartSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopStartAttach;
    juce::Label loopLengthLabel; juce::Slider loopLengthSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopLengthAttach;
    juce::Label crossfadeLabel; juce::Slider crossfadeSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossfadeAttach;

    juce::Label playModeLabel; juce::ComboBox playModeCombo; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> playModeAttach;
    juce::Label rootKeyLabel; juce::ComboBox rootKeyCombo;
    juce::TextButton reanalyzeButton{ "RE-ANALYZE" };

    juce::Label panLabel; juce::Slider panSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttach;
    juce::Label slotGainLabel; juce::Slider slotGainSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slotGainAttach;
    juce::Label octLabel; juce::Slider octSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> octAttach;
    juce::Label semiLabel; juce::Slider semiSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> semiAttach;
    juce::Label fineLabel; juce::Slider fineSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineAttach;

    juce::Label gainLabel; juce::Slider gainSlider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;

    SlotButton slotButtons[8]{ {"1"}, {"2"}, {"3"}, {"4"}, {"5"}, {"6"}, {"7"}, {"8"} };

    juce::ToggleButton randomSlotButton{ "Random Slot" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> randomSlotAttach;

    juce::Label materialLabel;
    juce::ComboBox materialCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> materialAttach;

    LfoUI lfoUIs[3];
    std::array<FxSlotUI, 4> fxSlots;
    juce::Label aegisLabel; juce::Slider aegisKnob; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aegisAttach;

    juce::Rectangle<int> waveArea;
    juce::Rectangle<int> modMatrixArea;
    juce::Rectangle<int> fxRackArea;
    juce::Rectangle<int> terminalArea;
    juce::Rectangle<int> slotsArea;

    DragMode dragMode = DragMode::None;

    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTerminalHUD(juce::Graphics& g);
    void drawSlotsIndicator(juce::Graphics& g);

    void updateSlotAttachments();
    void updateThemeColor(int slotIndex);
    void setupGlobalSlider(juce::Label& lbl, juce::Slider& sld, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att, const juce::String& id, const juce::String& name);
    void setupSlotSlider(juce::Label& lbl, juce::Slider& sld, const juce::String& name);

    float getZeroCrossedPosition(float normalizedPos, int slotIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Otodesk_samplerAudioProcessorEditor)
};