# M99d: Main Publication

Status: PASS

The reconstructed main was published with an exact
`--force-with-lease` expectation after a fresh fetch confirmed that the
remote main had not moved. The published main now contains the clean
pre-M99 baseline plus only the newly replayed unrelated changes.

The new M99 topic was recreated from that published main. The rejected topic
reference was deleted locally and remotely before this branch was recreated.

Verification:

- local and remote main resolve to the reconstructed candidate;
- the publication used a lease-protected forced update;
- no plain force push was used;
- the new topic has no rejected M99 ancestor;
- the candidate build and CTest results were available before publication.
