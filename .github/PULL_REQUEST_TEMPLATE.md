## What changed

<!-- A clear and concise description of what you changed -->

> Fixed login screen not showing users created via OOBE.

## Why

<!-- Why is this change needed? What problem does it solve? -->

> Users created during OOBE didn't appear on the login screen because...

## How to test

<!-- Steps to verify your change works. Be specific -->

> 1. Build ARSLANIUS.
> 2. Run OOBE, create a user.
> 3. Reboot. The user should appear on the login screen.

## Checklist

<!-- You must check all of these. If you can't, don't open the PR -->

- [ ] I compiled the code and it builds without errors
- [ ] I tested the change and it works as described
- [ ] My code follows the style in CONTRIBUTING.md (tabs, etc)
- [ ] If I made changes to `clearScreen()`, I'm sure the function works correctly, or even better, both in conhost and in Windows Terminal
- [ ] I agree my code will be licensed under AGPL-3.0

## Additional context

<!-- Anything else worth mentioning. Screenshots, logs, links -->

> Attached a screenshot of the login screen before and after the fix.
