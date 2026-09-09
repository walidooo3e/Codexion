*This project has been created as part of the 42 curriculum by wabdi.*

# Codexion

## Abstract

Codexion is a POSIX-threaded simulation of bounded, mutually-exclusive resource
contention, cast in the idiom of a shared co-working space in which *n* coders
compete for *n* USB dongles arranged in a cyclic adjacency graph. Each coder
requires exactly two dongles (its two ring-neighbours) held simultaneously
to enter a compiling phase; the central engineering problem is therefore an
instance of the classical Dining Philosophers problem (Dijkstra, 1965), subject
to an additional real-time constraint: a coder that fails to begin compiling
within a bounded interval of its previous compilation is considered to have
*burned out*, terminating the simulation. This document describes the design
choices made to guarantee mutual exclusion, deadlock-freedom, bounded waiting,
and burnout-detection precision under concurrent execution.

## 1. Description

The goal of the project is to model and correctly simulate a bounded set of
producer/consumer-like agents (coders) contending for a bounded pool of shared,
non-fungible resources (dongles), under two distinct scarcity constraints:

- **Simultaneity**: compiling requires two dongles held at once, not one
  dongle used twice — this is what introduces the possibility of deadlock.
- **Cooldown**: a dongle is not immediately reusable after release; it enters
  an unavailable state for `dongle_cooldown` milliseconds, which is what
  introduces genuine queuing and, consequently, the need for a fair
  arbitration policy.

Each coder is represented by an independent `pthread`, cycling through the
states `WAITING_FOR_DONGLES → COMPILING → DEBUGGING → REFACTORING`, and a
dedicated monitor thread continuously supervises the simulation for two
terminal conditions: a coder exceeding `time_to_burnout` milliseconds since
the start of its last compile (failure), or every coder having completed at
least `number_of_compiles_required` compilations (success).

The program accepts two mutually exclusive scheduling disciplines for
resolving contention on a given dongle:

- **FIFO** — requests are served in strict arrival order, implemented via a
  monotonically increasing arrival counter per dongle.
- **EDF** (Earliest Deadline First) — for a coder *c_i* whose most recent
  compile phase began at time *s_i*, its implicit burnout deadline is defined
  as

  ```
  D_i = s_i + time_to_burnout
  ```

  and, among all coders currently waiting on a given dongle, the one
  minimising *D_i* is granted access first. Ties (which can occur at
  millisecond granularity even though they are a measure-zero event in
  continuous time) are broken deterministically on ascending coder id, to
  guarantee that the resulting total order is well-defined.

Both policies are implemented atop a hand-rolled binary min-heap
(`t_heap`), as required by the subject — no standard-library priority queue
is used.

## 2. Instructions

### 2.1 Compilation

The project is built with the GNU `make` utility. The Makefile enforces
`-Wall -Wextra -Werror -pthread` and exposes the mandatory targets:

```sh
make        # builds the "codexion" binary
make clean  # removes intermediate object files
make fclean # removes object files and the binary
make re     # fclean, then a full rebuild
```

Every object file also depends on `include/codexion.h`, so a change to the
shared header correctly triggers a full recompilation rather than leaving
stale, mismatched object files linked into the binary.

### 2.2 Execution

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument                     | Unit | Description                                                                 |
|-------------------------------|------|-------------------------------------------------------------------------------|
| `number_of_coders`            | —    | Number of coder threads, and number of dongles                              |
| `time_to_burnout`              | ms   | Maximum tolerated interval without starting a new compile                   |
| `time_to_compile`              | ms   | Duration of the compiling phase                                              |
| `time_to_debug`                | ms   | Duration of the debugging phase                                              |
| `time_to_refactor`             | ms   | Duration of the refactoring phase                                            |
| `number_of_compiles_required`  | —    | Compiles per coder needed for a successful termination (must be ≥ 1)         |
| `dongle_cooldown`               | ms   | Minimum delay before a released dongle can be reacquired by anyone           |
| `scheduler`                    | —    | Arbitration policy: exactly `fifo` or `edf`                                  |

All numeric arguments are validated as non-negative, overflow-checked
integers; malformed input (non-digits, empty strings, values exceeding the
representable range, or a `number_of_coders` below 1) causes the program to
reject the input and exit rather than proceed on undefined state.

Example — an infeasible configuration, in which a lone coder possesses a
single dongle but requires two, and is therefore guaranteed to burn out:

```sh
./codexion 1 800 200 200 200 10 0 fifo
```

