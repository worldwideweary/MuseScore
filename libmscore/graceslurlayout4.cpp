// Adapted from Studio v4.7.5 SlurTieLayout::avoidCollisions and its clearance,
// adjustment-step and balance helpers. Licensed under GNU GPL version 2.
#include "spacingprofile4.h"
#include "graceslurlayout4.h"
#include "gracespacing4.h"
#include "beam.h"
#include "chord.h"
#include "measure.h"
#include "note.h"
#include "score.h"
#include "segment.h"
#include "slur.h"
#include "slurtie.h"
#include "stem.h"
#include "system.h"
#include <QTransform>
#include <algorithm>
#include <cmath>

namespace Ms {
bool fitGraceSlurCurve4(const SpacingShape4& physical, const Chord* target, qreal spatium,
                       bool up, GraceSlurCurve4& result, bool startFloated, bool endFloated, QString* reason)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::CURVE);
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    const auto finite = [](const QPointF& p) { return std::isfinite(p.x()) && std::isfinite(p.y()); };
    if (!target || !(spatium > 0) || !std::isfinite(spatium) || !physical.complete()
        || !finite(result.start) || !finite(result.end) || !finite(result.control1) || !finite(result.control2)
        || result.end.x() - result.start.x() <= 1e-6 * spatium)
        return fail("Invalid grace slur fitting input");
    std::vector<QRectF> obstacles;
    for (const auto& component : physical.elements) {
        QRectF rect = component.rect;
        if (!component.item || !std::isfinite(rect.left()) || !std::isfinite(rect.right())
            || !std::isfinite(rect.top()) || !std::isfinite(rect.bottom()) || rect.width() < 0 || rect.height() < 0)
            return fail("Invalid grace slur obstacle");
        const Element* owner = component.item;
        while (owner && !owner->isChord()) owner = owner->parent();
        // Actual endpoint-owned components receive no extra item clearance.
        // Intervening notes get Studio's 0.4-space clearance; other admitted
        // physical components get 0.1. Articulations are outside this scope.
        const qreal clearance = owner == target ? 0 : (component.item->isNote() ? 0.4 : 0.1) * spatium;
        rect.adjust(0, up ? -clearance : 0, 0, up ? 0 : clearance);
        if (!std::isfinite(rect.top()) || !std::isfinite(rect.bottom()))
            return fail("Non-finite grace slur clearance");
        obstacles.push_back(rect);
    }
    QPointF pp1 = result.start;
    const qreal angle = std::atan2(result.end.y() - pp1.y(), result.end.x() - pp1.x());
    QTransform rotate;
    rotate.rotateRadians(-angle);
    QPointF p2 = rotate.map(result.end - pp1);
    QPointF p3 = rotate.map(result.control1 - pp1);
    QPointF p4 = rotate.map(result.control2 - pp1);
    const qreal length = p2.x() / spatium;
    const qreal sign = up ? -1 : 1;
    const qreal arc = -sign * std::max(qreal(0.1), std::min(qreal(0.5),
                      qreal(0.04) * length * std::pow(std::cos(angle), 2))) * spatium;
    // Single automatic segment, no articulations or cross-staff context.
    // Studio balances floated endpoints at 0.9 and ordinary ones at 0.4.
    const qreal leftBalance = startFloated ? 0.9 : 0.4;
    const qreal rightBalance = endFloated ? 0.9 : 0.4;
    const qreal step = sign * std::min(qreal(1.5), qreal(0.30) * std::max(qreal(1), length / 16)) * spatium;
    const qreal pi = std::acos(qreal(-1));
    const auto transform = [&]() {
        QTransform t;
        t.translate(pp1.x(), pp1.y());
        t.rotateRadians(angle);
        return t;
    };
    const auto collisions = [&](bool& left, bool& mid, bool& right) {
        left = mid = right = false;
        const QTransform t = transform();
        const CubicBezier clearance(QPointF(), p3 + QPointF(0, arc), p4 + QPointF(0, arc), p2);
        // Include the final interval too, so successful fitting certifies the
        // complete endpoint span rather than stopping at 95 percent.
        for (int i = 0; i < 20; ++i) {
            const QRectF curve = QRectF(t.map(clearance.pointAtPercent(i / qreal(20))),
                                       t.map(clearance.pointAtPercent((i + 1) / qreal(20)))).normalized();
            for (const QRectF& obstacle : obstacles) {
                if (curve.right() <= obstacle.left() || curve.left() >= obstacle.right()) continue;
                // Studio Shape::clearsVertically: vertical order, not merely
                // rectangle intersection, must hold wherever x overlaps.
                if (up ? curve.bottom() >= obstacle.top() : curve.top() <= obstacle.bottom()) {
                    if (i < 6) left = true;
                    else if (i < 13) mid = true;
                    else right = true;
                    break;
                }
            }
        }
        return left || mid || right;
    };
    for (int iteration = 0; iteration <= 30; ++iteration) {
        bool left, mid, right;
        if (!collisions(left, mid, right)) {
            const QTransform t = transform();
            GraceSlurCurve4 fitted;
            fitted.start = pp1;
            fitted.control1 = t.map(p3);
            fitted.control2 = t.map(p4);
            fitted.end = t.map(p2);
            if (!finite(fitted.start) || !finite(fitted.control1) || !finite(fitted.control2) || !finite(fitted.end))
                return fail("Non-finite fitted grace slur");
            result = fitted;
            if (reason) reason->clear();
            return true;
        }
        if (iteration == 30) break;
        if (iteration % 2 == 0) {
            if (left) {
                const qreal delta = (1 - leftBalance) * step / 2;
                p3 += QPointF(-std::abs(delta), delta);
                p4 += QPointF(std::abs(delta), delta) / 2;
            }
            if (mid) {
                p3 += QPointF(0, (2 - leftBalance - rightBalance) * step / 2);
                p4 += QPointF(0, (2 - leftBalance - rightBalance) * step / 2);
            }
            if (right) {
                const qreal delta = (1 - rightBalance) * step / 2;
                p4 += QPointF(std::abs(delta), delta);
                p3 += QPointF(-std::abs(delta), delta) / 2;
            }
        }
        else {
            const QPointF endpoint = transform().map(p2);
            const qreal slant = (endpoint.y() - pp1.y()) / (endpoint.x() - pp1.x());
            const qreal slantLimit = 5 * pi / 180;
            if (left || mid || angle < -pi / 4) {
                qreal delta = leftBalance * step;
                if (!left && mid && std::abs(slant) > slantLimit && length > 8) {
                    const qreal ratio = std::abs(slant) / slantLimit;
                    delta *= ((up && slant > 0) || (!up && slant < 0)) ? 1 / ratio : ratio;
                }
                const QPointF change(std::sin(angle) * delta, std::cos(angle) * delta);
                pp1 += QPointF(0, delta);
                p3 += change * (p2.x() - p3.x()) / p2.x();
                p4 += change * (p2.x() - p4.x()) / p2.x();
                p2 -= change;
                p3 -= change;
                p4 -= change;
                if (p2.x() <= 1e-6 * spatium) return fail("Degenerate grace slur fitting span");
            }
            if (right || mid || angle > pi / 4) {
                qreal delta = rightBalance * step;
                if (!right && mid && std::abs(slant) > slantLimit && length > 8) {
                    const qreal ratio = std::abs(slant) / slantLimit;
                    delta *= ((up && slant < 0) || (!up && slant > 0)) ? 1 / ratio : ratio;
                }
                const QPointF change(std::sin(angle) * delta, std::cos(angle) * delta);
                p2 += change;
                if (p2.x() <= 1e-6 * spatium) return fail("Degenerate grace slur fitting span");
                p3 += change * p3.x() / p2.x();
                p4 += change * p4.x() / p2.x();
            }
        }
        // Studio's height and tangent bounds, applied in the updated frame.
        const qreal maxHeight = std::abs(p2.x());
        p3.setY(up ? std::max(p3.y(), -maxHeight) : std::min(p3.y(), maxHeight));
        p4.setY(up ? std::max(p4.y(), -maxHeight) : std::min(p4.y(), maxHeight));
        const QTransform t = transform();
        const QPointF endpoint = t.map(p2);
        if (!finite(pp1) || !finite(p2) || !finite(p3) || !finite(p4) || endpoint.x() <= pp1.x())
            return fail("Degenerate grace slur fitting geometry");
        for (QPointF* point : {&p3, &p4}) {
            QPointF system = t.map(*point);
            system.setX(std::max(pp1.x(), std::min(endpoint.x(), system.x())));
            *point = t.inverted().map(system);
        }
    }
    return fail("Grace slur clearance exceeds bounded fitting passes");
}

