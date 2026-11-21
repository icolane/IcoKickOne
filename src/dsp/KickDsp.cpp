#include "IcoKickOne/KickDsp.h"

#include <algorithm>
#include <cmath>

namespace kicklab
{
namespace
{
constexpr double twoPi = 6.2831853071795864769;

float nextNoise(Voice& voice)
{
    voice.noiseState ^= voice.noiseState << 13;
    voice.noiseState ^= voice.noiseState >> 17;
    voice.noiseState ^= voice.noiseState << 5;
    return static_cast<float>(voice.noiseState & 0x00ffffffu) / 8388608.0f - 1.0f;
}

float smoothClip(float sample)
{
    return std::tanh(sample);
}
}

std::array<float, 9> styleFactors(int style)
{
    switch (style)
    {
        case 0: return { 0.82f, 0.88f, 0.72f, 0.80f, 0.85f, 1.15f, 0.90f, 0.75f, 1.10f };
        case 1: return { 1.08f, 1.10f, 1.00f, 1.08f, 1.02f, 0.92f, 1.10f, 0.85f, 1.00f };
        case 2: return { 1.12f, 1.35f, 0.90f, 1.25f, 1.12f, 0.78f, 1.28f, 1.00f, 0.94f };
        case 3: return { 0.85f, 0.92f, 0.82f, 0.88f, 0.92f, 1.20f, 0.96f, 0.65f, 1.08f };
        case 4: return { 0.90f, 0.92f, 1.40f, 0.92f, 1.10f, 0.90f, 0.95f, 0.70f, 1.05f };
        case 5: return { 0.92f, 0.65f, 0.78f, 0.72f, 0.86f, 1.10f, 0.72f, 0.55f, 1.12f };
        case 6: return { 1.05f, 1.28f, 0.96f, 1.30f, 1.00f, 0.82f, 1.18f, 0.70f, 0.98f };
        case 7: return { 1.00f, 1.05f, 0.85f, 1.12f, 0.92f, 0.96f, 1.08f, 0.72f, 1.02f };
        default: return { 0.98f, 0.82f, 0.88f, 0.86f, 0.92f, 1.00f, 0.88f, 0.62f, 1.08f };
    }
}

float renderSample(Voice& voice, const Parameters& p, double sampleRate)
{
    if (! voice.active || sampleRate <= 0.0)
        return 0.0f;

    const auto factors = styleFactors(p.style);
    const auto age = static_cast<float>(voice.ageSeconds);
    const auto decay = std::max(0.12f, p.decaySeconds * factors[0]);
    if (age >= decay * 5.0f)
    {
        voice.active = false;
        return 0.0f;
    }

    const auto sweep = std::clamp(p.sweepSemitones * factors[1], 0.0f, 48.0f);
    const auto sweepTime = std::max(0.005f, p.sweepTimeMs * 0.001f);
    const auto pitch = std::pow(2.0, (sweep * std::exp(-age / sweepTime)) / 12.0);
    const auto fundamental = std::clamp(static_cast<double>(p.rootHz) * pitch, 25.0, 1200.0);
    voice.phase += twoPi * fundamental / sampleRate;
    voice.subPhase += twoPi * std::max(22.0, static_cast<double>(p.rootHz) * 0.5) / sampleRate;
    if (voice.phase >= twoPi)
        voice.phase = std::fmod(voice.phase, twoPi);
    if (voice.subPhase >= twoPi)
        voice.subPhase = std::fmod(voice.subPhase, twoPi);

    const auto bodyEnvelope = std::exp(-age / decay);
    const auto transientEnvelope = std::exp(-age / (0.004f + 0.035f * p.moduleAmount));
    const auto rumbleDecay = std::exp(-age / (decay * 1.8f));
    const auto secondHarmonic = static_cast<float>(std::sin(voice.phase * 2.0)) * (0.08f + 0.20f * p.body);
    auto body = static_cast<float>(std::sin(voice.phase)) * p.body + secondHarmonic;
    auto transient = nextNoise(voice) * p.click * transientEnvelope * 0.22f;
    auto sub = static_cast<float>(std::sin(voice.subPhase)) * p.rumble * factors[2] * rumbleDecay * 0.34f;
    const auto slope = body - voice.previousBody;
    voice.previousBody = body;

    auto sample = body * bodyEnvelope;
    if (p.modules[0])
        sample += slope * p.moduleAmount * factors[0] * bodyEnvelope * 1.8f;
    sample += transient;
    if (p.modules[2])
        sample += sub;
    if (p.modules[3])
        sample += smoothClip(sample * (1.0f + p.moduleAmount * 1.7f * factors[3])) * 0.20f;
    if (p.modules[4])
        sample += static_cast<float>(std::sin(voice.phase * 0.5)) * bodyEnvelope * p.moduleAmount * 0.10f;

    const auto drive = std::pow(10.0f, std::clamp(p.driveDb, 0.0f, 24.0f) * factors[1] / 20.0f);
    if (p.modules[1])
        sample = smoothClip(sample * drive) / std::max(1.0f, std::sqrt(drive));
    if (p.modules[5])
    {
        const auto cutoff = std::clamp(0.42f - p.moduleAmount * 0.26f, 0.03f, 0.45f);
        voice.lowPassState += cutoff * (sample - voice.lowPassState);
        sample = voice.lowPassState;
    }
    if (p.modules[6])
        sample = smoothClip(sample * 1.55f);
    if (p.modules[7])
        sample += nextNoise(voice) * transientEnvelope * 0.025f * p.moduleAmount;

    auto outputGain = std::clamp(p.output, 0.0f, 1.5f);
    if (p.autoGain)
        outputGain *= 1.0f / std::max(1.0f, 0.7f + drive * 0.18f);
    if (p.modules[8])
        sample = smoothClip(sample * 1.25f);

    voice.ageSeconds += 1.0 / sampleRate;
    return std::clamp(sample * outputGain, -0.98f, 0.98f);
}
}
