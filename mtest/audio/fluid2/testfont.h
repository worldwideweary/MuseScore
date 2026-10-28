// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_FLUID2_TESTFONT_H
#define MS_FLUID2_TESTFONT_H

// Generate an original minimal SF2 sine fixture, with melodic and drum presets.
// No installed or copyrighted external SoundFont is needed by the engine test.
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace Fluid2Test {
using Bytes = std::vector<unsigned char>;
struct Modulation {
      int pan = 0;
      int panAmount = -1; // Negative: no authored replacement of the default.
      int pitchSource = -1; // Raw SF2 source for an optional authored octave pitch route.
      int pitchAmountSource = 0;
      };
inline void u16(Bytes& b, unsigned value) { b.push_back(value & 255); b.push_back((value >> 8) & 255); }
inline void u32(Bytes& b, unsigned value) { u16(b, value & 65535); u16(b, value >> 16); }
inline void name(Bytes& b, const std::string& text, unsigned count)
      { for (unsigned i = 0; i < count; ++i) b.push_back(i < text.size() ? text[i] : 0); }
inline void append(Bytes& to, const Bytes& from) { to.insert(to.end(), from.begin(), from.end()); }
inline void chunk(Bytes& to, const std::string& tag, const Bytes& data)
      { name(to, tag, 4); u32(to, unsigned(data.size())); append(to, data); if (data.size() & 1) to.push_back(0); }
inline void generator(Bytes& b, unsigned op, unsigned amount) { u16(b, op); u16(b, amount); }
inline void preset(Bytes& b, const std::string& text, unsigned bank, unsigned bag, unsigned program = 0)
      { name(b, text, 20); u16(b, program); u16(b, bank); u16(b, bag); u32(b, 0); u32(b, 0); u32(b, 0); }
inline void sample(Bytes& b, const std::string& text, unsigned length, bool compressed = false, unsigned root = 69, unsigned loopPadding = 0)
      {
      name(b, text, 20); u32(b, 0); u32(b, length); u32(b, loopPadding); u32(b, compressed ? 44100 : length - loopPadding); u32(b, 44100);
      b.push_back(root); b.push_back(0); u16(b, 0); u16(b, compressed ? 0x11 : 1);
      }
inline bool writeFont(const char* path, const Bytes* compressed = nullptr, bool withPresets = true,
                      int extraProgram = -1, unsigned root = 69, int extraCoarseTune = 0,
                      unsigned extraBank = 0, bool keyPressurePitch = false, const Modulation* modulation = nullptr,
                      unsigned loopPadding = 0, unsigned exclusiveClass = 0)
      {
      if (loopPadding >= 22050 || (compressed && loopPadding)) return false;
      Bytes riff, info, version, label, sdta, samples, pdta, phdr, pbag, pgen, inst, ibag, igen, shdr;
      name(riff, "sfbk", 4);
      name(info, "INFO", 4); u16(version, compressed ? 3 : 2); u16(version, compressed ? 0 : 1); chunk(info, "ifil", version);
      name(label, "37E native sine test", 20); chunk(info, "INAM", label); chunk(riff, "LIST", info);
      name(sdta, "sdta", 4);
      if (compressed) samples = *compressed;
      else {
            for (unsigned i = 0; i < 44100; ++i) {
                  const int value = int(12000.0 * std::sin(6.283185307179586 * 440.0 * i / 44100.0));
                  u16(samples, unsigned(uint16_t(value)));
                  }
            for (unsigned i = 0; i < 46; ++i) u16(samples, 0);
            }
      chunk(sdta, "smpl", samples); chunk(riff, "LIST", sdta);
      name(pdta, "pdta", 4);
      const unsigned count = withPresets ? (extraProgram >= 0 ? 3 : 2) : 0;
      if (withPresets) {
            preset(phdr, root == 69 ? "Sine" : "Overlay", 0, 0);
            if (extraProgram >= 0) preset(phdr, "Extra", extraBank, 1, unsigned(extraProgram));
            preset(phdr, "Sine drum", 128, count - 1);
            }
      preset(phdr, "EOP", 0, count);
      unsigned genIndex = 0;
      for (unsigned i = 0; i <= count; ++i) {
            u16(pbag, genIndex); u16(pbag, 0);
            if (i == count) break;
            if (extraProgram >= 0 && i == 1 && extraCoarseTune) {
                  generator(pgen, 51, unsigned(uint16_t(extraCoarseTune))); ++genIndex;
                  }
            generator(pgen, 41, 0); ++genIndex;
            }
      generator(pgen, 0, 0);
      name(inst, "Sine instrument", 20); u16(inst, 0); name(inst, "EOI", 20); u16(inst, 1);
      const unsigned modCount = unsigned(keyPressurePitch) + unsigned(modulation && modulation->panAmount >= 0)
            + unsigned(modulation && modulation->pitchSource >= 0);
      u16(ibag, 0); u16(ibag, 0);
      u16(ibag, 3 + unsigned(modulation && modulation->pan) + unsigned(exclusiveClass != 0)); u16(ibag, modCount);
      generator(igen, 54, 1); generator(igen, 38, unsigned(uint16_t(-2400)));
      if (exclusiveClass) generator(igen, 57, exclusiveClass);
      if (modulation && modulation->pan) generator(igen, 17, unsigned(uint16_t(modulation->pan)));
      generator(igen, 53, 0); generator(igen, 0, 0);
      sample(shdr, "Sine sample", compressed ? unsigned(compressed->size()) : 44100, compressed != nullptr, root, loopPadding);
      sample(shdr, "EOS", 0);
      chunk(pdta, "phdr", phdr); chunk(pdta, "pbag", pbag); chunk(pdta, "pmod", Bytes(10, 0)); chunk(pdta, "pgen", pgen);
      Bytes imod;
      if (keyPressurePitch) { u16(imod, 10); u16(imod, 51); u16(imod, 12); u16(imod, 0); u16(imod, 0); }
      if (modulation && modulation->panAmount >= 0) {
            u16(imod, 0x028a); u16(imod, 17); u16(imod, unsigned(modulation->panAmount)); u16(imod, 0); u16(imod, 0);
            }
      if (modulation && modulation->pitchSource >= 0) {
            u16(imod, unsigned(modulation->pitchSource)); u16(imod, 51); u16(imod, 12);
            u16(imod, unsigned(modulation->pitchAmountSource)); u16(imod, 0);
            }
      append(imod, Bytes(10, 0));
      chunk(pdta, "inst", inst); chunk(pdta, "ibag", ibag); chunk(pdta, "imod", imod); chunk(pdta, "igen", igen);
      chunk(pdta, "shdr", shdr); chunk(riff, "LIST", pdta);
      Bytes file; chunk(file, "RIFF", riff);
      std::ofstream out(path, std::ios::binary);
      out.write(reinterpret_cast<const char*>(file.data()), std::streamsize(file.size()));
      return bool(out);
      }
}
#endif
