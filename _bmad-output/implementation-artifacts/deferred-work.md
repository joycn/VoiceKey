
- source_spec: `_bmad-output/implementation-artifacts/spec-living-room-audio-baseline.md`
  summary: Add resource/command-specific diagnostics for XVF baseline configuration failures.
  evidence: Baseline correctly fails closed but shares generic VK_XVF_IO with existing control failures; diagnosing unsupported commands requires additional telemetry, while current host tests verify safety.
