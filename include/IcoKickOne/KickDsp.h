#pragma once

#include <array>
#include <cstdint>

namespace kicklab
{
struct Parameters
{
    int style = 1;
    float rootHz = 52.0f;
    float sweepSemitones = 30.0f;
    float sweepTimeMs = 28.0f;
    float decaySeconds = 0.68f;
    float body = 0.88f;
    float click = 0.22f;
    float rumble = 0.25f;
    float driveDb = 8.0f;
    float output = 0.78f;
    float moduleAmount = 0.65f;
    bool pitchTrack = true;
    bool autoGain = true;
    std::array<bool, 9> modules { true, true, true, true, true, true, true, false, true };
};

struct Voice
{
    bool active = false;
    double phase = 0.0;
    double subPhase = 0.0;
    double ageSeconds = 0.0;
    float noteRatio = 1.0f;
    float lowPassState = 0.0f;
    float previousBody = 0.0f;
    std::uint32_t noiseState = 0x9e3779b9u;
};

float renderSample(Voice& voice, const Parameters& parameters, double sampleRate);
std::array<float, 9> styleFactors(int style);
}
