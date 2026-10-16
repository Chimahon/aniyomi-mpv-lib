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
