// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/audiobuffer.h"
#include "audio/midi/fluid2/sendmix.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

static void require(bool value, const char* message)
      { if (!value) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); } }

int main()
      {
      Ms::Fluid2SendMix sends;
      require(!sends.hasRoutedGain(0) && !sends.hasRoutedGain(1) && !sends.hasRoutedGain(2),
              "empty/invalid bank has a routed gain");
      sends.setGain(7, 1.0f);
      require(!sends.hasRoutedGain(0) && !sends.hasRoutedGain(1), "unowned gain enabled bank output planes");
      sends.setSource(255, 7);
      require(!sends.hasRoutedGain(0) && sends.hasRoutedGain(1), "high-bank gain aliased low bank");
      sends.setGain(7, 0.0f);
      require(!sends.hasRoutedGain(1), "zero gain retained send output planes");
      sends.setGain(7, 1.0f);
      sends.clearSource(255, 128);
      require(!sends.hasRoutedGain(1), "cleared owner retained send output planes");
      sends.clear();
      for (int source = 0; source < 256; ++source)
            require(sends.gain(source) == 0.0f, "nonzero default send");
      require(sends.setSource(0, 0) && sends.setSource(5, 0)
              && sends.setSource(128, 128) && sends.setSource(255, 0), "source binding failed");
      require(sends.setGain(0, 0.25f) && sends.setGain(128, 0.75f) && sends.setGain(255, 1.0f), "valid gain rejected");
      for (float bad : { -0.1f, 1.1f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
            require(!sends.setGain(0, bad) && sends.gain(0) == 0.25f, "invalid gain changed state");
      require(!sends.setGain(-1, 1) && !sends.setGain(256, 1), "invalid source gain accepted");

      std::vector<std::vector<float>> storage(256, std::vector<float>(3, 0));
      std::vector<float*> planes;
      for (auto& plane : storage) planes.push_back(plane.data());
      std::fill(storage[0].begin(), storage[0].end(), 1);
      std::fill(storage[1].begin(), storage[1].end(), 2);
      std::fill(storage[10].begin(), storage[10].end(), 3);
      std::fill(storage[11].begin(), storage[11].end(), 4);
      float auxiliary[10]; std::fill(auxiliary, auxiliary + 10, 10);
      sends.mix(0, 1, 3, 128, planes.data(), auxiliary);
      for (unsigned i = 1; i < 4; ++i)
            require(auxiliary[i * 2] == 11 && auxiliary[i * 2 + 1] == 11.5f, "low-bank send sum differs");
      for (auto& plane : storage) std::fill(plane.begin(), plane.end(), 0);
      for (unsigned i = 0; i < 3; ++i) {
            storage[0][i] = 5; storage[1][i] = 6; storage[254][i] = 7; storage[255][i] = 8;
            }
      sends.mix(1, 1, 3, 128, planes.data(), auxiliary);
      for (unsigned i = 1; i < 4; ++i)
            require(auxiliary[i * 2] == 16.5f && auxiliary[i * 2 + 1] == 18, "high-bank owner/bend gain differs");
      require(auxiliary[0] == 10 && auxiliary[1] == 10 && auxiliary[8] == 10 && auxiliary[9] == 10, "send wrote outside range");
      require(storage[0][0] == 5 && storage[255][2] == 8, "send modified dry planes");

      require(!sends.setSource(255, 128), "live owner change accepted");
      std::fill(auxiliary, auxiliary + 10, 0);
      sends.mix(1, 0, 3, 128, planes.data(), auxiliary);
      require(auxiliary[0] == 3.75f && auxiliary[1] == 4.5f, "ambiguous lane reached auxiliary");
      sends.clearSources();
      require(sends.gain(0) == 0.25f, "source reset cleared gain");
      std::fill(auxiliary, auxiliary + 10, 0);
      sends.mix(1, 0, 3, 128, planes.data(), auxiliary);
      require(auxiliary[0] == 0 && auxiliary[1] == 0, "unknown owner reached auxiliary");
      require(sends.setSource(255, 0), "reset lane cannot bind");
      sends.mix(2, 0, 3, 128, planes.data(), auxiliary);
      sends.mix(1, 0, 3, 16, planes.data(), auxiliary);
      require(auxiliary[0] == 0 && auxiliary[1] == 0, "invalid output layout contributed audio");
      sends.clearSource(255, 2);
      sends.setSource(255, 0);
      sends.mix(1, 0, 3, 128, planes.data(), auxiliary);
      require(auxiliary[0] == 0 && auxiliary[1] == 0 && auxiliary[2] == 0 && auxiliary[3] == 0
              && auxiliary[4] == 1.75f && auxiliary[5] == 2, "reset warmup did not suppress cached samples");
      sends.advance(2);
      sends.mix(1, 0, 1, 128, planes.data(), auxiliary);
      require(auxiliary[0] == 1.75f && auxiliary[1] == 2, "send did not resume after reset warmup");
      sends.clear();
      require(sends.gain(0) == 0 && sends.source(255) < 0, "full reset retained send state");

      // The failed first block writes scratch planes, but must reach neither
      // dry nor auxiliary. The successful second block uses its true offset.
      Ms::Fluid2AudioBuffer buffer(128);
      sends.setSource(0, 0); sends.setGain(0, 0.5f);
      std::vector<float> dry(513 * 2, 2), aux(513 * 2, 3);
      unsigned calls = 0, mixes = 0;
      require(!buffer.process(513, dry.data(), [&](int frames, int, float** data) {
            for (int i = 0; i < frames; ++i) { data[0][i] += 1; data[1][i] += 0.5f; }
            return ++calls > 1;
            }, [&](unsigned start, unsigned frames, unsigned groups, float** data) {
                  ++mixes; sends.mix(0, start, frames, groups, data, aux.data());
            }), "failed block reported success");
      require(calls == 2 && mixes == 1, "failed block reached send callback");
      for (unsigned i = 0; i < 1024; ++i) require(dry[i] == 2 && aux[i] == 3, "failed block leaked samples");
      require(dry[1024] == 3 && dry[1025] == 2.5f && aux[1024] == 3.5f && aux[1025] == 3.25f,
              "last-block dry/send sum differs");
      std::cout << "Fluid2 sends: zero defaults, bank/source gains, bend inheritance, conflicts and failed blocks passed\n";
      }
