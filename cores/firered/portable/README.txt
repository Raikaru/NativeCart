Portable runtime shims that remain FireRed-specific will move here over time.
For now they still live under `pokefirered_core/src/` and generated portable data.

Portable debugging reminders:
- Prefer fixing transformed runtime users in `src_transformed/` when equivalents exist.
- Audit mixed C/UTF-8 vs FireRed byte-string paths before assuming text rendering is wrong.
- Audit raw `INCBIN` globals and portable embedded asset headers before assuming a graphics regression is shader or renderer related.
- For generated portable text data, verify the generator output before patching shared printer code.
