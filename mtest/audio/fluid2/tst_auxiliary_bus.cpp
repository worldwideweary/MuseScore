// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/auxiliarybus.h"
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool test, const char* message)
      {
      if (!test) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
      }

int main()
      {
      Ms::AuxiliaryBus<4> bus;
      std::vector<float> dry(10, 7.0f);
      unsigned resets = 0, effects = 0, renders = 0;
      float tail = 0.0f;
      bool send = false;
      auto reset = [&]() { ++resets; tail = 0.0f; };
      auto render = [&](float* aux) {
            ++renders;
            require(aux != nullptr, "missing auxiliary bus");
            for (unsigned i = 0; i < 8; ++i) {
                  require(aux[i] == 0.0f, "bus not cleared between renders");
                  dry[i] += 1.0f;  // original dry synth mix, before the return
                  }
            if (send) aux[0] += 2.0f;
            };
      auto effect = [&](unsigned frames, float* input, float* wet) {
            ++effects;
            require(frames == 4, "wrong frame count");
            for (unsigned i = 0; i < frames * 2; ++i) {
                  require(wet[i] == 0.0f, "return scratch not cleared");
                  wet[i] += tail;
                  tail = input[i];  // deterministic one-sample delay
                  }
            };
      require(!bus.process(5, dry.data(), render, reset, effect), "accepted oversized render");
      require(!bus.process(0, dry.data(), render, reset, effect), "accepted empty render");
      require(!bus.process(4, nullptr, render, reset, effect), "accepted null dry buffer");
      require(!renders && !resets && !effects, "invalid request consumed time/reset");
      require(bus.process(4, dry.data(), render, reset, effect), "silent render rejected");
      require(resets == 1 && effects == 0 && dry[0] == 8.0f, "zero send changed dry audio");
      send = true;
      bus.process(4, dry.data(), render, reset, effect);
      require(effects == 1 && dry[0] == 9.0f && dry[1] == 11.0f, "wet return replaced dry mix");
      require(dry[8] == 7.0f && dry[9] == 7.0f, "render exceeded host buffer");
      send = false;
      bus.process(4, dry.data(), render, reset, effect);
      require(effects == 2, "silent input skipped an active tail");
      bus.requestReset();
      bus.requestReset();
      bus.process(4, dry.data(), render, reset, effect);
      require(resets == 2 && effects == 2 && tail == 0.0f, "reset retained return history");
      send = true;
      bus.process(4, dry.data(), render, reset, effect);
      require(effects == 3, "send did not restart after reset");
      std::cout << "Auxiliary bus: dry neutrality, additive return, tail and deferred reset checks passed\n";
      }
