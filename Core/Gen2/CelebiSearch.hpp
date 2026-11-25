/*
 * GS Ball Celebi (Crystal VC): leaf grids (a plan with press errors on 6th / YES / final, whole timeline shifted by a timer
 * offset X), the F-scored shiny search (fsearch), plan 1 / plan 2 (pick), hidden play-time coverage and clock windows.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's celebi_tool.py v0.6 (_grid_X, _fgrid_job, fgrid_jobs, scan_final, coverage, _fs_job,
 * fs_jobs, pick, sweep, windows, run_jobs). Checked against the prototype (Test/Gen2).
 */

#ifndef CELEBISEARCH_HPP
#define CELEBISEARCH_HPP

#include <Core/Gen2/CelebiPlan.hpp>
#include <atomic>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace Gen2
{
    using Enemy = std::array<std::uint8_t, 8>;

    /// Celebi's DVs (Atk/Def, Spe/Spc) as the prototype's 4-hex string bytes 6..7.
    inline std::uint16_t dvOf(const Enemy &e)
    {
        return static_cast<std::uint16_t>((e[6] << 8) | e[7]);
    }
    /// Gen 2 shininess: Def, Spe, Spc 10 and Atk in {2, 3, 6, 7, 10, 11, 14, 15}.
    bool dvShiny(std::uint16_t dv);
    inline bool isShiny(const Enemy &e)
    {
        return dvShiny(dvOf(e));
    }
    std::string hex(const Enemy &e);
    std::string dvHex(std::uint16_t dv);

    /// Leaves of one timer offset X: DVs for press errors (e6, eYES, efinal) in [-D, D] (0000 when no encounter).
    struct Grid
    {
        int X = 0, D = 0;
        std::vector<std::uint16_t> dv; // (2D+1)^3, index ((e6 + D) * W + eYES + D) * W + efinal + D
        int width() const
        {
            return 2 * D + 1;
        }
        std::uint16_t at(int e6, int ey, int ef) const
        {
            int w = width();
            return dv[((e6 + D) * w + ey + D) * w + ef + D];
        }
    };

    /// The prototype's _grid_X: whole plan shifted by X, errors on 6th / YES / final in [-D, D].
    Grid gridX(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, const Plan &p, int d6, int dyes, int final, int X, int D);
    /// The prototype's _fgrid_job: the save with play-time frame counter F, then gridX.
    Grid fgridJob(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final, const Rtc &rtc, int F, int D = 2, int X = 0);

    /// Final A at each VBlank of [lo, hi): the enemy bytes, or empty when no encounter within 1,500 frames.
    std::vector<std::optional<Enemy>> scanFinal(const RomPtr &rom, const Bytes &sram, const Rtc &rtc, const Events &ev, int lo, int hi,
                                                int hold = 5);

    struct Coverage
    {
        std::vector<int> F, s; // values (0-59) of wGameTimeFrames / wGameTimeSeconds for which the plan stays shiny
    };
    Coverage coverage(const RomPtr &rom, const Bytes &sram, const Plan &p, int d6, int dyes, int final, const Rtc &rtc);

    /// A shiny (6th, YES, final) combination with its F and s coverage.
    struct Hit
    {
        int d6, dyes, final;
        Rtc rtc;
        Enemy enemy;
        std::vector<int> fOk, sOk;
        bool operator==(const Hit &) const = default;
    };

    struct FsJob
    {
        std::string key; // fs_<d6>_<dyes>_<lo>_<hhmm>
        int d6, dyes, lo, hi;
        Rtc rtc;
    };
    /// (6th, YES) pairs by distance from the plan; per pair the whole final-A range (chunks of W) at each clock.
    std::vector<FsJob> fsJobs(const Plan &p, const std::vector<Rtc> &rtcs, int D = 3, int off = 40, int W = 100, int chunks = 20);
    /// The prototype's _fs_job: shinies in the job's final range, each scored over F and s.
    std::vector<Hit> fsJob(const RomPtr &rom, const Bytes &sram, const Plan &p, const FsJob &job);

    /// plan 1 = best F coverage (then s); plan 2 = most F values plan 1 misses (then its own coverage). Indices into hits.
    std::pair<std::optional<std::size_t>, std::optional<std::size_t>> pick(const std::vector<Hit> &hits);

    struct FgridJob
    {
        std::string key; // f_<plan>_<FF>[_x<X>]
        int plan, F, D, X;
    };
    /// The switch-rule grids: for each plan, F 0-59 at timer offset X.
    std::vector<FgridJob> fgridJobs(int nplans, int D = 2, int X = 0);

    using Progress = std::function<void(int done, int total)>;

    struct FsearchResult
    {
        std::vector<std::pair<std::string, std::vector<Hit>>> jobs; // committed jobs, in job order
        int committed = 0, total = 0;
        bool stopped = false; // the target was met
        /// all hits in job-key order (the prototype's all_hits over a job directory)
        std::vector<Hit> hits() const;
    };
    /**
     * @brief fsearch on `threads` threads: jobs committed in job order; stops when plan 1 covers `target` F values and plans
     * 1 + 2 together `unionTarget` (0 = ignore), as the prototype's fsearch. `done` holds jobs already computed (resume).
     */
    FsearchResult fsearch(const RomPtr &rom, const Bytes &sram, const Plan &p, const std::vector<FsJob> &jobs, int target, int unionTarget,
                          int threads = 0, const Progress &progress = {},
                          const std::map<std::string, std::vector<Hit>> &done = {});

    /// The switch-rule grids of plans (each F 0-59) at timer offsets Xs, on `threads` threads: (plan, F, X) -> grid.
    std::map<std::array<int, 3>, Grid> switchGrids(const RomPtr &rom, const Bytes &sram, const Plan &p, const std::vector<Hit> &plans, int D,
                                                   const std::vector<int> &Xs, int threads = 0, const Progress &progress = {});

    /// Enemy bytes for each of the 1,440 chip minutes (seconds 30, day 117).
    std::vector<std::optional<Enemy>> sweep(const RomPtr &rom, const Bytes &sram, const Events &ev, int threads = 0);
    /// In-game windows ("hh:mm-hh:mm") where pred holds, for a clock offset in minutes.
    std::vector<std::string> windows(const std::vector<std::optional<Enemy>> &res, int offsetMin,
                                     const std::function<bool(const std::optional<Enemy> &)> &pred = {});

    /// Runs fn(i) for i in [0, n) on `threads` threads (0 = hardware concurrency).
    void parallelFor(int n, int threads, const std::function<void(int)> &fn);

    /**
     * @brief Runs jobs in parallel and commits their results in job order, as the prototype's run_jobs (Pool.imap):
     * stop(committed) is checked after each commit in order; results after the stop are discarded.
     * @return number of committed results
     */
    template <class R>
    int runJobsOrdered(int n, int threads, const std::function<R(int)> &fn, std::vector<std::optional<R>> &out,
                       const std::function<bool(int)> &stop = {})
    {
        out.assign(n, std::nullopt);
        std::vector<std::optional<R>> buf(n);
        std::vector<std::atomic<bool>> ready(n);
        std::atomic<int> next { 0 };
        std::atomic<bool> halt { false };
        int nt = threads > 0 ? threads : std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> pool;
        for (int t = 0; t < nt; t++)
        {
            pool.emplace_back([&] {
                for (int i; !halt && (i = next++) < n;)
                {
                    buf[i] = fn(i);
                    ready[i].store(true, std::memory_order_release);
                    ready[i].notify_all();
                }
            });
        }
        int committed = 0;
        for (; committed < n; committed++)
        {
            ready[committed].wait(false, std::memory_order_acquire);
            out[committed] = std::move(buf[committed]);
            if (stop && stop(committed))
            {
                committed++;
                break;
            }
        }
        halt = true; // workers exit after their current job
        for (auto &th : pool)
        {
            th.join();
        }
        for (int i = committed; i < n; i++)
        {
            out[i].reset();
        }
        return committed;
    }
}

#endif // CELEBISEARCH_HPP