bool adjustGraceMergedSlurBeamAnchors4(const Slur* slur, QPointF& startPoint, QPointF& endPoint, QString* reason, bool allowVisualEdits,
                                      GraceMergedSlurQuery4* query)
{
    SpacingProfileScope4 profile(SpacingProfileStage4::ANCHORS);
    const auto fail = [reason](const char* message) {
        if (reason) *reason = QString::fromLatin1(message);
        return false;
    };
    if (!slur || !slur->startElement() || !slur->startElement()->isChord()
        || !slur->endElement() || !slur->endElement()->isChord()
        || !std::isfinite(startPoint.x()) || !std::isfinite(startPoint.y())
        || !std::isfinite(endPoint.x()) || !std::isfinite(endPoint.y()))
        return fail("Invalid combined grace beam anchors");
    const Chord* start = toChord(slur->startElement());
    const Chord* target = toChord(slur->endElement());
    if (!start->isGraceAfter() || !start->parent() || !start->parent()->isChord()
        || !start->beam() || start->beam()->elements().empty()
        || start->beam()->elements().back() == start || start->up() != slur->up())
        return fail("Combined grace slur does not use the beam-side start branch");
    const Chord* owner = toChord(start->parent());
    if (!spacingMergedGraceSlur4(slur, owner, target, allowVisualEdits) || !owner->measure()->system()
        || owner->measure()->system() != target->measure()->system())
        return fail("Unadapted combined grace beam anchors");
    GraceMergedSlurQuery4 localQuery;
    if (!(query ? query : &localQuery)->geometry(owner, reason)) return false;
    const qreal sp = slur->spatium();
    const qreal mag = start->chordMag();
    // Preserve Studio slurPos's beamWidthSp expression exactly. It uses the
    // style's space value times magS(), not the drawn beam width in points.
    const qreal beamWidth = start->score()->styleS(Sid::beamWidth).val() * start->beam()->magS();
    if (!(sp > 0) || !(mag > 0) || !(beamWidth > 0)
        || !std::isfinite(sp) || !std::isfinite(mag) || !std::isfinite(beamWidth)
        || !(start->stem()->len() > 0) || !(target->stem()->len() > 0)
        || !std::isfinite(start->stem()->len()) || !std::isfinite(target->stem()->len())
        || !std::isfinite(start->stem()->lineWidthMag()) || !std::isfinite(target->stem()->lineWidthMag()))
        return fail("Invalid combined grace stem or beam metrics");
    const qreal sign = slur->up() ? -1 : 1;
    const auto origin = [](const Chord* chord) {
        QPointF p = chord->pos() + chord->segment()->pos() + chord->measure()->pos();
        if (chord->isGrace()) p += chord->parent()->pos();
        // Match slurPos's note-centering adjustment; mirroring is rejected by
        // the validated joint component/ownership scope.
        p.rx() += chord->notes().front()->x();
        return p;
    };
    // Studio slurPos's beamed, non-final, stem-side start branch. Use the
    // source's beamWidthSp term here; the beam layout anchor width and drawn
    // beam polygon width are separate metrics.
    const qreal startHeight = start->stem()->len() + beamWidth / 2 + std::max(qreal(.35) * mag, qreal(.2)) * sp;
    const qreal startStemY = start->stemPos().y() - start->pagePos().y();
    QPointF adjustedStart = origin(start) + QPointF(
        start->stemPosX() + .15 * sp * mag + sign * start->stem()->lineWidthMag() / 2,
        startStemY + sign * startHeight);
    if (slur->up()) {
        const Beam* beam = start->beam();
        const qreal capRadius = start->score()->styleP(Sid::slurEndWidth) / 2;
        if (beam->elements().front() != start || !std::isfinite(capRadius) || capRadius < 0)
            return fail("Invalid leading beam edge for upper grace slur");
        // User-preferred alignment: the rounded visible start lies directly
        // above the drawn beam's leading edge. Beam::layout2 builds that edge
        // from the first up-stem position minus its full line width. Its cached
        // primary segment may still describe the spacing-preparation pass, so
        // derive the same edge from CURRENT chord coordinates instead.
        const qreal leadingEdge = start->pageX() - owner->measure()->system()->pagePos().x()
            + start->stemPosX() - start->stem()->lineWidthMag();
        adjustedStart.setX(leadingEdge + capRadius);
    }
    QPointF adjustedEnd = endPoint;
    if (start->up() == target->up()) {
        // The stem-side start forces the same-direction main endpoint to its
        // stem. This scope's target is unbeamed and has ordinary magnitude.
        const qreal endMag = target->chordMag();
        const qreal endHeight = target->stem()->len() + std::max(qreal(.35) * endMag, qreal(.2)) * sp;
        const qreal endStemY = target->stemPos().y() - target->pagePos().y();
        adjustedEnd = origin(target) + QPointF(
            target->stemPosX() + sign * target->stem()->lineWidthMag() / 2 - .15 * sp * endMag,
            endStemY + sign * endHeight);
    }
    if (!std::isfinite(adjustedStart.x()) || !std::isfinite(adjustedStart.y())
        || !std::isfinite(adjustedEnd.x()) || !std::isfinite(adjustedEnd.y()))
        return fail("Non-finite combined grace beam anchors");
    startPoint = adjustedStart;
    endPoint = adjustedEnd;
    if (reason) reason->clear();
    return true;
}
}
