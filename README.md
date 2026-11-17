# SmileMorph Precomp 0.3 — source candidate

An experimental native Windows x64 effect for Adobe After Effects 2027, with an ExtendScript setup helper. It uses animated open mask paths as guides for a morph between two still-image snapshots. The transition duration is set by the editor and is not fixed to a frame count or one-second interval.

## Status

This is a development prototype, not a production release. The initial build loaded in After Effects 2027 and evaluated the generated guide paths, but its render showed severe warp artifacts. A correction was compiled after that test. The corrected `.aex` has not been validated in After Effects because the development Windows machine's Smart App Control blocked the unsigned binary. Do not treat the current binary as a verified release.

The source is provided for review. No Adobe SDK headers, user projects, source images, built binaries, private security-policy inventory, or machine-specific logs are included.

## Requirements

- Windows x64
- Adobe After Effects 2027 SDK, obtained separately from Adobe and used under Adobe's SDK license
- Microsoft C++ build tools with the Windows SDK, invoked from an x64 Native Tools command prompt
- Python 3 on `PATH`

The build script expects `AE_SDK_EXAMPLES` to point to the SDK's `Examples` directory, which contains `Headers`, `Util`, and related SDK folders.

## Build

From an x64 Native Tools command prompt, set `AE_SDK_EXAMPLES` and run `build.cmd`:

```bat
set AE_SDK_EXAMPLES=C:\path\to\AfterEffectsSDK\Examples
build.cmd
```

This creates `SmileMorphPrecomp.aex` beside the script. The output is unsigned. Signing and distribution are separate release steps; Smart App Control may block an unsigned development build.

## Intended workflow

1. In After Effects, create a precomposition with aligned Before and After stills.
2. Draw open guide masks over the relevant features on a layer inside that precomp. Animate the same paths to match the After image; keep vertex count and order consistent.
3. Select the precomp layer in its parent composition and run `Setup SmileMorph Precomp.jsx` through **File > Scripts > Run Script File**.
4. In the generated duplicate, move the first and last **Transition** keyframes to the desired endpoints. The helper creates separate path bridges for the Before, After, and current guide positions.
5. Adjust the inner guide paths as needed. The setup helper preserves the source composition and creates a separate test copy.

The helper can read endpoint keys from a RE:Flex reference effect when available. Otherwise, it starts from the precomp layer/work-area bounds. It does not assume a particular duration or frame rate.

## Current limitations

- Experimental warp quality; corrected build still needs host and visual validation.
- CPU render, 8/16 bpc; no 32-bpc, GPU, or Multi-Frame Rendering support claimed.
- Maximum 600 original vertices across open guides.
- Two aligned still images of matching pixel dimensions; video sources, time-remapping during setup, and reverse-time setup are not supported.
- Keep all guide paths open, with matching vertex counts and point order. The helper currently expects an unmasked parent precomp layer.
- This is not a feature-complete or quality-equivalent replacement for RE:Flex.

## License

Project source is licensed under MIT; see `LICENSE.txt`. Adobe After Effects SDK files are not included and remain subject to Adobe's terms.
