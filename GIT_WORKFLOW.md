# Git workflow for this hackathon

Judges will look at your **commit history**, not just the final code. Push
early, push often, and commit each feature/milestone separately - never one
giant "final commit" at the end.

## One-time setup

```bash
cd Digital-Twin          # this folder
git init
git branch -M main
git remote add origin https://github.com/<your-username>/<your-repo>.git
```

## Suggested commit sequence (one push per milestone)

Do these as **separate commits**, ideally on separate days/sessions to build
a real history - don't just run them back-to-back at the last minute.

```bash
# 1. Project scaffold
git add README.md LICENSE .gitignore
git commit -m "docs: add README, license and gitignore"
git push -u origin main

# 2. Frontend - home page
git add frontend/home-page
git commit -m "feat(frontend): add home page app launcher"
git push

# 3. Frontend - status dashboard
git add frontend/status-page
git commit -m "feat(frontend): add EV digital twin gauge dashboard"
git push

# 4. Backend
git add backend
git commit -m "feat(backend): add Spring Boot battery REST API"
git push

# 5. Firmware
git add firmware
git commit -m "feat(firmware): add ESP32 sensor + ThingSpeak telemetry sketch"
git push

# 6. Diagrams / docs
git add docs
git commit -m "docs: add architecture, workflow and circuit diagrams"
git push

# 7. Screenshots / demo (after you add real files)
git add docs/screenshots demo
git commit -m "docs: add demo screenshots and video link"
git push
```

## Commit message conventions

Use a short prefix so the history reads clearly:

- `feat:` a new feature
- `fix:` a bug fix
- `docs:` documentation only
- `refactor:` code change that doesn't add a feature or fix a bug
- `test:` adding or fixing tests
- `chore:` tooling, config, dependency bumps

Example: `feat(firmware): compute SOC/SOH/DTE from sensor readings`

## Before your final push

- [ ] Remove any real Wi-Fi password or ThingSpeak API key from tracked files
      (`firmware/**/config.h` should never appear in `git status`)
- [ ] Fill in the [Team](README.md#team) table in the README
- [ ] Add real screenshots to `docs/screenshots/`
- [ ] Add your demo video link to `demo/` or the README's Demo section
- [ ] Confirm `git log --oneline` shows multiple, clearly-labelled commits
