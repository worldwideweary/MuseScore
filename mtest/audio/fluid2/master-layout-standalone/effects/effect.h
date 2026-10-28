// Test-only Qt bridge: only MasterSynthesizer's public header layout is used.
// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_MASTER_LAYOUT_EFFECT_STUB_H
#define MS_MASTER_LAYOUT_EFFECT_STUB_H
#include <string>
#include <vector>
#define Q_OBJECT
#define slots
#define signals public
#define emit
using QString = std::string;
template <typename T> using QList = std::vector<T>;
class QObject { public: explicit QObject(QObject* = nullptr) {} virtual ~QObject() {} };
namespace Ms { class Effect; }
#endif
