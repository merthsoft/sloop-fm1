# Automatic installer hosting

`.github/workflows/pages.yml` publishes the checked-in `docs` site on pushes to
`merthsloop`. It checks the central firmware version, package name and installer SHA-256
before deploying; a version bump without a matching packaged site fails deployment.
`main` stays upstream-only and does not deploy this fork.

In the GitHub repository, set **Settings → Pages → Source → GitHub Actions**.
If the `github-pages` environment has branch restrictions, allow `merthsloop`.
Repository Actions must also be enabled. Once the workflow is committed and pushed,
the intended public installer URL is:

https://merthsoft.github.io/sloop-fm1/webapp/installer/

This publishes the already-built firmware and browser editor; it does not compile the
hardware firmware in GitHub Actions. Continue building/testing firmware and regenerating
`docs` locally before pushing a new firmware version. The generated offline editor links
back to this fork's public site. GitHub Pages serves HTTPS, suitable for Web MIDI.

Deployment configuration is prepared locally. Repository Pages settings and successful
live deployment have not yet been verified. See GitHub's official custom Pages workflow
documentation: https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages
