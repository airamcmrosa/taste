---
name: coordinate
description: Coordinator flow for executing tasks from docs/tasks.md via subagents (implementer, test-writer, test-runner). Use when asked to coordinate/orchestrate/delegate task execution.
---
# Coordinate

You are the coordinator. You delegate, reconcile, commit and push. You do not implement.

## Flow
1. Read `active-task` memory, then only the needed `## T-NN` blocks and the REQ rows they cite (never whole tasks.md).
2. Split into slices. Tasks with no dependency between them run **in parallel, same worktree**, in one message with multiple Agent calls. Dependent tasks run in order.
3. Brief each subagent concisely: goal, the relevant **snippets** (not whole files), file paths to touch, expected output. Nothing more.
4. Subagents **never commit or push**.
5. Code tasks (CLAUDE.md §2.1 applies) use three distinct agents:
   - `implementer` (Read/Write/Grep/Glob only, no Bash) writes the code.
   - `test-writer` (different agent; same tool limits) writes tests from the REQs.
   - `test-runner` (haiku, read-only, only `scripts/run-tests.sh <T-NN>`) runs the suite; the script writes `.test-results/<T-NN>.json`. If the script doesn't exist, make creating it the first test-writer slice.
   Implementer never writes, runs or judges the tests. On failure: send the runner's output back to the implementer, then rerun.
6. Docs-only / hardware-only tasks are exempt from tests; use `implementer` (or do the doc edit directly if trivial).

## Reconciliation (coordinator only, after all slices of a batch return)
- Check changed files match the briefs (`git status`/`git diff`).
- For code tasks, apply the Test Gate: result file exists, `status` pass, `failed` 0, newer than code/test changes.
- Update: the task's `status` line and checkbox in `docs/tasks.md`, the index table, frontmatter `active_task`, and the `active-task` memory card.
- Then `git add` the batch, commit (with the attribution line from the session), and `git push`. Do not commit if the gate fails; report to the user.
