// Optional layout timing, enabled at process startup with MS_LAYOUT4_PROFILE=1.
// Licensed under GNU GPL version 2; see LICENSE.GPL.
#ifndef MS_SPACINGPROFILE4_H
#define MS_SPACINGPROFILE4_H
#include <QElapsedTimer>
#include <QDebug>
#include <QStringList>
#include <array>

namespace Ms {
enum class SpacingProfileStage4 {
    UPDATE, SYSTEM, PLAN, BEFORE, AFTER, MERGED, GEOMETRY, BEAMS,
    OBSTACLES, ANCHORS, CURVE, CONTEXT, BEAM_REUSE,
    COMPONENTS, GRACE_DISTANCE, PADDING_TABLE, ELEMENT_SHAPE, CHORD_CHECK, SHAPE_REUSE, COUNT
};
struct SpacingProfileData4 {
    struct Entry { qint64 calls = 0, inclusive = 0, self = 0; };
    std::array<Entry, static_cast<int>(SpacingProfileStage4::COUNT)> entries;
    QElapsedTimer command;
    bool collecting = false;
    qint64* parentChildren = nullptr;
};
inline bool spacingProfileEnabled4()
{
    static const bool enabled = qgetenv("MS_LAYOUT4_PROFILE") == "1";
    return enabled;
}
inline SpacingProfileData4& spacingProfileData4()
{
    // No score pointers or geometry survive an edit. Each layout thread owns
    // its counters; disabled runs do not initialize this state or read a clock.
    static thread_local SpacingProfileData4 data;
    return data;
}
inline void beginSpacingProfile4()
{
    if (!spacingProfileEnabled4()) return;
    auto& data = spacingProfileData4();
    data.entries = {};
    data.parentChildren = nullptr;
    data.command.start();
    data.collecting = true;
}
class SpacingProfileScope4 {
    SpacingProfileStage4 _stage;
    bool _active;
    QElapsedTimer _timer;
    qint64 _children = 0;
    qint64* _parent = nullptr;
public:
    explicit SpacingProfileScope4(SpacingProfileStage4 stage) : _stage(stage),
        _active(spacingProfileEnabled4() && spacingProfileData4().collecting)
    {
        if (!_active) return;
        auto& data = spacingProfileData4();
        _parent = data.parentChildren;
        data.parentChildren = &_children;
        _timer.start();
    }
    ~SpacingProfileScope4()
    {
        if (!_active) return;
        const qint64 elapsed = _timer.nsecsElapsed();
        auto& data = spacingProfileData4();
        auto& entry = data.entries[static_cast<int>(_stage)];
        ++entry.calls;
        entry.inclusive += elapsed;
        entry.self += elapsed - _children;
        if (_parent) *_parent += elapsed;
        data.parentChildren = _parent;
    }
    SpacingProfileScope4(const SpacingProfileScope4&) = delete;
    SpacingProfileScope4& operator=(const SpacingProfileScope4&) = delete;
};
inline void endSpacingProfile4()
{
    if (!spacingProfileEnabled4()) return;
    auto& data = spacingProfileData4();
    if (!data.collecting) return;
    const qreal elapsed = data.command.nsecsElapsed() / 1000000.0;
    data.collecting = false;
    static const char* names[] = { "update", "system", "plan", "before", "after", "merged",
        "geometry", "beams", "obstacles", "anchors", "curve", "context", "beamReuse",
        "components", "graceDistance", "paddingTable", "elementShape", "chordCheck", "shapeReuse" };
    static_assert(sizeof(names) / sizeof(*names) == static_cast<int>(SpacingProfileStage4::COUNT),
                  "Every layout timing stage needs a label");
    QStringList parts;
    for (int i = 0; i < static_cast<int>(SpacingProfileStage4::COUNT); ++i) {
        const auto& entry = data.entries[i];
        if (entry.calls)
            parts << QString("%1=%2/%3/%4").arg(names[i]).arg(entry.calls)
                .arg(entry.inclusive / 1000000.0, 0, 'f', 3).arg(entry.self / 1000000.0, 0, 'f', 3);
    }
    qDebug().noquote() << QString("layout4-profile command=%1ms [calls/inclusive-ms/self-ms] %2")
        .arg(elapsed, 0, 'f', 3).arg(parts.join(' '));
}
}
#endif
