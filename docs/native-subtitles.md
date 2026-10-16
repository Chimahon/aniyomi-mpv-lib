# Native subtitle lookup

The pinned MPV and libass sources are patched during dependency preparation.
Patches fail on incompatible source versions rather than applying partially.

MPV exposes `sub-text-geometry` and `secondary-sub-text-geometry` as JSON
strings. Schema version 1 contains `w`, `h` (rendered frame size), and `events`
in painting order. Each event contains its `id`, decoded `text`, video-timeline
`start` and `end` in seconds, and selectable `spans`. Span `start`/`end` are
UTF-16 offsets, and `x`, `y`, `w`, `h` are rendered pixel bounds.

Text and geometry come from the same libass layout. Shaping clusters retain
their source ranges; bitmap padding is excluded. Bounds follow transforms,
rectangular clipping, and event collision shifts. Bounds are axis-aligned;
they do not describe per-pixel masks or vector clipping. Drawing events and
image-only subtitles have no selectable text.

The decoder snapshots geometry under its existing lock. Callers own the
returned snapshot. Timing is converted to the video timeline once, inside
MPV; applications must not apply subtitle delay or speed a second time.

Geometry is collected only when requested, rather than on every video frame.

`MPVLib.setSubtitleTextFilter` accepts a thread-safe deletion/whitespace filter.
The existing app regex implementation runs before libass shapes and positions
text, preserving styling on retained characters. Drawings bypass the text
filter. Returning null leaves text unchanged; an empty result hides the event.
Invalid mappings and Java exceptions leave playback and the original text intact.
Callbacks must never call MPV APIs. Set `sub-text-filter-refresh` to true after
changing filter settings to redraw a paused frame. The bridge clears its filter
reference when the player is destroyed.
