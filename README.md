# Performance Logging System Benchmarks

Attempts to log performance events affecting the main application performance as little as possible.

# Benchmarks

System:
- CPU: Ryzen 5950x (16 phisical cores, 3400GHz)
- OS: Ubuntu 22.04 LTS, disabled SMT, no turbo boost, isolated cores in full no HZ mode.
- Benchmarks are ran on isolated cores with custom init program (no other programs except bash and kernel threads are running in the system).

## Semi-lock-free performance log (`SLFPL`)

| Threads | Log batch size | Logs per second AVG (millions) | CV |
| - | - | - | - |
| 1 | 1000000 | 33.5±0.10/0.30% (base) | 0.07% |
| 2 | 1000000 | 33.2±0.08/0.23% ($\color{RedOrange}-0.95％$) | 0.09% |
| 4 | 1000000 | 33.1±0.10/0.32% ($\color{RedOrange}-1.17％$) | 0.13% |
| 8 | 1000000 | 32.7±0.13/0.41% ($\color{red}-2.41％$) | 0.18% |

## Thread-local performance log (`TLPL`)

| Threads | Log batch size | Logs per second AVG (millions) | CV |
| - | - | - | - |
| 1 | 1000000 | | |
| 2 | 1000000 | | |
| 4 | 1000000 | | |
| 8 | 1000000 | | |

## TODO

- Measure the logging overhead in cpu-intensive tasks.
- Measure the logging overhead in memory-intensive tasks and reduce it using non-temporal memory operations if possible (L1, L2, L3, RAM, random accesses, prefetcher-friendly accesses).

