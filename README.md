# Otodesk Sampler

> **Multi-Slot Advanced Sampler Plugin for Ableton Live**  
> A powerful VST3 sampler with intelligent pitch detection, real-time stretching, and a comprehensive modulation engine.

![Release](https://img.shields.io/badge/release-v1.0-blue)
![License](https://img.shields.io/badge/license-AGPLv3-green)
![JUCE](https://img.shields.io/badge/JUCE-8.0.8-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20-lightgrey)
![Downloads](https://img.shields.io/github/downloads/OTODESK4193/OtodeskSampler/total.svg)

---

##
<img src="/Screenshot.jpg" width="600">

👉 **[Watch the Demo Video on X (動作デモ動画はこちら！)](https://x.com/kijyoumusic/status/2050780250024657267?s=20)**



## 🎹 Overview

Otodesk Sampler is a sophisticated sampler plugin designed for modern music production workflows. It combines multi-slot sample playback, intelligent pitch detection, real-time time-stretching, and a deep modulation/effects system—all optimized for Ableton Live.

### Key Features

✨ **8 Independent Sample Slots** — Layer up to 8 samples with individual controls  
🎯 **Intelligent Pitch Detection** — Automatic root key detection with visual analysis  
⏱️ **Real-Time Time-Stretching** — RubberBand integration for artifact-free stretching  
🔄 **Advanced Looping** — Zero-crossing detection, crossfading, and loop automation  
📡 **3 LFOs per Slot** — Modulate any parameter with 13+ target destinations  
🎨 **4 Effect Slots** — Ring Modulation, Phantom Delay, Freeze, Hall Reverb, Limiter  
🛡️ **Aegis Limiter** — Adaptive soft-clipping protection  
🎓 **Material Detection** — Auto-analyze samples for optimal processing  

---

## 📋 System Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Windows 10+ |
| **DAW** | Ableton Live 11+ |
| **Format** | VST3 |
| **RAM** | 2GB minimum (4GB+ recommended) |
| **CPU** | Multi-core (Intel i5/AMD Ryzen 5 equivalent) |
| **Sample Rates** | 44.1kHz – 192kHz |

---

## 🚀 Quick Start

### Installation

1. **Download** the latest `.vst3` file from [Releases](https://github.com/OTODESK4193/OtodeskSampler/releases/tag/%EF%BD%961.0.0)
2. **Place** in your VST3 plugin directory:
   - **Windows**: `C:\Program Files\Common Files\VST3\`
3. **Rescan** plugins in Ableton Live (Preferences → Plug-in Sources → Re-Scan)
4. **Drag & Drop** Otodesk Sampler onto a MIDI track

### Loading Your First Sample

1. **Drag & Drop** an audio file (.wav, .mp3, .aif) onto the waveform area
2. **Select** a slot (1–8) to target the sample
3. **Auto-Analysis** begins automatically (you'll see pitch info in the HUD)
4. **Play** a MIDI note to trigger the sample

---

## 🎛️ Interface Guide

### Main Controls

#### Envelope & Timing
- **Attack**: Note-on fade-in time (1ms – 2000ms)
- **Release**: Note-off fade-out time (1ms – 5000ms)

#### Sample Region
- **Sample Start/End**: Define playback region (0.0–1.0, normalized)
- **Loop Start/Length**: Set looping region with zero-crossing optimization
- **X-Fade**: Crossfade at loop seam (0.0–0.5)

#### Pitch
- **Octave**: Shift by octaves (−3 to +3)
- **Semi (st)**: Shift by semitones (−24 to +24)
- **FineTune**: Cent-level adjustment (−100 to +100)

#### Mixing
- **Slot Pan**: Left-Right positioning (−1.0 to +1.0)
- **Slot Gain**: Per-slot output level (−36 to +12 dB)
- **Master Gain**: Global output control (−36 to +12 dB)

#### Playback
- **Mode**: Toggle Loop / One-Shot per slot
- **Material**: Auto-select analysis method (Crisp, Smooth, Formant)
- **Random Slot**: Randomly select from loaded slots on each note

---

## 🎨 Effects Engine

### 4 Effect Slots

| Effect | Description | Range |
|--------|-------------|-------|
| **Mutator** | Ring modulation + comb filtering | ±100 |
| **Phantom Delay** | Tempo-synced slapback with ducking | ±100 |
| **Warp Smear** | Freeze effect with long feedback tail | ±100 |
| **Abyss Reverb** | Hall-style convolution reverb with modulation | ±100 |

### Safety Limiter
- **Aegis**: Soft-clipping protection (−24 to 0 dB, 0 = bypass)

---

## 📡 LFO System

### 3 LFOs per Slot

Each LFO offers:
- **5 Waveforms**: Sine, Saw, Square, Random S&H, Noise
- **13 Modulation Targets**:
  - Loop Start / Loop Length / X-Fade
  - Pan / Sample Start
  - Pitch (Octave, Semitone, Fine)
  - FX 1–4 Amount
- **Tempo Sync**: Free (0.1–20 Hz) or Synced (1/1, 1/2, 1/4, 1/8, 1/16, 1/32)
- **Min/Max Range**: Define modulation depth

### Example: Grain Texture
Use very short samples with fast attack/release and LFO on Sample Start for evolving granular effects.

---

## 🛠️ Building from Source

### Requirements
- **JUCE 8.0.8**
- **Visual Studio 2022** (Windows) 
- **RubberBand** time-stretch library
- **C++17** standard or later

---

### Component Structure

```
PluginProcessor (Audio Engine)
├── Sample Loading & Pitch Detection
├── APVTS State Management
├── Multi-threaded Analysis Thread
└── Effect/LFO Routing

PluginEditor (UI)
├── Waveform Display
├── Parameter Attachments
├── Slot Selection
└── Visual Feedback

Otodesk_samplerDSP (Core DSP)
├── OtodeskVoice (Sample Playback)
├── OtodeskSynth (Voice Manager, 16 voices)
├── LFO / ChaosLFO (Modulation)
└── Effects (Mutator, Phantom, Warp, Abyss, Aegis)
```

### Key Classes

- **OtodeskSynth**: Manages up to 16 simultaneous voices
- **OtodeskVoice**: Individual sample playback with ADSR and pitch control
- **LFO / ChaosLFO**: Tempo-aware modulation generators
- **MutatorFX**: Ring modulation + comb filtering hybrid
- **PhantomDelay**: Tempo-synced delay with dynamic ducking
- **WarpSmear**: Long-tail freeze effect
- **AbyssReverb**: Diffusion-based reverb with LFO modulation
- **AegisLimiter**: Adaptive soft-clipping limiter

---

## 🎹 Usage Tips

### Grain Synthesis
Short samples + fast attack/release + Sample Start LFO = evolving granular textures

### Layered Pads
Load different samples in each slot, modulate pan & gain with synced LFOs for rich, shifting soundscapes

### Percussion Transformation
Apply high-amount Mutator effect to drums, chain with Abyss Reverb for metallic atmospheres

### Tempo-Locked Modulation
Sync LFOs to DAW tempo and apply to loop positions or pitch for rhythmically aligned morphing

---

## 🐛 Troubleshooting

### Sample won't load
✓ Verify file format is supported (.wav, .mp3, .aif)  
✓ Check file is not corrupted (open in external audio editor)  

### Click/pop at loop point
✓ Increase X-Fade (crossfade) value  
✓ Modulate Loop Length with smooth LFO waveform  
✓ Use MIDI envelope (Attack/Release) to soften edges  

### Pitch detection is wrong
✓ Try different Material Mode (Auto → Crisp/Smooth/Formant)  
✓ Click RE-ANALYZE to retrigger analysis  

### High CPU usage
✓ Reduce active voices (fewer simultaneous MIDI notes)  
✓ Simplify effect chains (fewer FX slots active)  
✓ Disable unused LFOs  

### Plugin won't appear in DAW
✓ Ensure VST3 folder path is correct  
✓ Run DAW rescan (Ableton: Preferences → Plug-in Sources → Re-Scan)  
✓ Verify 64-bit build on 64-bit system  

---

## 📖 Otodesk Sampler _ Quick Start Guide for Beginners(PDF）

👉 [**Otodesk Sampler _ Quick Start Guide for Beginners(PDF)**](OtodeskSampler_QuickStartGuide.pdf)

---
## 📖 OtodeskSampler_Manual (PDF)）

👉 [**OtodeskSampler_Manual (PDF)**](OtodeskSampler_Manual.pdf)
---
---

## License

This project is licensed under the GNU Affero General Public License v3.0 (AGPLv3) - see the [LICENSE](LICENSE) file for details.

This software is built using the **JUCE 8** framework. In accordance with JUCE 8's open-source licensing terms, this entire project is distributed under the AGPLv3.

### ライセンスについて
このプロジェクトは **GNU Affero General Public License v3.0 (AGPLv3)** のもとで公開されています。詳細は [LICENSE](LICENSE) ファイルを作成してください。

本プラグインは **JUCE 8** フレームワークを使用して開発されています。JUCE 8のオープンソースライセンス規約に基づき、本ソフトウェアのソースコードおよびバイナリにはAGPLv3が適用されます。

---

### Third-Party Libraries
- **JUCE 8.0.8**: JUCE End-User License Agreement
- **RubberBand**: GNU General Public License v2.0
- **KissFFT**: BSD License (pitch detection)

---

## 🎓 Credits

**Developer**: @kijyoumusic (OTODESK)  
**Music Production Background**: Electronic Music, Sound Design  
**Target DAW**: Ableton Live 11+  
**Framework**: JUCE 8.0.8  
**Platform Support**: Windows 10+

---

## 📞 Support

- **Social**: [@kijyoumusic](https://twitter.com/kijyoumusic)

---

**Enjoy the Otodesk Sampler! Happy sampling! 🎵**
