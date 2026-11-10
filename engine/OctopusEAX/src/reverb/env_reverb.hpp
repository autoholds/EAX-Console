// C++17, header-only, Orbital audio thread — line-for-line port of ReverbPresets.rs (EnvironmentReverb / Coefficients / Core / RateBridge)
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace env {

inline float bitsf(uint32_t b) { float f; std::memcpy(&f, &b, 4); return f; }
inline uint32_t fbits(float f) { uint32_t b; std::memcpy(&b, &f, 4); return b; }
inline float minf(float a, float b) { return std::fmin(a, b); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

constexpr size_t DelayLength = 0x20000;
constexpr size_t CoefficientCount = 62;
constexpr size_t TapCount = 27;
constexpr size_t LineCount = 8;
constexpr size_t OutputCount = 4;
constexpr size_t MovableTapCount = 17;
constexpr size_t FirstLineTap = 9;

namespace C {
constexpr size_t InputGain = 0, Rotation = 1, FeedTapGain = 2, LineLowPass = 3, LineAttenuation = 11, LineFilter = 19,
                 LateFeedGain = 43, ReflectionsGain = 44, InputLowPass = 45, BlockerFeedback = 46, InputScale = 47,
                 OscillatorRate = 48, ModulationDepth = 49, EarlyGain = 50, LateOutputGain = 54, EarlyAllPassRead = 58;
}
namespace T {
constexpr size_t Input = 0, LateFeed = 1, EarlySource = 2, EarlyAllPass = 5, Line = 9, FeedLine = 17, Modulated = 24, RearDelay = 25;
}

struct Parameters {
    int32_t Room, RoomHF;
    float DecayTime, DecayHFRatio;
    int32_t Reflections;
    float ReflectionsDelay;
    int32_t Reverb;
    float ReverbDelay, EnvironmentSize, Diffusion, AirAbsorptionHF, EchoDepth, EchoTime, ModulationDepth, ModulationTime, HFReference;
    bool DecayHFLimit;

    bool operator==(const Parameters& o) const {
        return Room == o.Room && RoomHF == o.RoomHF && DecayTime == o.DecayTime && DecayHFRatio == o.DecayHFRatio &&
               Reflections == o.Reflections && ReflectionsDelay == o.ReflectionsDelay && Reverb == o.Reverb &&
               ReverbDelay == o.ReverbDelay && EnvironmentSize == o.EnvironmentSize && Diffusion == o.Diffusion &&
               AirAbsorptionHF == o.AirAbsorptionHF && EchoDepth == o.EchoDepth && EchoTime == o.EchoTime &&
               ModulationDepth == o.ModulationDepth && ModulationTime == o.ModulationTime && HFReference == o.HFReference &&
               DecayHFLimit == o.DecayHFLimit;
    }
    bool operator!=(const Parameters& o) const { return !(*this == o); }
};

struct Preset {
    const char* Name;
    uint32_t DefaultAmount;
    Parameters Settings;
};

static const Preset Presets[15] = {
    {"Auditorium",     50, {-790,  -476,  4.32f,  0.59f, -789,  0.02f,  -289,   0.03f,  21.6f, 1.0f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.25f,  5000.0f, true}},
    {"Ball Park",      65, {-1190, 0,     14.69f, 1.54f, 790,   0.089f, -10000, 0.076f, 7.7f,  1.0f,  -5.0f, 0.3f,  0.075f, 0.0f,   0.04f,  5000.0f, true}},
    {"Bathroom",       50, {-370,  -1200, 1.49f,  0.54f, -370,  0.007f, 1030,   0.011f, 1.4f,  1.0f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.25f,  5000.0f, true}},
    {"Cathedral",      50, {0,     -1020, 5.9f,   0.7f,  -4660, 0.048f, 140,    0.048f, 10.0f, 0.8f,  -9.0f, 0.09f, 0.2f,   0.0f,   0.128f, 5000.0f, true}},
    {"Chapel",         65, {0,     -490,  2.03f,  0.5f,  -7120, 0.023f, -330,   0.044f, 10.0f, 0.5f,  -7.0f, 0.05f, 0.213f, 0.0f,   0.208f, 5000.0f, true}},
    {"Church",         50, {0,     -1500, 2.32f,  0.7f,  -5830, 0.032f, -720,   0.037f, 10.0f, 0.72f, -5.0f, 0.07f, 0.192f, 0.0f,   0.128f, 5000.0f, true}},
    {"Concert Hall",   50, {-390,  -500,  3.92f,  0.7f,  -1230, 0.02f,  -2,     0.029f, 19.6f, 1.0f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.25f,  5000.0f, true}},
    {"Garage",         50, {0,     -390,  1.45f,  0.72f, 310,   0.025f, -910,   0.012f, 10.0f, 0.8f,  -5.0f, 0.0f,  0.25f,  0.0f,   1.67f,  5000.0f, true}},
    {"Haunted Cavern", 50, {0,     -1500, 17.01f, 0.91f, 790,   0.111f, 1070,   0.05f,  10.0f, 0.4f,  -5.0f, 0.0f,  0.25f,  0.413f, 0.042f, 5000.0f, false}},
    {"Indoor Arena",   60, {-600,  -390,  4.25f,  0.41f, -120,  0.05f,  830,    0.021f, 10.0f, 0.9f,  -5.0f, 0.0f,  0.099f, 0.086f, 0.2f,   5000.0f, true}},
    {"Jazz Club",      65, {-600,  -530,  1.26f,  0.7f,  -170,  0.006f, 720,    0.093f, 10.0f, 1.0f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.057f, 5000.0f, true}},
    {"Living Room",    50, {-1360, -6000, 0.5f,   0.1f,  -1376, 0.003f, -1104,  0.004f, 2.5f,  1.0f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.25f,  5000.0f, true}},
    {"Opera House",    50, {0,     -300,  6.2f,   1.06f, -816,  0.083f, -1090,  0.011f, 31.0f, 0.95f, -5.0f, 0.0f,  0.25f,  0.0f,   0.395f, 5000.0f, true}},
    {"Small Room",     50, {0,     -440,  0.4f,   0.83f, -1620, 0.002f, 430,    0.003f, 2.0f,  0.9f,  -5.0f, 0.0f,  0.25f,  0.0f,   0.25f,  5000.0f, true}},
    {"Theater",        50, {-20,   -340,  2.0f,   0.7f,  -660,  0.01f,  540,    0.015f, 10.0f, 1.0f,  -5.0f, 0.0f,  0.128f, 0.0f,   0.128f, 5000.0f, true}},
};
constexpr size_t PresetCount = 15;
constexpr size_t DefaultPreset = 6;

// ---------------------------------------------------------------- coefficients

constexpr float MaximumSampleRate = 48000.0f;
constexpr float MillibelsToDecibels = 0.01f;
constexpr float DecibelsToNepers = 0.11513f;
constexpr float TwoPi = 6.2831855f;
constexpr float Pi = 3.1415927f;
constexpr double PiWide = 3.1415927410125732;
constexpr float DecayFloorDecibels = -60.0f;
constexpr float SpeedOfSound = 340.0f;
constexpr float LowPassCeiling = 0.985f;
constexpr float ReflectionsBaseGain = 0.125f;
constexpr float OutputGain = 0.6665f;
constexpr float EarlyAllPassReadGain = 1.0f;
constexpr float LowFrequencyReference = 250.0f;
constexpr double LowFrequencyDecayRatio = 1.0;
constexpr float SampleRateToModulationBase = 0.010000001f;
constexpr float FeedTapStart = 0.0491f;
constexpr float FeedTapOffset = 0.0091f;
constexpr float FeedTapRatioBase = 0.18533605f;
constexpr float FeedTapRatioPower = 0.14285715f;
constexpr double LineSplits[4] = {0.37, 0.28, 0.64, 0.8};

struct TapLayout { int32_t Base; uint32_t MaximumBits; int32_t MinimumSamples; };

static const TapLayout Layout[TapCount] = {
    {0x4b10, 0x3ecccccd, 0x20}, {0x4b10, 0x3ecccccd, 0x20}, {0x4fa1, 0x3cc49ba6, 1}, {0x4fa1, 0x3cc49ba6, 1},
    {0x4fa1, 0x3cc49ba6, 1},    {0x54f2, 0x3ce56042, 0},    {0x59e3, 0x3cd4fdf4, 0}, {0x5ea4, 0x3ccccccd, 0},
    {0x6455, 0x3cf5c290, 0},    {0x9376, 0x3e808313, 0},    {0xa4f7, 0x3dbe76c9, 0}, {0xccf8, 0x3e5a1cad, 0},
    {0xd849, 0x3d75c290, 0},    {0xf4da, 0x3e1ba5e4, 0},    {0x10733, 0x3dc7ae15, 0}, {0x1210c, 0x3e0ccccd, 0},
    {0x135bd, 0x3de147af, 0},   {0x13d4e, 0x3d23d70b, 0},   {0x13d4e, 0x3d23d70b, 0}, {0x13d4e, 0x3d23d70b, 0},
    {0x13d4e, 0x3d23d70b, 0},   {0x13d4e, 0x3d23d70b, 0},   {0x13d4e, 0x3d23d70b, 0}, {0x13d4e, 0x3d23d70b, 0},
    {0x13f3f, 0x3c23d70b, 0},   {0x13fbb, 0x3b122532, 0},   {0x14020, 0x3ae6afce, 0},
};

inline float MaximumTime(size_t i) { return bitsf(Layout[i].MaximumBits); }

struct Tap { int32_t Base = 0, Read = 0; };

struct Diffusion { float Rotation, Attenuation, Compensation; };

struct Plan {
    std::array<float, CoefficientCount> Coefficients{};
    std::array<float, TapCount> Times{};
    float ModulationBase = 0.0f, SampleRate = 0.0f;
    Parameters Settings{};
    float HFDecayTime = 0.0f;
    Diffusion Diff{};
    float FeedLevel = 0.0f;
};

inline Diffusion DiffusionFor(float Amount, float EchoDepth, float EchoTime, float ModulationDepth) {
    double Loss = (2.7857 - (double)EchoTime * 7.1429) * (double)EchoDepth * 0.2;
    const double ModulationLoss = (double)ModulationDepth * 0.3;
    if (Loss <= ModulationLoss) Loss = ModulationLoss;
    const double Raw = (double)Amount * (1.0 - Loss);
    const double Limited = Raw < 1.0 ? (Raw <= 0.0 ? 0.0 : Raw) : 1.0;
    const double Angle = (std::pow(Limited, 8.0) * 0.4 + Limited * 0.6) * PiWide * 0.25;
    const double Cosine = std::cos(Angle);
    const float Cube = (float)(Cosine * Cosine * Cosine);
    return {(float)std::tan(Angle), Cube, 1.0f / (Cube * Cube)};
}

inline std::array<float, 3> ShelfFor(double Difference, bool LowDecaysSlower, float Reference, float SampleRate) {
    const double Amplitude = std::pow(10.0, (Difference * 2.0) / 20.0);
    const double Half = (Amplitude + 1.0) * 0.5;
    const double Angle = (double)((Reference * TwoPi) / SampleRate);
    double Forward = Half - 1.0;
    const double Sine = std::sin(Angle);
    const double HalfLogTwo = (double)std::log(2.0f) * 0.5;
    const double RootTwo = (double)std::sqrt(2.0f);
    double Alpha = std::sinh((RootTwo * HalfLogTwo * Angle) / Sine) * Sine;
    if (LowDecaysSlower) { Alpha *= Amplitude; Forward = -Forward; }
    const double Scaled = Alpha / std::sqrt(Amplitude);
    return {(float)Forward, (float)Half, (float)((1.0 - Scaled) / (Scaled + 1.0))};
}

struct Decay {
    std::array<float, LineCount> LowPass{}, Attenuation{};
    std::array<std::array<float, 3>, LineCount> Shelf{};
    float Normalisation = 0.0f;
};

inline Decay DecayFor(const std::array<float, LineCount>& Lengths, const Parameters& S, float HFDecayTime, const Diffusion& D, float FeedLevel, float SampleRate) {
    const double Time = (double)S.DecayTime;
    const double LowLimit = 200.0 / (Time + 100.0);
    const double LowRatio = LowLimit < LowFrequencyDecayRatio ? LowLimit : LowFrequencyDecayRatio;
    double HighRatio = (double)HFDecayTime / Time;
    if (HighRatio <= 0.1) HighRatio = 0.1;
    double Angle = (double)((S.HFReference * TwoPi) / SampleRate);
    if (1.0 < HighRatio) Angle = PiWide;
    const double Exponent = 2.0 - 2.0 / HighRatio;
    const double Cosine = std::cos(Angle);
    Decay R;
    double Energy = 0.0;
    for (size_t L = 0; L < LineCount; ++L) {
        const double Middle = (double)(Lengths[L] * DecayFloorDecibels) / Time;
        const double Low = (double)(Lengths[L] * DecayFloorDecibels) / (LowRatio * Time);
        const double Gain = std::pow(10.0, Middle / 20.0);
        double Difference, LineGain;
        if (Low <= Middle) { Difference = Low - Middle; LineGain = Gain; }
        else { Difference = Middle - Low; LineGain = std::pow(10.0, (Low * 2.0 - Middle) / 20.0); }
        Energy += Gain * Gain;
        R.Shelf[L] = ShelfFor(Difference, Low > Middle, LowFrequencyReference, SampleRate);
        const double Ratio = (std::pow(Gain, Exponent) - 1.0) / (1.0 - Cosine);
        float LowPass;
        if (Ratio == 0.0) LowPass = 0.0f;
        else {
            const double Linear = Ratio * -2.0 - 2.0;
            const double Root = std::sqrt(Linear * Linear - Ratio * 4.0 * Ratio);
            double Pole = (Root - Linear) / (Ratio * 2.0);
            if (1.0 < std::fabs(Pole)) Pole = (-Linear - Root) / (Ratio * 2.0);
            LowPass = (float)Pole;
        }
        if (LowPassCeiling <= LowPass) LowPass = LowPassCeiling;
        R.LowPass[L] = LowPass;
        R.Attenuation[L] = (float)LineGain * D.Attenuation;
    }
    const double Level = std::sqrt((double)(2.0f / (D.Compensation * 8.0f)) * (1.0 - Energy * 0.125));
    R.Normalisation = (float)Level / FeedLevel;
    return R;
}

inline float RoomHighFrequencyPole(float Decibels, float Reference, float SampleRate) {
    const float Gain = (float)std::pow(10.0, (double)((Decibels * 0.5f) / 10.0f));
    if (Gain >= 1.0f) return 0.0f;
    const float Cosine = std::cos((Reference * TwoPi) / SampleRate);
    const float Root = std::sqrt((1.0f - Cosine) * Gain * 2.0f - (1.0f - Cosine * Cosine) * Gain * Gain);
    const float Pole = ((1.0f - Cosine * Gain) - Root) / (1.0f - Gain);
    return Pole < 1.0f ? (Pole <= 0.0f ? 0.0f : Pole) : 1.0f;
}

inline void RoomLowFrequencyBlocker(float Reference, float SampleRate, float& PoleOut, float& ScaleOut) {
    const float Cosine = std::cos((Reference * TwoPi) / SampleRate);
    const double Gain = 1.0;
    const double Offset = (double)Cosine - 1.0;
    const double Sum = Offset + Gain * 2.0;
    const double Cross = Offset - Gain * 2.0 * (double)Cosine;
    double Disc = Cross * Cross - Sum * Sum;
    if (Disc <= 0.0) Disc = 0.0;
    const double Raw = (-Cross - std::sqrt(Disc)) / (Sum + 1.1920928955078125e-7);
    double Floor, Ceiling;
    if (Raw < 0.0) { Floor = -0.9987000226974487; Ceiling = -1.000000013351432e-10; }
    else { Floor = 1.000000013351432e-10; Ceiling = 0.9987000226974487; }
    const double Pole = Ceiling <= Raw ? Ceiling : (Raw <= Floor ? Floor : Raw);
    PoleOut = (float)Pole;
    ScaleOut = (float)(Pole * 0.5 + 0.5);
}

inline std::array<float, LineCount> LineLengths(float EnvironmentSize, float EchoDepth, float EchoTime, float ModulationLimit) {
    const double Volume = (double)std::pow(EnvironmentSize, 3.0f) * 0.0625;
    const double Limited = (double)ModulationLimit < Volume ? (double)ModulationLimit : Volume;
    const double Scale = Limited < 1.0 ? (Limited <= 0.0625 ? 0.0625 : Limited) : 1.0;
    const float Echo = std::pow(EchoDepth, 0.71428573f);
    const double Skew = (double)Echo / 2.1;
    const double Ratio = std::pow((Skew + 1.0) / (2.0 - Skew), 1.0 / 3.0);
    const double RatioFourth = Ratio * Ratio * Ratio * Ratio;
    const double Remaining = 1.0 - (double)Echo;
    const double Blend = 1.0 - (std::pow(Remaining, 8.0) * 0.9 + Remaining * 0.1);
    const double Longest = (double)EchoTime * Blend + (1.0 - Blend) * 0.25;
    double Pairs[4];
    double Power = 1.0;
    for (double& P : Pairs) {
        P = (Power * Longest) / (((1.0 - RatioFourth) / (1.0 - Ratio)) * 0.25);
        Power *= Ratio;
    }
    const double Total = Longest * 4.0;
    if (Scale * 1.05 < Total)
        for (double& P : Pairs) P *= Scale / Total;
    std::array<float, LineCount> L{};
    for (size_t i = 0; i < 4; ++i) {
        const float First = (float)(Pairs[i] / (LineSplits[i] + 1.0));
        const float Second = (float)(Pairs[i] / (1.0 / LineSplits[i] + 1.0));
        L[i * 2] = minf(First, MaximumTime(T::Line + i * 2));
        L[i * 2 + 1] = minf(Second, MaximumTime(T::Line + i * 2 + 1));
    }
    return L;
}

inline std::array<float, TapCount> TapTimes(const Parameters& S, const std::array<float, LineCount>& Lengths) {
    std::array<float, TapCount> Times{};
    Times[T::Input] = minf(S.ReflectionsDelay, MaximumTime(T::Input));
    Times[T::LateFeed] = minf(S.ReverbDelay + S.ReflectionsDelay, MaximumTime(T::LateFeed));
    const float Spread = ((S.ReverbDelay + 0.01f) / 0.11f) * 0.024f;
    Times[T::EarlySource] = minf(Spread * 0.723f, MaximumTime(T::EarlySource));
    Times[T::EarlySource + 1] = minf(Spread * 0.435f, MaximumTime(T::EarlySource + 1));
    Times[T::EarlySource + 2] = minf(Spread, MaximumTime(T::EarlySource + 2));
    const float Stretch = S.ReverbDelay / 0.1f;
    static const float Slopes[4] = {0.017940002f, 0.01682f, 0.01606f, 0.019000001f};
    static const float Offsets[4] = {0.010470001f, 0.0099100005f, 0.00953f, 0.011000001f};
    for (size_t ch = 0; ch < OutputCount; ++ch)
        Times[T::EarlyAllPass + ch] = minf(Stretch * Slopes[ch] + Offsets[ch], MaximumTime(T::EarlyAllPass + ch));
    for (size_t i = 0; i < LineCount; ++i) Times[T::Line + i] = Lengths[i];
    const float Ratio = std::pow(FeedTapRatioBase, FeedTapRatioPower);
    float Position = FeedTapStart;
    for (size_t i = 0; i < 7; ++i) {
        Times[T::FeedLine + i] = minf(Position - FeedTapOffset, MaximumTime(T::FeedLine + i));
        Position *= Ratio;
    }
    for (size_t i = T::Modulated; i < TapCount; ++i) Times[i] = MaximumTime(i);
    return Times;
}

inline Tap TapFor(size_t Index, float Time, float SampleRate) {
    int32_t Samples = (int32_t)(Time * SampleRate);
    if (Samples < Layout[Index].MinimumSamples) Samples = Layout[Index].MinimumSamples;
    return {Layout[Index].Base, Layout[Index].Base - Samples};
}

inline uint32_t ChangedTaps(const Parameters& Old, const Parameters& New) {
    uint32_t Mask = 0;
    const bool Refl = Old.ReflectionsDelay != New.ReflectionsDelay;
    const bool Rev = Old.ReverbDelay != New.ReverbDelay;
    if (Refl) Mask |= 1u << T::Input;
    if (Refl || Rev) Mask |= 1u << T::LateFeed;
    if (Rev) Mask |= 0x7fu << T::EarlySource;
    const bool ModChanged = Old.ModulationDepth != New.ModulationDepth || Old.ModulationTime != New.ModulationTime;
    auto SizeOf = [](const Parameters& P) { return clampf(P.EnvironmentSize, 1.0f, 100.0f); };
    if (SizeOf(Old) != SizeOf(New) || Old.EchoDepth != New.EchoDepth || Old.EchoTime != New.EchoTime || ModChanged)
        Mask |= 0xffu << T::Line;
    return Mask;
}

inline void ApplyDecay(const Plan& P, const std::array<float, LineCount>& Lengths, std::array<float, CoefficientCount>& Co) {
    const float Reverb = (float)P.Settings.Reverb * MillibelsToDecibels;
    const Decay D = DecayFor(Lengths, P.Settings, P.HFDecayTime, P.Diff, P.FeedLevel, P.SampleRate);
    for (size_t L = 0; L < LineCount; ++L) {
        Co[C::LineLowPass + L] = D.LowPass[L];
        Co[C::LineAttenuation + L] = D.Attenuation[L];
        for (size_t k = 0; k < 3; ++k) Co[C::LineFilter + L * 3 + k] = D.Shelf[L][k];
    }
    Co[C::LateFeedGain] = std::exp(Reverb * DecibelsToNepers) * D.Normalisation;
}

inline Plan PlanFor(const Parameters& S, float SampleRateIn, const std::array<float, LineCount>* Playing) {
    const float SampleRate = minf(SampleRateIn, MaximumSampleRate);
    const float Room = (float)S.Room * MillibelsToDecibels;
    const float RoomHF = (float)S.RoomHF * MillibelsToDecibels;
    const float Reflections = (float)S.Reflections * MillibelsToDecibels;
    const float AirAbsorption = S.AirAbsorptionHF * MillibelsToDecibels;
    const float EnvironmentSize = 100.0f <= S.EnvironmentSize ? 100.0f : (S.EnvironmentSize <= 1.0f ? 1.0f : S.EnvironmentSize);
    const float ModulationBase = SampleRate * SampleRateToModulationBase * 0.5f;

    Plan P;
    auto& Co = P.Coefficients;
    Co[C::InputGain] = std::exp(Room * DecibelsToNepers);

    float HFDecayTime = (S.DecayHFRatio > 0.1f ? S.DecayHFRatio : 0.1f) * S.DecayTime;
    if (S.DecayHFLimit && AirAbsorption != 0.0f) {
        const float Limit = DecayFloorDecibels / (AirAbsorption * SpeedOfSound);
        if (Limit <= HFDecayTime) HFDecayTime = Limit;
    }

    const Diffusion D = DiffusionFor(S.Diffusion, S.EchoDepth, S.EchoTime, S.ModulationDepth);
    Co[C::Rotation] = D.Rotation;

    const float FeedTapGain = 1.0f - S.EchoDepth;
    Co[C::FeedTapGain] = FeedTapGain;
    const float FeedLevel = std::sqrt((FeedTapGain * FeedTapGain + 1.0f) * 0.5f);

    Co[C::OscillatorRate] = std::sin(Pi / (S.ModulationTime * SampleRate)) * 2.0f;
    const float Sweep = S.ModulationDepth * 4.0f * S.ModulationTime;
    float ModulationLimit, Depth;
    if (Sweep <= 1.0f) { ModulationLimit = 1.0f; Depth = Sweep; }
    else { ModulationLimit = (Sweep * 0.3f + 1.0f) / (Sweep * 1.3f); Depth = 1.0f; }
    Co[C::ModulationDepth] = (ModulationBase - 2.0f) * Depth;

    const auto Lengths = LineLengths(EnvironmentSize, S.EchoDepth, S.EchoTime, ModulationLimit);
    Co[C::ReflectionsGain] = std::exp(Reflections * DecibelsToNepers) * ReflectionsBaseGain;
    Co[C::InputLowPass] = RoomHighFrequencyPole(RoomHF, S.HFReference, SampleRate);
    float Blocker, Scale;
    RoomLowFrequencyBlocker(LowFrequencyReference, SampleRate, Blocker, Scale);
    Co[C::BlockerFeedback] = Blocker;
    Co[C::InputScale] = Scale;
    for (size_t ch = 0; ch < OutputCount; ++ch) {
        Co[C::EarlyGain + ch] = OutputGain;
        Co[C::LateOutputGain + ch] = OutputGain;
        Co[C::EarlyAllPassRead + ch] = EarlyAllPassReadGain;
    }

    P.Times = TapTimes(S, Lengths);
    P.ModulationBase = ModulationBase;
    P.SampleRate = SampleRate;
    P.Settings = S;
    P.HFDecayTime = HFDecayTime;
    P.Diff = D;
    P.FeedLevel = FeedLevel;
    auto Decayed = P.Coefficients;
    ApplyDecay(P, Playing ? *Playing : Lengths, Decayed);
    P.Coefficients = Decayed;
    return P;
}

// ---------------------------------------------------------------- core

class Core {
public:
    std::array<float, CoefficientCount> Coefficients{}, Targets{};
    std::array<Tap, TapCount> Taps{};
    float EarlyCoefficient = 0.4f;
    float ModulationBase = 0.0f;
    uint32_t PendingTaps = 0;
    int32_t FadeSamples = 480;
    float FadeFactor = 0.97857207f;

    Core() : Memory(DelayLength, bitsf(SilenceFloorBits)) {
        const float Seed = bitsf(StateSeedBits);
        InputLowPass = {Seed, Seed};
        BlockerInput = BlockerOutput = Seed;
        LineFilter.fill(Seed);
        LineLowPassState.fill(Seed);
    }

    void SnapToTargets() {
        Coefficients = Targets;
        Increments.fill(0.0f);
        Countdown = 0;
        PendingTaps = 0;
        Phase = Idle;
        MoverTap = 0;
    }

    void ArmSmoothing() {
        Countdown = 0;
        for (size_t i = 0; i < CoefficientCount; ++i) {
            const float Difference = Targets[i] - Coefficients[i];
            if (std::fabs(Difference) <= SmoothingThreshold) Increments[i] = 0.0f;
            else { Increments[i] = Difference * SmoothingStep; Countdown = SmoothingSamples; }
        }
    }

    inline void StepSmoothing() {
        if (Countdown > 0) {
            --Countdown;
            for (size_t i = 0; i < CoefficientCount; ++i) {
                if (Increments[i] != 0.0f) {
                    if (Countdown == 0) Coefficients[i] = Targets[i];
                    else Coefficients[i] += Increments[i];
                }
            }
        }
    }

    // returns moved tap index, or -1
    inline int StepMover() {
        if (Phase == Idle) {
            if (PendingTaps == 0) return -1;
            while ((PendingTaps & (1u << MoverTap)) == 0) MoverTap = MoverTap == MovableTapCount - 1 ? 0 : MoverTap + 1;
            Phase = FadingOut;
            MoverCount = FadeSamples;
        }
        const size_t Gain = TapGain[MoverTap];
        if (Phase == FadingOut) {
            Coefficients[Gain] *= FadeFactor;
            const int32_t Remaining = MoverCount;
            --MoverCount;
            Increments[Gain] = 0.0f;
            if (Remaining == 0) {
                PendingTaps &= ~(1u << MoverTap);
                return (int)MoverTap;
            }
            return -1;
        }
        const float Faded = FadeFactor * Coefficients[Gain];
        Increments[Gain] = 0.0f;
        Coefficients[Gain] = Faded + MoverStep;
        const int32_t Remaining = MoverCount;
        --MoverCount;
        if (Remaining == 0) {
            ArmSmoothing();
            Phase = Idle;
            if (PendingTaps == 0) MoverTap = 0;
            else if (++MoverTap == MovableTapCount) MoverTap = 0;
        }
        return -1;
    }

    void CompleteMove(size_t TapIdx) {
        const size_t Gain = TapGain[TapIdx];
        Coefficients[Gain] = 0.0f;
        MoverStep = (1.0f - FadeFactor) * Targets[Gain];
        Phase = FadingIn;
        MoverCount = FadeSamples;
        Increments[Gain] = 0.0f;
    }

    void Reset() {
        const float Seed = bitsf(StateSeedBits);
        std::fill(Memory.begin(), Memory.end(), bitsf(SilenceFloorBits));
        WritePointer = 0;
        ModulationIndex = (int32_t)ModulationBase;
        ModulationFraction = 0.0f;
        Oscillator = {1.0f, 1.0f};
        InputLowPass = {Seed, Seed};
        BlockerInput = BlockerOutput = Seed;
        LineFilter.fill(Seed);
        LineLowPassState.fill(Seed);
    }

    inline std::array<float, OutputCount> Process(float Input) {
        const auto Co = Coefficients;
        const float InputPole = Co[C::InputLowPass];
        InputLowPass[0] = Input * Co[C::InputGain] * (1.0f - InputPole) + InputPole * InputLowPass[0];
        InputLowPass[1] = (1.0f - InputPole) * InputLowPass[0] + InputPole * InputLowPass[1];
        const float Scaled = InputLowPass[1] * Co[C::InputScale];
        const float PreviousBlockerInput = BlockerInput;
        BlockerInput = -0.0f - Scaled;
        BlockerOutput = Co[C::BlockerFeedback] * BlockerOutput + Scaled + PreviousBlockerInput;
        WriteBase(T::Input, BlockerOutput * InputLineScale);

        const float Reflections = ReadTap(T::Input) * Co[C::ReflectionsGain] * ReflectionsScale;
        WriteBase(T::EarlySource, Reflections);

        std::array<float, OutputCount> Out{};
        const float AllPass = EarlyCoefficient;
        for (size_t ch = 0; ch < OutputCount; ++ch) {
            const float Source = ch == 0 ? Reflections : ReadTap(T::EarlySource + ch - 1);
            const float Driven = Source * Co[C::EarlyGain + ch];
            const float O = ReadTap(T::EarlyAllPass + ch) * Co[C::EarlyAllPassRead + ch] + Driven * AllPass;
            WriteBase(T::EarlyAllPass + ch, Driven - O * AllPass);
            Out[ch] = O;
        }

        const float LateFeed = ReadTap(T::LateFeed) * Co[C::LateFeedGain];
        WriteBase(T::FeedLine, LateFeed);

        const float OscRate = Co[C::OscillatorRate];
        Oscillator[0] = ClampUnit(OscRate * Oscillator[1] + Oscillator[0]);
        Oscillator[1] = ClampUnit(Oscillator[1] - Oscillator[0] * OscRate);

        const float FeedGain = Co[C::FeedTapGain];
        const float FeedTaps[4] = {ReadTap(T::FeedLine), ReadTap(T::FeedLine + 1), ReadTap(T::FeedLine + 2), ReadTap(T::FeedLine + 3)};
        const float RawTaps[3] = {ReadTap(T::FeedLine + 4), ReadTap(T::FeedLine + 5), ReadTap(T::FeedLine + 6)};

        float Lines[LineCount];
        for (size_t L = 0; L < 6; ++L) {
            const float In = ReadTap(T::Line + L) * Co[C::LineAttenuation + L];
            const float Injected = (L % 2 == 0) ? RawTaps[L / 2] : FeedGain * FeedTaps[L / 2];
            Lines[L] = LineStage(L, In, Injected);
        }

        const float ModulatedSource = ReadTap(T::Line + 6) * Co[C::LineAttenuation + 6];
        WriteBase(T::Modulated, ModulatedSource);
        const int32_t ModulatedBase = (int32_t)((uint32_t)Taps[T::Modulated].Base - (uint32_t)ModulationIndex);
        const float Near = Memory[AddressOf(ModulatedBase)];
        const float Far = Memory[AddressOf((int32_t)((uint32_t)ModulatedBase - 1u))];
        const float Interpolated = (1.0f - ModulationFraction) * Near + ModulationFraction * Far;
        Lines[6] = LineStage(6, Interpolated, LateFeed);

        const float LastInput = ReadTap(T::Line + 7) * Co[C::LineAttenuation + 7];
        Lines[7] = LineStage(7, LastInput, FeedGain * FeedTaps[3]);

        const float Rot = Co[C::Rotation];
        {
            const float SeventhPair = Lines[7] * Rot + Lines[6];
            const float FirstPair = Rot * Lines[1] + Lines[0];
            const float SecondPair = Rot * Lines[3] + Lines[2];
            const float ThirdPair = Rot * Lines[5] + Lines[4];
            const float LowerSum = Rot * SecondPair + FirstPair;
            const float LowerDifference = SecondPair - Rot * FirstPair;
            const float UpperSum = Rot * SeventhPair + ThirdPair;
            const float UpperDifference = SeventhPair - Rot * ThirdPair;
            const float Top = Rot * UpperSum + LowerSum;
            WriteBase(T::Line + 1, Top);
            Out[0] += Top * Co[C::LateOutputGain];
            const float Second = UpperSum - LowerSum * Rot;
            WriteBase(T::Line + 5, Second);
            WriteBase(T::RearDelay, Second);
            Out[2] += ReadTap(T::RearDelay) * Co[C::LateOutputGain + 2];
            const float Third = UpperDifference * Rot + LowerDifference;
            WriteBase(T::Line + 3, Third);
            Out[1] += Third * Co[C::LateOutputGain + 1];
            const float Fourth = UpperDifference - LowerDifference * Rot;
            WriteBase(T::Line + 7, Fourth);
            WriteBase(T::RearDelay + 1, Fourth);
            Out[3] += ReadTap(T::RearDelay + 1) * Co[C::LateOutputGain + 3];
        }
        {
            const float SecondPair = Lines[3] - Rot * Lines[2];
            const float FirstPair = Lines[1] - Rot * Lines[0];
            const float SeventhPair = Lines[7] - Rot * Lines[6];
            const float LowerSum = Rot * SecondPair + FirstPair;
            const float ThirdPair = Lines[5] - Rot * Lines[4];
            const float UpperSum = Rot * SeventhPair + ThirdPair;
            const float LowerDifference = SecondPair - Rot * FirstPair;
            WriteBase(T::Line + 2, Rot * UpperSum + LowerSum);
            const float UpperDifference = SeventhPair - Rot * ThirdPair;
            WriteBase(T::Line + 6, UpperSum - LowerSum * Rot);
            WriteBase(T::Line + 4, UpperDifference * Rot + LowerDifference);
            WriteBase(T::Line, UpperDifference - LowerDifference * Rot);
        }

        const float Position = Oscillator[0] * Co[C::ModulationDepth] + ModulationBase;
        ModulationIndex = (int32_t)Position;
        ModulationFraction = Position - (float)ModulationIndex;
        ++WritePointer;
        return Out;
    }

private:
    static constexpr uint32_t DelayMask = (uint32_t)DelayLength - 1;
    static constexpr uint32_t SilenceFloorBits = 0x302bcc77;
    static constexpr float SilenceReplacement = 1.0e-7f;
    static constexpr uint32_t StateSeedBits = 0x2edbe6ff;
    static constexpr float InputLineScale = 1.5f;
    static constexpr float ReflectionsScale = 4.0f;
    static constexpr float SmoothingThreshold = 2.0e-6f;
    static constexpr float SmoothingStep = 0.00390625f;
    static constexpr int32_t SmoothingSamples = 0x100;
    static constexpr size_t TapGain[MovableTapCount] = {44, 43, 51, 52, 53, 58, 59, 60, 61, 11, 12, 13, 14, 15, 16, 17, 18};
    enum MoverPhase { Idle, FadingOut, FadingIn };

    std::vector<float> Memory;
    uint32_t WritePointer = 0;
    int32_t ModulationIndex = 0;
    float ModulationFraction = 0.0f;
    std::array<float, 2> Oscillator{1.0f, 1.0f};
    std::array<float, 2> InputLowPass{};
    float BlockerInput = 0.0f, BlockerOutput = 0.0f;
    std::array<float, LineCount> LineFilter{}, LineLowPassState{};
    std::array<float, CoefficientCount> Increments{};
    int32_t Countdown = 0;
    MoverPhase Phase = Idle;
    size_t MoverTap = 0;
    int32_t MoverCount = 0;
    float MoverStep = 0.0f;

    static inline float GuardSilence(float v) { return (fbits(v) & 0x7fffffffu) < SilenceFloorBits ? SilenceReplacement : v; }
    static inline float ClampUnit(float v) { return v < 1.0f ? (v <= -1.0f ? -1.0f : v) : 1.0f; }
    inline size_t AddressOf(int32_t Offset) const { return (size_t)((WritePointer + (uint32_t)Offset) & DelayMask); }
    inline float ReadTap(size_t i) const { return Memory[AddressOf(Taps[i].Read)]; }
    inline void WriteBase(size_t i, float v) { Memory[AddressOf(Taps[i].Base)] = v; }

    inline float LineStage(size_t L, float In, float Injected) {
        const size_t F = C::LineFilter + L * 3;
        const float Forward = Coefficients[F], Direct = Coefficients[F + 1], Feedback = Coefficients[F + 2];
        const float Inner = LineFilter[L] - Feedback * In;
        LineFilter[L] = GuardSilence(Feedback * Inner + In);
        const float LowPass = Coefficients[C::LineLowPass + L];
        const float O = GuardSilence((Inner * Forward + Injected + In * Direct) * (1.0f - LowPass) + LowPass * LineLowPassState[L]);
        LineLowPassState[L] = O;
        return O;
    }
};

// ---------------------------------------------------------------- rate bridge (only engaged above 48 kHz)

constexpr size_t MaximumDivider = 8;
constexpr size_t BridgeTapsPerPhase = 32;
constexpr size_t BridgeLongest = BridgeTapsPerPhase * MaximumDivider;
constexpr double BridgeBeta = 8.0;

class RateBridge {
public:
    explicit RateBridge(size_t divider = 1) { init(divider); }

    void init(size_t divider) {
        Divider = divider < 1 ? 1 : (divider > MaximumDivider ? MaximumDivider : divider);
        const size_t Length = BridgeTapsPerPhase * Divider;
        const double Middle = (double)(Length - 1) / 2.0;
        auto Bessel = [](double v) {
            double Sum = 1.0, Term = 1.0;
            for (int s = 1; s < 40; ++s) { Term *= (v / (2.0 * s)) * (v / (2.0 * s)); Sum += Term; }
            return Sum;
        };
        double Wide[BridgeLongest] = {};
        double Total = 0.0;
        for (size_t i = 0; i < Length; ++i) {
            const double Offset = (double)i - Middle;
            const double Angle = 3.141592653589793 * Offset / (double)Divider;
            const double Ideal = std::fabs(Angle) < 1.0e-12 ? 1.0 : std::sin(Angle) / Angle;
            const double Across = Offset / Middle;
            const double In = 1.0 - Across * Across;
            Wide[i] = Ideal * Bessel(BridgeBeta * std::sqrt(In > 0.0 ? In : 0.0)) / Bessel(BridgeBeta);
            Total += Wide[i];
        }
        Taps.fill(0.0f);
        for (size_t i = 0; i < Length; ++i) Taps[i] = (float)(Wide[i] / Total);
        Inputs.fill(0.0f);
        InputPosition = 0;
        Phase = 0;
        for (auto& o : Outputs) o.fill(0.0f);
        OutputPosition = 0;
    }

    inline bool Decimate(float Input, float& Out) {
        const size_t Length = BridgeTapsPerPhase * Divider;
        InputPosition = (InputPosition + 1) % Length;
        Inputs[InputPosition] = Input;
        if (++Phase < Divider) return false;
        Phase = 0;
        float Sum = 0.0f;
        for (size_t i = 0; i < Length; ++i) Sum += Taps[i] * Inputs[(InputPosition + Length - i) % Length];
        Out = Sum;
        return true;
    }

    inline std::array<float, 4> Interpolate(const std::array<float, 4>* Fresh) {
        if (Fresh) {
            OutputPosition = (OutputPosition + 1) % BridgeTapsPerPhase;
            for (size_t ch = 0; ch < 4; ++ch) Outputs[ch][OutputPosition] = (*Fresh)[ch];
        }
        std::array<float, 4> Heard{};
        for (size_t s = 0; s < BridgeTapsPerPhase; ++s) {
            const float W = Taps[Phase + s * Divider] * (float)Divider;
            const size_t P = (OutputPosition + BridgeTapsPerPhase - s) % BridgeTapsPerPhase;
            for (size_t ch = 0; ch < 4; ++ch) Heard[ch] += W * Outputs[ch][P];
        }
        return Heard;
    }

private:
    size_t Divider = 1;
    std::array<float, BridgeLongest> Taps{}, Inputs{};
    size_t InputPosition = 0, Phase = 0;
    std::array<std::array<float, BridgeTapsPerPhase>, 4> Outputs{};
    size_t OutputPosition = 0;
};

// ---------------------------------------------------------------- environment reverb

class EnvironmentReverb {
public:
    static constexpr float SendScale = 3.0f;
    static constexpr float MaximumAmount = 100.0f;

    EnvironmentReverb() : Amount((float)Presets[DefaultPreset].DefaultAmount), AppliedSettings(Presets[DefaultPreset].Settings) {}

    void setPreset(int i) { PresetIndex = i < 0 ? 0 : ((size_t)i >= PresetCount ? PresetCount - 1 : (size_t)i); }
    void setAmount(float a) { if (std::isfinite(a)) Amount = clampf(a, 0.0f, MaximumAmount); }
    void suspend() { Applied = false; SendPrimed = false; }
    double decayTime() const { return Presets[PresetIndex].Settings.DecayTime; }

    static size_t DividerFor(float rate) {
        const size_t d = (size_t)std::ceil(rate / MaximumSampleRate);
        return d < 1 ? 1 : (d > MaximumDivider ? MaximumDivider : d);
    }
    static float SendFor(float a) { return a < 1.0f ? 0.0f : (a / MaximumAmount) * SendScale; }
    static float InputFor(float Mixed, float Send, float TargetSend) {
        const float Fed = Mixed * Send;
        return ((TargetSend == 0.0f && Send < 1.0e-4f) || !(std::fabs(Fed) >= SilentInput())) ? SilentInput() : Fed;
    }

    // exact plugin behaviour: interleaved buffer, wet summed into every channel pair and hard-clipped at full scale
    void process(float* data, int frames, int channels, float rate) {
        if (channels <= 0 || !(rate > 0.0f)) return;
        begin(rate);
        const float Smoothing = std::exp(-1.0f / (SendSmoothingSeconds * rate));
        const float Target = SendFor(Amount);
        if (!SendPrimed) { Send = Target; SendPrimed = true; }
        for (int f = 0; f < frames; ++f) {
            float* Frame = data + (size_t)f * channels;
            Send = Target + (Send - Target) * Smoothing;
            float Mixed;
            if (channels == 1) Mixed = Frame[0];
            else { Mixed = (Frame[0] + Frame[1]) * 0.5f; }
            const auto Wet = step(InputFor(Mixed, Send, Target));
            if (std::fabs(Wet[0]) < InaudibleOutput && std::fabs(Wet[1]) < InaudibleOutput) continue;
            if (channels >= 2) {
                Frame[0] = clampf(Frame[0] + Wet[0], -1.0f, 1.0f);
                Frame[1] = clampf(Frame[1] + Wet[1], -1.0f, 1.0f);
            } else {
                Frame[0] = clampf(Frame[0] + (Wet[0] + Wet[1]) * 0.5f, -1.0f, 1.0f);
            }
        }
    }

    // same engine and send law, wet only: in2 == nullptr means a mono source fed to both sides
    void processWet(const float* in, const float* in2, float* wl, float* wr, int frames, float rate) {
        if (frames <= 0) return;
        if (!(rate > 0.0f)) { std::fill(wl, wl + frames, 0.0f); std::fill(wr, wr + frames, 0.0f); return; }
        begin(rate);
        const float Smoothing = std::exp(-1.0f / (SendSmoothingSeconds * rate));
        const float Target = SendFor(Amount);
        if (!SendPrimed) { Send = Target; SendPrimed = true; }
        for (int i = 0; i < frames; ++i) {
            Send = Target + (Send - Target) * Smoothing;
            const float Mixed = in2 ? (in[i] + in2[i]) * 0.5f : in[i];
            const auto Wet = step(InputFor(Mixed, Send, Target));
            if (std::fabs(Wet[0]) < InaudibleOutput && std::fabs(Wet[1]) < InaudibleOutput) { wl[i] = wr[i] = 0.0f; continue; }
            wl[i] = Wet[0];
            wr[i] = Wet[1];
        }
    }

private:
    static constexpr float SendSmoothingSeconds = 0.005f;
    static constexpr float InaudibleOutput = 1.0e-6f;
    static constexpr float FadeSecondsPerSample = 0.010000001f;
    static constexpr double FadeDepthOctaves = -15.0;
    static float SilentInput() { return bitsf(0x302bcc77); }

    Core Engine;
    size_t PresetIndex = DefaultPreset;
    float Amount;
    bool Applied = false;
    Plan Active;
    Parameters AppliedSettings;
    float AppliedRate = 0.0f;
    std::array<float, TapCount> CurrentTimes{};
    size_t Divider = 1;
    float Send = 0.0f;
    bool SendPrimed = false;
    RateBridge Bridge;

    const Parameters& Wanted() const { return Presets[PresetIndex].Settings; }

    std::array<float, LineCount> Playing() const {
        std::array<float, LineCount> p{};
        for (size_t i = 0; i < LineCount; ++i) p[i] = CurrentTimes[FirstLineTap + i];
        return p;
    }

    void begin(float rate) {
        if (!Applied || AppliedRate != rate) start(rate);
        else if (AppliedSettings != Wanted()) retune();
    }

    void start(float rate) {
        Divider = DividerFor(rate);
        const float EngineRate = rate / (float)Divider;
        const Parameters W = Wanted();
        Plan Fresh = PlanFor(W, EngineRate, nullptr);
        Engine.Targets = Fresh.Coefficients;
        Engine.SnapToTargets();
        CurrentTimes = Fresh.Times;
        for (size_t i = 0; i < TapCount; ++i) Engine.Taps[i] = TapFor(i, Fresh.Times[i], Fresh.SampleRate);
        Engine.ModulationBase = Fresh.ModulationBase;
        Engine.FadeSamples = (int32_t)(Fresh.SampleRate * FadeSecondsPerSample);
        Engine.FadeFactor = (float)std::pow(2.0, FadeDepthOctaves / (double)Engine.FadeSamples);
        Engine.Reset();
        Active = Fresh;
        Applied = true;
        AppliedSettings = W;
        AppliedRate = rate;
        Bridge.init(Divider);
    }

    void retune() {
        const auto P = Playing();
        const Parameters W = Wanted();
        Plan Next = PlanFor(W, Active.SampleRate, &P);
        Engine.PendingTaps |= ChangedTaps(AppliedSettings, W);
        Engine.Targets = Next.Coefficients;
        Engine.ArmSmoothing();
        Active = Next;
        AppliedSettings = W;
    }

    inline std::array<float, OutputCount> runEngine(float Input) {
        const auto Out = Engine.Process(Input);
        const int Moved = Engine.StepMover();
        if (Moved >= 0) {
            const size_t m = (size_t)Moved;
            CurrentTimes[m] = Active.Times[m];
            Engine.Taps[m] = TapFor(m, Active.Times[m], Active.SampleRate);
            if (m >= FirstLineTap) {
                auto Targets = Engine.Targets;
                ApplyDecay(Active, Playing(), Targets);
                Engine.Targets = Targets;
                Engine.ArmSmoothing();
            }
            Engine.CompleteMove(m);
        }
        Engine.StepSmoothing();
        return Out;
    }

    inline std::array<float, OutputCount> step(float Input) {
        if (Divider == 1) return runEngine(Input);
        float Slowed;
        if (Bridge.Decimate(Input, Slowed)) {
            const auto Fresh = runEngine(Slowed);
            return Bridge.Interpolate(&Fresh);
        }
        return Bridge.Interpolate(nullptr);
    }
};

}
