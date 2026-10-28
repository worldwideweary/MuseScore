# WIP checkpoint v0.27 — one-note partial slur shape

Apply after v0.26. The native rule is not simply a special direction for a
one-note slur. Studio uses a half-spatium stem inset, constrains the free end
0.25sp beyond the attached end, then avoids endpoints landing on staff lines.
With the S03 C4 example this yields about 1.75sp and 1.77sp for the two ends,
instead of the backport's clearly tilted 1.50sp and 1.75sp.

Use that stem inset and staff-line adjustment for partial slurs, retaining
ordinary slur layout and manual endpoint offsets. One-note partial slurs also
use Studio's square-root shoulder-height rule, rather than the old short-arc
linear rule. Existing 0.25sp anchor tests check pre-adjustment anchor geometry;
a new test checks the actual displayed endpoints and preserves manual edits.

Final series validation is recorded in the last checkpoint. User's previous
37E-to-4.7.5 loading checks are accepted and no longer deferred.
