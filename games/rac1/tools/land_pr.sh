#!/usr/bin/env bash
# Merge a PR, only if every check of its newest commit passed.
#
#   bash tools/land_pr.sh BRANCH
#
# Exit 0 means it merged. Anything else means it did not: read the output.
# It waits for runs of the newest commit, so an older failed or passed run cannot decide this. It uses a merge commit,
# so the hashes in .git-blame-ignore-revs stay valid.
set -uo pipefail
branch=${1:?usage: land_pr.sh BRANCH}
repo=$(gh repo view --json nameWithOwner --jq .nameWithOwner)
number=$(gh pr list -R "$repo" --head "$branch" --json number --jq '.[0].number')
[ -n "$number" ] || { echo "*** no open PR for $branch"; exit 1; }
sha=$(gh pr view "$number" -R "$repo" --json headRefOid --jq .headRefOid)

for _ in $(seq 1 30); do
    runs=$(gh run list -R "$repo" --commit "$sha" --json databaseId --jq length 2>/dev/null)
    [ "${runs:-0}" -ge 2 ] && break
    read -r -t 6 < /dev/zero || true
done

gh pr checks "$number" -R "$repo" --watch > /dev/null 2>&1
status=$?
gh pr checks "$number" -R "$repo" 2>&1 | cut -f1,2 | tr '\t' ' ' | tr '\n' ';'
echo
if [ "$status" -ne 0 ]; then
    echo "*** #$number: checks failed, not merging"
    exit 1
fi
gh pr merge "$number" -R "$repo" --merge --delete-branch=false > /dev/null 2>&1
state=$(gh pr view "$number" -R "$repo" --json state --jq .state)
echo "#$number: $state"
[ "$state" = MERGED ] || exit 1
gh api -X DELETE "repos/$repo/git/refs/heads/$branch" > /dev/null 2>&1 || true
