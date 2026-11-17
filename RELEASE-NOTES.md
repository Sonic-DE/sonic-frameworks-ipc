# Sonic Frameworks IPC Release Notes

Sonic Frameworks IPC is how SonicDE programs talk to each other quickly and reliably behind the scenes.

## Unreleased

- On FreeBSD and GhostBSD, SonicDE programs can now tell exactly which program they are talking to,
  as they already could on Linux.

## 6.31.0 (2026-11-10)

- New SonicDE framework for fast, reliable communication between SonicDE programs such as the login screen, the screen locker and the keyboard layout switcher.
- Built-in protection against corrupted or oversized messages keeps those programs stable.
- Behind the scenes: its developer documentation now appears on the SonicDE website.
