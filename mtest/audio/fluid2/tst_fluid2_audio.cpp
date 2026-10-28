// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/audiobuffer.h"

#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition, const char* message)
      {
      if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
            }
      }

// Stereo sentinels stand in for a second synth already mixed into the host.
// The renderer deliberately adds, matching FluidSynth's process API contract.
static void check(unsigned groups, unsigned frames, unsigned failCall = 0)
      {
      Ms::Fluid2AudioBuffer buffer(groups);
      std::vector<float> output(frames * 2 + 2, -7.0f);
      std::vector<float> expected = output;
      unsigned position = 0;
      unsigned calls = 0;
      float** firstPlanes = nullptr;
      std::vector<float*> firstPointers;
      auto render = [&](int count, int outputs, float** planes) {
            require(count > 0 && count <= Ms::Fluid2AudioBuffer::BlockFrames, "invalid block size");
            require(outputs == int(groups * 2), "wrong planar output count");
            ++calls;
            if (!firstPlanes) {
                  firstPlanes = planes;
                  firstPointers.assign(planes, planes + outputs);
                  }
            else {
                  require(planes == firstPlanes, "output pointer array moved during rendering");
                  for (int plane = 0; plane < outputs; ++plane)
                        require(planes[plane] == firstPointers[plane], "audio plane moved during rendering");
                  }
            for (int plane = 0; plane < outputs; ++plane) {
                  for (int i = 0; i < count; ++i) {
                        require(planes[plane][i] == 0.0f, "stale samples accumulated across blocks");
                        planes[plane][i] += float((plane + 1) * 0.125 + (position + unsigned(i)) * 0.25);
                        }
                  }
            if (calls != failCall) {
                  for (unsigned group = 0; group < groups; ++group) {
                        for (int i = 0; i < count; ++i) {
                              expected[1 + (position + unsigned(i)) * 2] += planes[group * 2][i];
                              expected[2 + (position + unsigned(i)) * 2] += planes[group * 2 + 1][i];
                              }
                        }
                  }
            position += unsigned(count);
            return calls != failCall;
            };
      const bool result = buffer.process(frames, output.data() + 1, render);
      require(result == (failCall == 0), "wrong render failure result");
      require(position == frames, "rendered wrong frame count");
      require(calls == (frames + Ms::Fluid2AudioBuffer::BlockFrames - 1) / Ms::Fluid2AudioBuffer::BlockFrames,
              "wrong block count");
      require(output == expected, "stereo mix, output bounds, or failed-block isolation differs");

      // Reuse the same buffers after the long/failing render. No old samples
      // or frames outside a smaller request may reach the host.
      float shortOutput[] = { 9.0f, 10.0f, 11.0f, 12.0f };
      buffer.process(1, shortOutput + 1, [&](int count, int outputs, float** planes) {
            require(count == 1, "wrong short block size");
            for (int p = 0; p < outputs; ++p) require(planes[p][0] == 0.0f, "stale reused samples");
            planes[0][0] += 1.0f;
            planes[1][0] -= 2.0f;
            return true;
            });
      require(shortOutput[0] == 9.0f && shortOutput[1] == 11.0f
              && shortOutput[2] == 9.0f && shortOutput[3] == 12.0f, "reused stereo/bounds mismatch");
      }

static void checkSparseReuse()
      {
      Ms::Fluid2AudioBuffer buffer(128);
      unsigned pass = 0;
      for (unsigned frames : {512u, 1u, 512u, 63u, 512u, 1u, 512u}) {
            std::vector<float> output(frames * 2, 0.125f), expected(output);
            const bool fail = pass == 3;
            const bool result = buffer.process(frames, output.data(), [&](int count, int outputs, float** planes) {
                  for (int p = 0; p < outputs; ++p)
                        for (int i = 0; i < count; ++i)
                              require(planes[p][i] == 0.0f, "sparse block retained a long/failed tail");
                  if (pass == 0 || fail) {
                        planes[255][count - 1] = -0.25f;
                        if (!fail) expected[(count - 1) * 2 + 1] -= 0.25f;
                        }
                  if (pass == 1) { planes[0][0] = 0.5f; expected[0] += 0.5f; }
                  return !fail;
                  });
            require(result == !fail && output == expected, "sparse reuse/failure changed PCM");
            ++pass;
            }
      }

int main()
      {
      require(!Ms::Fluid2AudioBuffer::validGroupCount(0), "zero groups accepted");
      require(!Ms::Fluid2AudioBuffer::validGroupCount(129), "too many groups accepted");
      for (unsigned groups : { 1u, 2u, 16u, 128u }) {
            for (unsigned frames : { 0u, 1u, 63u, 64u, 511u, 512u, 513u, 1025u, 4096u, 8193u })
                  check(groups, frames);
            check(groups, 1025, 1);
            check(groups, 1025, 2);
            check(groups, 1025, 3);
            }
      Ms::Fluid2AudioBuffer buffer(1);
      bool called = false;
      auto unused = [&](int, int, float**) { called = true; return true; };
      require(buffer.process(0, nullptr, unused), "empty render failed");
      require(!buffer.process(1, nullptr, unused), "null output accepted");
      require(!called, "render called for empty/null output");
      checkSparseReuse();
      std::cout << "Fluid2 planar buffer: block boundaries, additive stereo, zeroing, reuse and failures passed\n";
      }
