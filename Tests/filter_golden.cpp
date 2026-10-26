// filter_golden — byte-level parity of nassau-dsp's filters and shaper.
//
//     filter_golden generate <path>   write the reference renders
//     filter_golden verify   <path>   render again and compare
//
// The battery drives every structure this library exports to its consumers
// (LadderFilter, SvfFilter, HpfCascade, shapeTriodeK) with one fixed,
// seeded input: white noise plus a slow saw, the cutoff swept per control
// block and ramped per sample through advanceCoeff(), as nassau-analogue's
// SynthCore drives them. Regenerating the fixture is a deliberate act
// (a reviewed commit), never something a test does.
//
// Tolerance: 0 on the platform the fixture was generated on is what a pure
// move should show; across compilers and C libraries std::tan may differ in
// its last bit, so verify accepts 1e-6 absolute (about 10 ULP of float32 at
// full scale, nassau-analogue's own golden tolerance) and prints the maximum
// error so a reader sees how far inside it the render is.

#include "synth_filter.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kFs = 48000.0;
constexpr int kFrames = 24000;  // 0.5 s per case

std::vector<float> makeInput() {
  Xorshift32 rng(0xC0FFEEu);
  std::vector<float> in(kFrames);
  double saw = 0.0;
  for (int i = 0; i < kFrames; ++i) {
    saw += 110.0 / kFs;
    if (saw >= 1.0) saw -= 1.0;
    in[static_cast<size_t>(i)] = static_cast<float>(0.3 * rng.nextBipolar() + 0.4 * (2.0 * saw - 1.0));
  }
  return in;
}

// The cutoff at control block b: an exponential sweep 80 Hz -> 12 kHz.
double cutoffAt(int b, int blocks) {
  const double t = static_cast<double>(b) / static_cast<double>(blocks - 1);
  return 80.0 * std::pow(12000.0 / 80.0, t);
}

enum class Kind { Ladder, Svf, Hpf2, Hpf4, Shaper };

struct Case {
  Kind kind;
  double res;    // resonance % (low-passes) or shaper k
};

void renderCase(const Case& c, const std::vector<float>& in, std::vector<float>& out) {
  LadderFilter lad;
  SvfFilter svf;
  HpfCascade hpf;
  lad.reset();
  svf.reset();
  hpf.reset();
  const int blocks = kFrames / kControlBlock;
  for (int b = 0; b < blocks; ++b) {
    const double fc = cutoffAt(b, blocks);
    // Per-sample ramp of g toward this block's target, as SynthCore does.
    const double gTarget = std::tan(kAmpPi * fc / kFs);
    double gStart = 0.0;
    switch (c.kind) {
      case Kind::Ladder: gStart = static_cast<double>(lad.g); lad.setControlRate(fc, kFs, c.res); break;
      case Kind::Svf:    gStart = static_cast<double>(svf.g); svf.setControlRate(fc, kFs, c.res); break;
      case Kind::Hpf2:
      case Kind::Hpf4:   gStart = static_cast<double>(hpf.g); hpf.setControlRate(fc, kFs); break;
      case Kind::Shaper: break;
    }
    if (b == 0) gStart = gTarget;
    for (int i = 0; i < kControlBlock; ++i) {
      const int n = b * kControlBlock + i;
      const double gNow = gStart + (gTarget - gStart) * static_cast<double>(i + 1) / kControlBlock;
      const nassau_real x = static_cast<nassau_real>(in[static_cast<size_t>(n)]);
      double y = 0.0;
      switch (c.kind) {
        case Kind::Ladder: lad.advanceCoeff(static_cast<nassau_real>(gNow)); y = lad.process(x); break;
        case Kind::Svf:    svf.advanceCoeff(static_cast<nassau_real>(gNow)); y = svf.process(x); break;
        case Kind::Hpf2:   hpf.advanceCoeff(static_cast<nassau_real>(gNow)); y = hpf.process(x, 2); break;
        case Kind::Hpf4:   hpf.advanceCoeff(static_cast<nassau_real>(gNow)); y = hpf.process(x, 4); break;
        case Kind::Shaper: y = shapeTriodeK(3.0 * static_cast<double>(x), c.res); break;
      }
      out.push_back(static_cast<float>(y));
    }
  }
}

std::vector<float> renderBattery() {
  const Case cases[] = {
      {Kind::Ladder, 0.0}, {Kind::Ladder, 50.0}, {Kind::Ladder, 100.0},
      {Kind::Svf, 0.0},    {Kind::Svf, 50.0},    {Kind::Svf, 100.0},
      {Kind::Hpf2, 0.0},   {Kind::Hpf4, 0.0},
      {Kind::Shaper, 0.0}, {Kind::Shaper, 0.5},  {Kind::Shaper, 2.0},
  };
  const std::vector<float> in = makeInput();
  std::vector<float> out;
  out.reserve(sizeof(cases) / sizeof(cases[0]) * kFrames);
  for (const Case& c : cases) renderCase(c, in, out);
  return out;
}

bool writeFile(const std::string& path, const std::vector<float>& v) {
  std::ofstream f(path, std::ios::binary);
  const uint32_t n = static_cast<uint32_t>(v.size());
  f.write(reinterpret_cast<const char*>(&n), sizeof n);
  f.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(float)));
  return static_cast<bool>(f);
}

bool readFile(const std::string& path, std::vector<float>& v) {
  std::ifstream f(path, std::ios::binary);
  uint32_t n = 0;
  if (!f.read(reinterpret_cast<char*>(&n), sizeof n)) return false;
  v.resize(n);
  return static_cast<bool>(f.read(reinterpret_cast<char*>(v.data()), static_cast<std::streamsize>(n * sizeof(float))));
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: filter_golden generate|verify <path>\n";
    return 2;
  }
  const std::string mode = argv[1], path = argv[2];
  const std::vector<float> now = renderBattery();

  if (mode == "generate") {
    if (!writeFile(path, now)) {
      std::cerr << "cannot write " << path << "\n";
      return 1;
    }
    std::cout << "Generated filter golden: " << path << " (" << now.size() << " values)\n";
    return 0;
  }
  if (mode != "verify") {
    std::cerr << "unknown mode " << mode << "\n";
    return 2;
  }
  std::vector<float> golden;
  if (!readFile(path, golden)) {
    std::cerr << "FAIL: cannot read golden " << path << "\n";
    return 1;
  }
  if (golden.size() != now.size()) {
    std::cerr << "FAIL: golden has " << golden.size() << " values, render has " << now.size() << "\n";
    return 1;
  }
  double maxErr = 0.0;
  size_t maxIdx = 0, exact = 0;
  for (size_t i = 0; i < now.size(); ++i) {
    const double e = std::fabs(static_cast<double>(now[i]) - static_cast<double>(golden[i]));
    if (std::memcmp(&now[i], &golden[i], sizeof(float)) == 0) ++exact;
    if (e > maxErr || std::isnan(e)) {
      maxErr = std::isnan(e) ? 1e9 : e;
      maxIdx = i;
    }
  }
  const double kTol = 1e-6;
  std::printf("Filter golden: %zu values, %zu bit-exact, max abs error %.3e at %zu (tol %.1e)\n", now.size(), exact,
              maxErr, maxIdx, kTol);
  if (maxErr > kTol) {
    std::printf("FAIL: render differs from golden beyond tolerance\n");
    return 1;
  }
  std::printf("PASS\n");
  return 0;
}
