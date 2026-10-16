---
name: writing-style
description: House writing rules for this repo. Load before writing or editing any prose, including README, docs, ADRs, changelog, commit messages, PR descriptions, issue text, code comments and header doc comments. Use it as a final pass on anything already written. Trigger on "write the docs", "update the README", "commit this", "open a PR", "add comments", or any request that produces text a human will read.
---

# Writing style

Everything written here has to read like an engineer wrote it. Not a model.

## Hard rules

1. **No em dashes.** Use a period, a comma, parentheses, or a colon.
2. **No AI filler.** Banned outright: delve, leverage (as a verb), seamless, robust,
   comprehensive, cutting-edge, best-in-class, game-changing, unlock, empower, foster,
   navigate (figurative), realm, landscape, tapestry, "it's worth noting", "it's important to
   note", "in today's world", "at the end of the day".
3. **No closing summary that restates the section.** Stop when the point is made.
4. **No duplication.** If a fact appears in two places, one of them links to the other.
5. **Short paragraphs.** Three or four sentences. Break anything longer.
6. **Plain words.** "fix" not "implement a solution for". "use" not "utilise". "so" not
   "thereby". "start" not "commence".
7. **No hedging stacks.** Pick one: "probably", not "it may potentially be possible that".
8. **No triads for rhythm.** Three items only when there are genuinely three.
9. **Numbers over adjectives.** "Fast" is not a claim. "7.09 ns" is, and it names where it was
   measured. Every claim about speed, size or coverage carries the number and the place that
   measured it.

## Commits and PRs

- Conventional Commits: `type(scope): summary`. Types: feat, fix, docs, chore, refactor, test,
  build, ci, perf.
- Summary in the imperative, lower case, no trailing period, under 72 characters.
- Body explains why, not what. The diff already says what.
- **Never add `Co-Authored-By`, `Generated with`, or any other AI attribution.**

Good:

```
fix(parse): reject a digit run longer than four before it overflows

The accumulator stops at four digits, so a longer run only sets a flag; the
flag was checked after the group was stored, which read an unset field.
```

Bad:

```
feat: 🚀 Implement comprehensive parsing

This commit leverages cutting-edge techniques to seamlessly parse...

Co-Authored-By: Claude <noreply@anthropic.com>
```

## Code comments and doc comments

Comment why, not what. If the code needs a comment to say what it does, rename something.

Good:

```cpp
// The block table holds at most two month starts, so two conditional adds are enough (ADR-0002 §3).
```

Bad:

```cpp
// This function converts the serial day back to a date and returns it.
```

Doc comments on public members are required. Say what the member does, what it returns, and which
`errc` value it can return. Skip the marketing.

## Docs and README

- Lead with what the thing is and who it is for. No throat clearing.
- State limits honestly and early (the BS range, the patro data range). A doc that hides a limit
  costs more trust than the limit does.
- Tables for facts. Prose for reasoning.
- Code samples must compile. If they cannot yet, say so.

## Final pass

Read it back and cut:

- Every em dash.
- Every sentence that could be deleted without losing information.
- Every word from the banned list.
- Every paragraph over four sentences.
- Every restatement of something said above.

If a section survives the cut unchanged, it was probably already fine.
