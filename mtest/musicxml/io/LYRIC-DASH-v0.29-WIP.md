# v0.29 WIP — MusicXML references after the lyric dash default update

Apply after v0.28. Update only the 14 references identified by the user's
MusicXML test log. The new default thickness is 0.1sp, so the native writer
omits an explicit 0.1sp, preserves imported explicit 0.15sp, and exports the
default as 1 MusicXML tenth instead of 1.5. No import/export logic is changed.
The dash dimension tests in repeat cover defaults and custom save/reload.
Final validation is recorded with v0.30.
