// Test-only Qt bridge for compiling the unmodified production Zita DSP.
// No widgets, moc, or Qt state serialization are exercised by these tests.
// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_TEST_ZITA_EFFECT_H
#define MS_TEST_ZITA_EFFECT_H
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#define Q_OBJECT
namespace Ms {
class QString {
      std::string text;
   public:
      QString(const char* value = "") : text(value) {}
      QString arg(double value) const { return QString(std::to_string(value).c_str()); }
      double toDouble() const { return std::stod(text); }
      };
struct IdValue {
      int id;
      QString data;
      IdValue(int i, QString v) : id(i), data(v) {}
      };
struct SynthesizerGroup : std::vector<IdValue> {
      void setName(const char*) {}
      };
class EffectGui;
struct ParDescr {
      int id;
      const char* name;
      bool log;
      float min, max, init;
      };
class Effect {
   protected:
      EffectGui* _gui = nullptr;
   public:
      virtual ~Effect() {}
      virtual void process(int, float*, float*) = 0;
      virtual const char* name() const = 0;
      virtual void init(float) {}
      virtual const std::vector<ParDescr>& parDescr() const = 0;
      virtual void setNValue(int, double) = 0;
      virtual double nvalue(int) const = 0;
      virtual SynthesizerGroup state() const = 0;
      virtual void setState(const SynthesizerGroup&) = 0;
      virtual EffectGui* gui() { return nullptr; }
      double value(int index) const { return nvalue(index); }
      void setValue(int index, double value) { setNValue(index, value); }
      };
}
#endif
