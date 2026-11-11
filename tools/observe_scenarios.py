#!/usr/bin/env python3
"""Run the three fixed CLI scenarios and verify their exported daily observations."""
import argparse
import csv
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess


def observe(program, output, days, source_commit):
    output.mkdir(parents=True, exist_ok=True)
    (output / "observations.json").unlink(missing_ok=True)
    result = {"source_commit": source_commit,
              "observed_at_jst": datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat(),
              "binary_sha256": hashlib.sha256(program.read_bytes()).hexdigest(),
              "days": days, "scenarios": {}}
    for scenario in ("balanced", "shortage", "oversupply"):
        csv_path = output / f"{scenario}.csv"
        command = [str(program), "--scenario", scenario, "--days", str(days), "--csv", str(csv_path)]
        run = subprocess.run(command, text=True, capture_output=True, check=False)
        (output / f"{scenario}.stdout.log").write_text(run.stdout, encoding="utf-8")
        (output / f"{scenario}.stderr.log").write_text(run.stderr, encoding="utf-8")
        if run.returncode != 0 or run.stderr:
            raise RuntimeError(f"{scenario} failed: exit={run.returncode}, stderr={run.stderr}")
        with csv_path.open(encoding="utf-8", newline="") as source:
            reader = csv.DictReader(source)
            if reader.fieldnames != ["day", "price_index", "change_percent", "total_money"]:
                raise ValueError(f"{scenario}: unexpected CSV columns")
            rows = list(reader)
        if len(rows) != days:
            raise ValueError(f"{scenario}: expected {days} observations, got {len(rows)}")
        prices = [int(price) for price in re.findall(r"小麦: \d+個 \(価格: (\d+)コイン\)", run.stdout)]
        if len(prices) != days:
            raise ValueError(f"{scenario}: daily market price count differs from CSV")
        initial = re.search(r"開始時の総資金: (\d+)コイン", run.stdout)
        final = re.search(r"終了時の総資金: (\d+)コイン", run.stdout)
        if not initial or not final or initial[1] != "7000" or final[1] != initial[1]:
            raise ValueError(f"{scenario}: cash conservation failed")
        previous = 100.0
        indices = []
        for day, (row, price) in enumerate(zip(rows, prices), 1):
            index, change = float(row["price_index"]), float(row["change_percent"])
            expected_index = price / 10.0 * 100.0
            expected_change = (index / previous - 1.0) * 100.0
            if int(row["day"]) != day or int(row["total_money"]) != 7000:
                raise ValueError(f"{scenario}: wrong day or cash on day {day}")
            if not math.isfinite(index) or index <= 0 or not math.isfinite(change):
                raise ValueError(f"{scenario}: invalid price observation on day {day}")
            if not math.isclose(index, expected_index, rel_tol=1e-12, abs_tol=1e-10):
                raise ValueError(f"{scenario}: index differs from market price on day {day}")
            if not math.isclose(change, expected_change, rel_tol=1e-12, abs_tol=1e-10):
                raise ValueError(f"{scenario}: daily change differs on day {day}")
            previous = index
            indices.append(index)
        purchases = re.findall(r"が小麦を(\d+)コインで購入しました。", run.stdout)
        wages = re.findall(r"が(\d+)コインの収入を得ました。", run.stdout)
        hungry = len(re.findall(r"ポイント低下し", run.stdout))
        if len(purchases) + hungry != 2 * days:
            raise ValueError(f"{scenario}: food activity does not account for both NPCs each day")
        summary = {"exit_code": run.returncode, "stderr_bytes": len(run.stderr.encode()),
                   "observations": len(rows), "first_three_indices": indices[:3],
                   "last_index": indices[-1], "max_index": max(indices),
                   "max_index_day": indices.index(max(indices)) + 1,
                   "min_index": min(indices), "min_index_day": indices.index(min(indices)) + 1,
                   "initial_cash": int(initial[1]), "final_cash": int(final[1]),
                   "purchase_count": len(purchases), "purchase_value": sum(map(int, purchases)),
                   "wage_value": sum(map(int, wages)),
                   "hungry_person_days": hungry,
                   "loan_count": len(re.findall(r"が100コインの融資を受けました。", run.stdout))}
        result["scenarios"][scenario] = summary
        blocks = re.split(r"\n=== Day (\d+) ===\n", run.stdout)
        excerpt = blocks[0] + "\n"
        selected = sorted({1, days})
        for day in selected:
            excerpt += f"\n=== Day {day} ===\n" + blocks[day * 2]
        (output / f"{scenario}.excerpt.log").write_text(excerpt, encoding="utf-8")
    (output / "observations.json").write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return result


def plot(output, days):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    matplotlib.rcParams["svg.hashsalt"] = "economy-observations"
    colors = {"balanced": "#2563eb", "shortage": "#d97706", "oversupply": "#15803d"}
    figure, axes = plt.subplots(1, 2, figsize=(12, 4.6), constrained_layout=True)
    for scenario, color in colors.items():
        with (output / f"{scenario}.csv").open(encoding="utf-8") as source:
            rows = list(csv.DictReader(source))
        xs = [int(row["day"]) for row in rows]
        ys = [float(row["price_index"]) for row in rows]
        for axis in axes:
            axis.plot(xs, ys, label=scenario, color=color, linewidth=2.2)
    for axis in axes:
        axis.axhline(100, color="#64748b", linestyle="--", linewidth=0.8, zorder=0)
        axis.set_xlabel("Day")
        axis.set_ylabel("Fixed-basket price index (base = 100)")
        axis.grid(alpha=0.18)
        axis.spines[["top", "right"]].set_visible(False)
    axes[0].set_title(f"{days}-day observations")
    axes[0].set_xlim(1, max(2, days))
    axes[0].legend(frameon=False)
    axes[1].set_title("First 10 days")
    axes[1].set_xlim(1, min(10, max(2, days)))
    early = []
    for scenario in colors:
        with (output / f"{scenario}.csv").open(encoding="utf-8") as source:
            early += [float(row["price_index"]) for row in list(csv.DictReader(source))[:10]]
    axes[1].set_ylim(max(0, min(early) - 15), max(early) + 20)
    figure.savefig(output / "price-indices.svg", metadata={"Date": None})
    figure.savefig(output / "price-indices.png", dpi=150)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--days", type=int, default=100)
    parser.add_argument("--plot", action="store_true", help="Requires matplotlib")
    args = parser.parse_args()
    if not 1 <= args.days <= 10000:
        parser.error("days must be from 1 to 10000")
    if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        parser.error("source-commit must be a full commit SHA")
    result = observe(args.program.resolve(strict=True), args.output.resolve(), args.days, args.source_commit)
    if args.plot:
        plot(args.output.resolve(), args.days)
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
