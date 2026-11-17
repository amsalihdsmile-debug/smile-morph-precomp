# Code signing policy

## Status

SmileMorph has not applied to or been accepted by SignPath Foundation. No SmileMorph binary is currently signed, and no signed release is available. The current native plug-in is an experimental prototype that still requires visual validation in After Effects.

If the project is accepted, the intended service is: **Free code signing provided by SignPath.io, certificate by SignPath Foundation.** This statement describes the intended route only and is not a claim of acceptance or signing.

## Maintainers and signing roles

- **Author/committer:** `amsalihdsmile-debug` (GitHub repository owner).
- **Reviewer:** the maintainer reviews all external contributions before merging, with extra attention to source, build scripts, release workflows, and dependencies.
- **Approver:** `amsalihdsmile-debug` approves signing requests only after the release has been built from the reviewed repository revision and validated in After Effects.
- **Account security:** multi-factor authentication must be enabled for GitHub and any signing service before signing is configured.

## Signing scope and release checks

Only the SmileMorph native After Effects plug-in (`SmileMorphPrecomp.aex`) may be submitted for signing. It must be built from this repository's reviewed release commit using a trusted, hosted build workflow. Arbitrary local binaries, user projects, Adobe SDK binaries, and third-party binaries are outside the signing scope.

Every signing request requires maintainer approval. Before approval, the maintainer must confirm the source revision, build workflow result, plug-in identity and version metadata, and visual validation in a supported After Effects version. Until those checks and SignPath approval are in place, builds remain unsigned development artifacts and must not be described as signed or production-ready.

## Privacy

The plug-in and setup helper do not transmit project, image, or mask data to networked systems. The build uses local developer tools and the separately obtained Adobe After Effects SDK. Any future component that transmits user data must be documented here and accompanied by appropriate user-facing notice and controls before release.

## Verification and response

For a future signed release, verify the Authenticode signature and publisher in Windows before installation, and compare the release checksum with the value published alongside that exact artifact. If a signing or release issue is suspected, stop signing and distribution, investigate the affected source and artifacts, and notify the signing provider when applicable. Never reuse a revoked, expired, or untrusted signing identity.
