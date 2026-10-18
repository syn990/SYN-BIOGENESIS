# drift: how long one seed stays one world

`drift.py` runs the shipped Biogenesis 0.9 jar on OpenJDK 27 (the
reference) and these on the same seeds, then compares their state on every
frame:

| Variant | What it is |
|---|---|
| `port` | SYN-BIOGENESIS (`syn-biogenesis --headless`) |
| `port-libc` | SYN-BIOGENESIS on the C library's `sin`/`cos`/`pow` (`make libc`), as on a processor other than x86_64 |
| `java8`, `java11`, `java17`, `java21` | the same jar on other HotSpot versions (Java 8 from the system, the rest from `fetch-jdks.sh`) |
| `java27-fdlibm` | the same jar on Java 27 with `-XX:-UseLibmIntrinsic`: `Math` computed by fdlibm, as HotSpot does on processors it has no Intel routines for |

Each run writes one line per frame with a hash of the state that
`tools/javaref` dumps (`HashRun.java`, `syn-biogenesis --headless --hash`).
For each seed and variant it records the first frame that differs, how
many differences healed (the worlds became identical again), and the
frame from which they never agree again. For the first ten seeds that part
for good in time, it then takes full dumps and follows how far the two
worlds move apart.

```sh
tools/javaref/fetch.sh          # once
tools/drift/fetch-jdks.sh       # optional: Java 11, 17, 21
tools/drift/drift.py            # 100 seeds x 20,000 frames; about half an hour on 6 cores
tools/drift/summary.py          # the README's table
```

Results go to `docs/drift/` (CSV and the README's charts, drawn by
`charts.py`).
