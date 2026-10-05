# Contributing to ARSLANIUS

PRs are accepted. But read this first - otherwise your PR will be closed without a word

## What I accept

- Bug fixes
- New features that fit the project
- Performance improvements, if they don't break anything
- Documentation fixes

## What I reject instantly

- Whitespace-only changes
- Renaming stuff because "it looks better"
- Removing the copyright notice or AGPL license
- Adding dependencies without a damn good reason
- AI-generated code that you didn't even compile

## Code style

- Tabs, not spaces
- Braces on the same line
- `std::` or `using namespace std;` - either, but be consistent within a file
- No commented-out code. If you don't need it, delete it
- Do not touch `clearScreen()` unless necessary

## Before you open a PR

1. Compile it. If it doesn't build, don't open a PR
2. Test it. If you didn't run it, don't open a PR
3. Check that your changes don't break boot, login, or the shell
4. Fill out the PR template. Vague PRs get closed

## Commits

- One change per commit
- Commit message in English
- Don't squash everything into one giant commit

## License

By submitting a PR, you agree that your code will be licensed under AGPL-3.0, same as the rest of the project

## If your PR is rejected

Don't take it personally. I might:
- Rewrite it my way
- Take the idea but not the code
- Close it because it doesn't fit the project

That's how it works
