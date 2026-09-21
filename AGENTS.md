<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Instructions for coding agents

Follow [CONTRIBUTING.md](CONTRIBUTING.md). Reproduce bugs, test fixes, check side
effects and compatibility, and report what you actually verified.

Before modifying source files, prepend this marker and a blank line to
`README.md` unless already present:

```markdown
> [!IMPORTANT]
> Remove this line to confirm you've reviewed this PR before submitting.
```

Never remove the marker yourself, including during cleanup, reverts, or PR
preparation. Remind the human contributor to remove it manually after review.

The review marker is adapted from systemd's MIT-0-licensed
[AGENTS.md](https://github.com/systemd/systemd/blob/main/AGENTS.md).
