// nassau-dsp filter tests (NassauDigital gate X1.2).
//
// What this file proves about Source/DSP/synth_filter.h, measured on real
// signals through the public structs (R6), with no SDK, no Qt and no
// nassau-analogue on the include path:
//
//   * slopes: the ladder falls 24 dB/oct, the SVF 12 dB/oct above cutoff,
//     the HP cascade rises 12/24 dB/oct (2/4 poles) below it;
//   * resonance: at 100 % both low-passes self-oscillate near cutoff, at 0 %
//     an impulse decays to nothing;
//   * the per-sample process() regions contain no division and no
//     transcendental of their own (the shapeTriodeK call in the ladder is the
//     one documented exception, nassau-analogue G11.10);
//   * determinism: reset, the same input, the same bytes.
//
// The deeper battery (G4.1-G4.11) stays in nassau-analogue's own
// Tests/filter_tests.cpp, which keeps running against these very headers
// through its submodule. Byte-level parity is filter_golden.cpp's job.

#include "synth_filter.h"
#include "test_util.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(const std::string& what, bool ok) {
  ++g_checks;
  if (!ok) ++g_failures;
  std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
}

void checkNum(const std::string& what, bool ok, double value) {
  ++g_checks;
  if (!ok) ++g_failures;
  std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "  [measured " << value << "]\n";
}

constexpr double kFs = 48000.0;

// Runs a filter the way its contract says: setControlRate once per
// nassau::kControlBlock samples, process() per sample in between.
struct Ladder {
  LadderFilter f;
  double fc, res;
  int n = 0;
  Ladder(double fc_, double res_) : fc(fc_), res(res_) { f.reset(); }
  float tick(float x) {
    if (n++ % nassau::kControlBlock == 0) f.setControlRate(fc, kFs, res);
    return static_cast<float>(f.process(x));
  }
};

struct Svf {
  SvfFilter f;
  double fc, res;
  int n = 0;
  Svf(double fc_, double res_) : fc(fc_), res(res_) { f.reset(); }
  float tick(float x) {
    if (n++ % nassau::kControlBlock == 0) f.setControlRate(fc, kFs, res);
    return static_cast<float>(f.process(x));
  }
};

struct Hpf {
  HpfCascade f;
  double fc;
  int poles;
  int n = 0;
  Hpf(double fc_, int poles_) : fc(fc_), poles(poles_) { f.reset(); }
  float tick(float x) {
    if (n++ % nassau::kControlBlock == 0) f.setControlRate(fc, kFs);
    return static_cast<float>(f.process(x, poles));
  }
};

template <typename F>
double magOf(F&& makeFilter, double freq) {
  auto filt = makeFilter();
  return magDb(
      [&](const float* in, float* out, int n) {
        for (int i = 0; i < n; ++i) out[i] = filt.tick(in[i]);
      },
      freq, kFs);
}

// An impulse into the filter, then silence; returns the output.
template <typename F>
std::vector<float> impulseResponse(F filt, int frames) {
  std::vector<float> out(static_cast<size_t>(frames));
  for (int i = 0; i < frames; ++i) out[static_cast<size_t>(i)] = filt.tick(i == 0 ? 1.0f : 0.0f);
  return out;
}