Example — a feasible, contention-heavy configuration under EDF, expected to
complete all required compiles without any burnout:

```sh
./codexion 5 2000 200 200 200 7 0 edf
```

## 3. Resources

### 3.1 References

- E. W. Dijkstra, *Hierarchical Ordering of Sequential Processes*, 1971 —
  origin of the Dining Philosophers problem and of resource-ordering as a
  deadlock-avoidance technique.
- E. G. Coffman, M. Elphick, A. Shoshani, *System Deadlocks*, ACM Computing
  Surveys, 1971 — formalisation of the four necessary conditions for
  deadlock referenced in §4.
- C. L. Liu, J. W. Layland, *Scheduling Algorithms for Multiprogramming in a
  Hard-Real-Time Environment*, Journal of the ACM, 1973 — theoretical basis
  for Earliest-Deadline-First scheduling.
- T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, *Introduction to
  Algorithms* — binary heap construction and maintenance, used as the
  reference for the priority-queue implementation.
- `pthread_mutex_lock(3)`, `pthread_cond_wait(3)`, `pthread_cond_timedwait(3)`,
  `gettimeofday(2)` — POSIX manual pages consulted for the exact memory and
  scheduling semantics of the primitives used throughout the project.

### 3.2 Use of AI

An AI assistant (Claude, Anthropic) was used strictly as a debugging and
review aid over already-drafted code, for the following, specific tasks:

- Diagnosing a segmentation fault traced to a dongle's wait-queue heap
  (`t_dongle.heap`) being referenced by `dongle_acquire` without ever having
  been allocated in `dongle_init`, and proposing the corresponding allocation
  and teardown logic.
- Diagnosing a linker failure (duplicate symbol / missing symbol) caused by
  stale object files after a source-level split of dongle logic out of
  `coder.c`, and clarifying that a `make fclean && make` was required.
- Reviewing argument parsing for robustness: identifying an unguarded
  integer-overflow path and the absence of a lower bound on
  `number_of_coders`, and proposing the corresponding checks.
- Diagnosing a liveness/termination bug in which coder threads already inside
  a compile/debug/refactor cycle would continue logging further state
  transitions after a peer thread had already burned out, because their
  blocking sleep could not be interrupted by the stop signal, and proposing
  the interruptible-sleep primitive (`sim_sleep_ms`) used to resolve it.
- Empirically validating the resulting behaviour (burnout timing precision,
  cooldown enforcement, FIFO/EDF grant-order divergence, absence of dongle
  duplication) against repeated executions of representative parameter sets.

All AI-assisted changes were reviewed, tested, and are fully understood by
the author prior to submission, in accordance with the AI usage guidelines
of the curriculum.

## 4. Blocking cases handled

### 4.1 Deadlock prevention and Coffman's conditions

A deadlock requires the simultaneous presence of four conditions (Coffman et
al., 1971): mutual exclusion, hold-and-wait, no preemption, and circular
wait. The first three are inherent to the problem statement and cannot be
relaxed without violating the subject (a dongle genuinely cannot be shared,
a coder genuinely holds one dongle while awaiting the other, and no dongle
may be forcibly revoked). The implementation therefore targets the fourth
condition.

Dongles are indexed `0..n-1`; each coder's two neighbouring dongles are
acquired in strictly ascending index order (`acquire_pair`), regardless of
which is conventionally the coder's "left" or "right" resource. This imposes
a fixed total order on resource acquisition across the entire system: if
every thread only ever requests a higher-indexed resource while already
holding a lower-indexed one, the *wait-for* relation among threads cannot
contain a cycle, and deadlock is precluded by the standard resource-ordering
argument (Havender, 1968).

*Known limitation.* This technique guarantees safety, not fairness. In a
cyclic topology, exactly one coder — the one whose neighbourhood wraps
around index `0` — must reverse its natural acquisition order, which causes
dongle `0` to be the first-choice target of two coders instead of one,
introducing a structural, topology-induced contention asymmetry. This does
not threaten correctness (no duplication, no deadlock is ever observed), but
it was empirically measured to bias burnout risk, under tight parameters,
towards the coders adjacent to that dongle.

### 4.2 Starvation prevention and cooldown handling

Under FIFO, each dongle maintains a strictly increasing arrival counter; a
waiting coder is only granted the dongle once it is the minimum-key entry of
that dongle's wait-heap, guaranteeing bounded waiting in arrival order.
Under EDF, the same heap is keyed by the deadline `D_i` defined in §1,
biasing the grant order towards whichever coder is closest to burning out —
directly mitigating starvation risk for a coder that has been unlucky in
prior rounds.

