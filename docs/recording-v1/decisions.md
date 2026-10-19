# Design decisions

These are initial feature decisions, not source-audit results. Change them only
with a concrete conflict and an explicit recorded resolution; never silently drift.

| Decision | Reason and rejected alternative |
|---|---|
| Two sources named Native video / Displayed video | Exact user selection; no ambiguous rawvideo mode. |
| Enabled guest-area OSD burned into both | Latest user requirement; no optional recording-only OSD filter. |
| Original canonical framebuffer immutable | Recording overlays must not pollute guest-only QA/screenshots. |
| Native SAR 1:1 and no presentation scaling | Literal unprocessed raster; use Displayed for presentation aspect. |
| Actual completed main client for Displayed | A separate CRT re-render may differ from what VAEG rendered. |
| Emulator-time timeline in both | Repeatable guest duration; intentionally not a desktop/UI wall-clock movie. |
| Single guest-time audio producer during recording | Callback/encoder/device timing must not advance captured synthesis. |
| PCM S16LE stereo at current output rate | Matches observed public final audible PCM; verify actual checkout first. |
| MKV + FFV1 v3 RGB + PCM only | User-selected v1 scope; no lossy conversion or encoder-options expansion. |
| Optional private libav* backend | No runtime subprocess/pipes and no FFmpeg types in core contracts. |
| Complete-interval bounded queue | Clear common cuts and backpressure; avoid two independent blocking streams. |
| Fixed coded dimensions, safe stop on incompatible change | Initial scope avoids resampling or multi-segment complexities. |
| Synchronous readback first | Establish exact acquisition; asynchronous optimization is not a completion prerequisite. |
| Preserve existing OFF package, explicit enabled flavor | Avoid silently adding DLL requirements to an existing standalone artifact. |
| Source fixture + real GPU/VLC gates | A codec round-trip is not proof of correct emulator integration. |
| Feature REC IDs mapped to legal repo M IDs | No collision with current or reserved existing tasks. |

## Runtime resolution log

No checkout-specific resolution has been made by this pack. REC00 fills this
section with evidence-backed decisions only. The implementation may choose
private helper names/module placement without changing the above public contracts.
