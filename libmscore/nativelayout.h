// Process-wide opt-outs for comparing the historical engraving algorithms.
#ifndef MS_NATIVE_LAYOUT_H
#define MS_NATIVE_LAYOUT_H

#include <QByteArray>
#include <QtGlobal>

namespace Ms {

inline bool useStudioStemLayout()
      {
      static const bool legacy = qEnvironmentVariableIntValue("MSCORE_LEGACY_STEM_LAYOUT") == 1;
      return !legacy;
      }

inline bool useNativeSlurFit()
      {
      static const bool legacy = qEnvironmentVariableIntValue("MSCORE_LEGACY_SLUR_FIT") == 1;
      return !legacy;
      }

inline bool useNativeRestLayout()
      {
      static const bool legacy = qEnvironmentVariableIntValue("MSCORE_LEGACY_REST_LAYOUT") == 1;
      return !legacy;
      }

inline bool useNativeNaturalSpacing()
      {
      // Also accept the original opt-out used by existing comparison scripts.
      static const bool legacy = qEnvironmentVariableIntValue("MSCORE_LEGACY_NATURAL_SPACING") == 1
                              || qgetenv("MSCORE_NATIVE_NATURAL_SPACING") == "0";
      return !legacy;
      }

} // namespace Ms

#endif // MS_NATIVE_LAYOUT_H