Cooldown is enforced by an `available_at_ms` timestamp on the dongle,
updated on release; a waiting coder that already holds priority in the
wait-heap but finds the dongle within its cooldown window blocks on
`pthread_cond_timedwait` until exactly that deadline, rather than busy-
polling, and is re-evaluated automatically once it elapses.

### 4.3 Precise burnout detection

A dedicated monitor thread samples every coder's `last_compile_start_ms`
once per millisecond and compares the elapsed time against
`time_to_burnout`, guaranteeing that a burnout is detected — and its log
line printed — within a bounded latency of the actual deadline, well inside
the 10 ms tolerance required by the subject.

### 4.4 Prompt, race-free termination

A subtlety not immediately visible from the burnout-detection requirement
alone is that detecting a burnout is not sufficient: every other coder
thread must also stop making further progress promptly. A plain `usleep`
call cannot be interrupted, so a coder already mid-phase when a peer burns
out would otherwise run its remaining phases — and log them — to completion.
This is resolved by decomposing every phase's sleep into 1 ms increments
(`sim_sleep_ms`), each followed by a check of the shared stop flag, and by
inserting an equivalent check between the compiling and debugging/refactoring
phases in the coder's control loop. Once a stop is requested, no coder
thread logs, or is credited with, any further state transition.

### 4.5 Log serialization

All state-change messages are written through a single function
(`log_state`) guarded by one mutex shared by every coder thread and the
monitor thread, so that the sequence "compute timestamp → format → write" is
executed as an indivisible critical section; no two log lines can ever be
interleaved on the same line of output.

## 5. Thread synchronization mechanisms

| Primitive | Scope | Protects |
|---|---|---|
| `pthread_mutex_t lock` (per dongle) | one dongle | `in_use`, `available_at_ms`, the dongle's wait-heap |
| `pthread_cond_t cond` (per dongle) | one dongle | blocks a waiting coder until woken by a release, a cooldown deadline, or a simulation stop |
| `pthread_mutex_t state_lock` (per coder) | one coder | `state`, `last_compile_start_ms`, `compiles_done` |
| `pthread_mutex_t stop_lock` (global) | whole simulation | the boolean stop flag |
| `pthread_mutex_t log_lock` (global) | whole simulation | serializes every `printf` call |

**Mutual exclusion on a dongle.** Consider two coders whose ring
neighbourhood shares a common dongle *d*. Absent synchronization, both
threads could observe `d.in_use == false` concurrently, and both proceed to
compile while physically holding the same dongle — a duplication that would
violate the subject's core invariant. By requiring every read-then-write of
`d.in_use` to occur while holding `d.lock`, the check-then-act sequence
(`is it free and off cooldown and am I first in the wait-heap? → mark it in
use`) becomes an atomic operation: only one thread can observe the resource
as available and transition it before releasing the mutex, so no second
thread can ever observe the same "available" window.

**Blocking without busy-waiting.** A coder that loses the race for a dongle
calls `pthread_cond_wait` (or `pthread_cond_timedwait`, when it must wait
out a known cooldown deadline) on `d.cond`, atomically releasing `d.lock`
while it sleeps. `dongle_release` re-acquires the same mutex, updates
`available_at_ms`, and calls `pthread_cond_broadcast`, ensuring every waiter
re-checks the predicate under the lock before deciding whether to proceed or
resume waiting — the standard, spurious-wakeup-safe consumer pattern.

**Thread-safe communication between coders and the monitor.** The monitor
thread reads `last_compile_start_ms` for every coder once per millisecond,
concurrently with each coder's own thread writing that same field at the
start of its compile phase. Because both the write (in `coder_compile`) and
the read (in the monitor) occur while holding the same `state_lock`, the
POSIX mutex semantics establish a happens-before relation between the
unlock following the write and the subsequent lock preceding the read
(Lamport, 1978): the monitor is therefore guaranteed to observe an
up-to-date value, never a torn or stale one, without any additional memory
barrier.

**Global stop signalling.** `sim_request_stop` is the single writer of the
stop flag (under `stop_lock`) and, in the same call, broadcasts every
dongle's condition variable, so that a coder blocked indefinitely on
`pthread_cond_wait` for a dongle it will now never need is woken
immediately, re-checks the stop flag, and exits its acquisition loop rather
than waiting until program termination.