std::string readFile(const char* path) {
  std::ifstream f(path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string stripLineComments(const std::string& in) {
  std::string out;
  size_t i = 0;
  while (i < in.size()) {
    if (in[i] == '/' && i + 1 < in.size() && in[i + 1] == '/') {
      while (i < in.size() && in[i] != '\n') ++i;
    } else {
      out += in[i++];
    }
  }
  return out;
}

// Every PER-SAMPLE PROCESS BEGIN/END region of the file, comments stripped.
std::vector<std::string> processRegions(const std::string& src) {
  std::vector<std::string> regions;
  const std::string b = "PER-SAMPLE PROCESS BEGIN", e = "PER-SAMPLE PROCESS END";
  size_t at = 0;
  while ((at = src.find(b, at)) != std::string::npos) {
    const size_t bodyStart = src.find('\n', at);
    const size_t endMark = src.find(e, at);
    if (bodyStart == std::string::npos || endMark == std::string::npos) break;
    const size_t bodyEnd = src.rfind('\n', endMark);
    regions.push_back(stripLineComments(src.substr(bodyStart, bodyEnd - bodyStart)));
    at = endMark;
  }
  return regions;
}

}  // namespace

int main() {
  std::cout << "nassau-dsp filter tests\n";

  std::cout << "\nGroup: slopes above/below cutoff (resonance 0)\n";
  {
    // Measured an octave pair three octaves past a 200 Hz corner, where the
    // asymptote holds and the bilinear transform's warping toward Nyquist
    // has not yet steepened it (4 -> 8 kHz at a 500 Hz corner reads -26.5).
    const double l1 = magOf([] { return Ladder(200.0, 0.0); }, 1600.0);
    const double l2 = magOf([] { return Ladder(200.0, 0.0); }, 3200.0);
    checkNum("ladder (fc 200 Hz): -24 dB/oct +/- 1 between 1.6 and 3.2 kHz", std::fabs((l2 - l1) + 24.0) < 1.0,
             l2 - l1);

    const double s1 = magOf([] { return Svf(200.0, 0.0); }, 1600.0);
    const double s2 = magOf([] { return Svf(200.0, 0.0); }, 3200.0);
    checkNum("SVF (fc 200 Hz): -12 dB/oct +/- 0.5 between 1.6 and 3.2 kHz", std::fabs((s2 - s1) + 12.0) < 0.5,
             s2 - s1);

    const double h2a = magOf([] { return Hpf(4000.0, 2); }, 250.0);
    const double h2b = magOf([] { return Hpf(4000.0, 2); }, 500.0);
    checkNum("HP cascade, 2 poles: +12 dB/oct +/- 1 between 250 and 500 Hz", std::fabs((h2b - h2a) - 12.0) < 1.0,
             h2b - h2a);
    const double h4a = magOf([] { return Hpf(4000.0, 4); }, 250.0);
    const double h4b = magOf([] { return Hpf(4000.0, 4); }, 500.0);
    checkNum("HP cascade, 4 poles: +24 dB/oct +/- 2 between 250 and 500 Hz", std::fabs((h4b - h4a) - 24.0) < 2.0,
             h4b - h4a);

    const double pass = magOf([] { return Ladder(4000.0, 0.0); }, 100.0);
    checkNum("ladder passband (fc 4 kHz) at 100 Hz within 0.5 dB of unity", std::fabs(pass) < 0.5, pass);
  }

  std::cout << "\nGroup: resonance\n";
  {
    const int frames = static_cast<int>(2.0 * kFs);
    const int tailFrom = static_cast<int>(1.5 * kFs);

    const auto lRes = impulseResponse(Ladder(1000.0, 100.0), frames);
    const double lTail = rms(lRes, tailFrom, frames);
    checkNum("ladder at 100 %: still sounding 1.5 s after an impulse (tail RMS > 0.01)", lTail > 0.01, lTail);
    const double lF0 = measuredF0(std::vector<float>(lRes.begin() + tailFrom, lRes.end()), kFs);
    checkNum("ladder at 100 %: oscillates within 15 % of the 1 kHz cutoff", std::fabs(lF0 / 1000.0 - 1.0) < 0.15,
             lF0);

    const auto sRes = impulseResponse(Svf(1000.0, 100.0), frames);
    const double sTail = rms(sRes, tailFrom, frames);
    checkNum("SVF at 100 %: still sounding 1.5 s after an impulse (tail RMS > 0.01)", sTail > 0.01, sTail);

    const auto lDry = impulseResponse(Ladder(1000.0, 0.0), frames);
    const double lDryTail = rms(lDry, tailFrom, frames);
    checkNum("ladder at 0 %: impulse tail RMS below 1e-9", lDryTail < 1e-9, lDryTail);
  }

  std::cout << "\nGroup: determinism (R8)\n";
  {
    Xorshift32 rng;
    std::vector<float> in(48000);
    for (auto& s : in) s = static_cast<float>(rng.nextBipolar() * 0.5);
    auto render = [&] {
      Ladder l(1200.0, 70.0);
      Svf s(800.0, 60.0);
      Hpf h(200.0, 4);
      std::vector<float> out(in.size() * 3);
      for (size_t i = 0; i < in.size(); ++i) {
        out[i * 3 + 0] = l.tick(in[i]);
        out[i * 3 + 1] = s.tick(in[i]);
        out[i * 3 + 2] = h.tick(in[i]);
      }
      return out;
    };
    const auto a = render();
    const auto b = render();
    check("ladder + SVF + HP cascade: two renders are byte-identical",
          std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
  }

  std::cout << "\nGroup: the per-sample process() regions (R12)\n";
  {
#ifdef NASSAU_FILTER_H_PATH
    const std::string src = readFile(NASSAU_FILTER_H_PATH);
    check("synth_filter.h is readable for inspection", !src.empty());
    const auto regions = processRegions(src);
    checkNum("exactly two marked process() regions (ladder, SVF)", regions.size() == 2,
             static_cast<double>(regions.size()));
    const char* forbidden[] = {"/", "tan(", "exp(", "exp2(", "pow(", "log(", "sin(", "cos(", "sqrt("};
    for (size_t r = 0; r < regions.size(); ++r) {
      bool clean = !regions[r].empty();
      for (const char* tok : forbidden) clean = clean && regions[r].find(tok) == std::string::npos;
      check(std::string("region ") + std::to_string(r) + ": no '/' and no transcendental in its own text", clean);
    }
    // The ladder's one division lives in its feedback saturator, by design
    // (nassau-analogue G11.10). Pin that it is exactly one call, so a second
    // cannot slip in under the clean-text check above.
    if (!regions.empty()) {
      size_t calls = 0, at = 0;
      while ((at = regions[0].find("shapeTriodeK(", at)) != std::string::npos) {
        ++calls;
        ++at;
      }
      checkNum("ladder region: exactly one shapeTriodeK call (its documented division)", calls == 1,
               static_cast<double>(calls));
    }
#else
    check("NASSAU_FILTER_H_PATH defined by the build", false);
#endif
  }

  std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
