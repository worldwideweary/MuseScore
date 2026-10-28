# Native FluidSynth SoundFont fixture

`sine.ogg` is an original, mono, one-second 440 Hz sine at 44100 Hz. It contains
no samples from an external SoundFont. It is distributed with these tests under
GNU GPL version 2 (see LICENSE.GPL).

Generated once with FFmpeg's libvorbis encoder:

```sh
ffmpeg -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=1' \
  -c:a libvorbis -q:a 4 -map_metadata -1 sine.ogg
```

Tests do not invoke or require FFmpeg. `testfont.h` embeds these already encoded
bytes in a minimal SF3 at runtime, using compressed byte offsets in the sample
header and decoded-frame offsets for the loop. The same builder generates SF2,
a valid font with no presets and an SF3 with damaged compressed sample data.
SF2/SF3 are compared for usable playback and tuning, not bit-identical PCM: the
Vorbis fixture is lossy and has a different amplitude from the SF2 fixture.
