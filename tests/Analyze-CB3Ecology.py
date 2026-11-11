"""Seed-level paired analysis. No tick is treated as an independent replicate."""
import argparse
import csv
import json
import math
from pathlib import Path
import random
import statistics as stats
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAPS = 10000
INDEX_CACHE = {}
SCENARIOS = {"normal":"Обычный мир", "cluster":"Скопление", "scarce":"Дефицит", "crisis":"Кризис", "neutral":"Нейтральные группы"}
METRICS = {"bots":"Поздняя численность", "final_bots":"Численность на тике 600",
           "brain_effective":"Разнообразие initial-мозга", "founder_effective":"Разнообразие основателей",
           "mean_energy":"Средняя энергия, ед.", "low_energy_pct":"Мало энергии, п.п.",
           "ps_pct":"Фотосинтетики, п.п.", "predator_pct":"Хищники, п.п.", "organic_pct":"Органическое питание, п.п.",
           "mixed_pct":"Смешанное питание, п.п.", "unknown_pct":"Неизвестное питание, п.п.",
           "recovery_ticks":"Время восстановления, тики", "left_pct":"Исходно левая группа, п.п.",
           "team_effective":"Разнообразие нейтральных групп", "translation_population":"Изменение эффекта сдвига: численность",
           "translation_left":"Изменение эффекта сдвига: левая группа, п.п."}
EVENTS = {"extinct":"Вымирание", "bottleneck":"Провал ниже 1% стартовой численности",
          "predator_absent":"Нет специализированных хищников в позднем окне", "not_recovered":"Нет восстановления к тику 600"}


def condition(row):
    return SCENARIOS[row["scenario"]] + (f", сдвиг +{row['shift']}" if row["shift"] else "")


def bootstrap(values):
    n = len(values)
    if not n:
        return math.nan, math.nan, math.nan
    if n not in INDEX_CACHE:
        rng = random.Random(20261111)
        INDEX_CACHE[n] = [[rng.randrange(n) for _ in range(n)] for _ in range(BOOTSTRAPS)]
    estimates = sorted(sum(values[i] for i in indices) / n for indices in INDEX_CACHE[n])
    def q(p):
        index = (BOOTSTRAPS - 1) * p
        lo = int(index)
        return estimates[lo] + (estimates[min(lo+1, BOOTSTRAPS-1)] - estimates[lo]) * (index-lo)
    return stats.mean(values), q(.025), q(.975)


def wilson(k, n):
    z = 1.959963984540054
    p = k / n
    center = (p + z*z/(2*n)) / (1 + z*z/n)
    half = z * math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / (1 + z*z/n)
    return max(0, center-half), min(1, center+half)


def load(path, expected):
    rows = list(csv.DictReader(path.open(encoding="utf-8-sig", newline="")))
    groups = defaultdict(list)
    initials = {}
    for row in rows:
        seed, shift = int(row["seed"]), int(row["shift"])
        initial_key = (row["scenario"], seed)
        assert initials.setdefault(initial_key, row["initial_sha256"]) == row["initial_sha256"]
        key = (row["scenario"], shift, row["variant"], seed)
        groups[key].append({k: float(v) for k, v in row.items() if k not in ("scenario", "variant", "initial_sha256")})
    assert len(groups) == expected * 14, f"Incomplete study: {len(groups)} / {expected*14} runs"
    # These scenarios have identical physics until the intervention at tick 251.
    # Their repeated prefix tests reproducibility without counting it as new seeds.
    seeds = {key[3] for key in groups}
    for seed in seeds:
        assert initials[("normal", seed)] == initials[("crisis", seed)]
        for variant in ("before", "after"):
            normal = sorted(groups[("normal",0,variant,seed)],key=lambda r:r["tick"])
            crisis = sorted(groups[("crisis",0,variant,seed)],key=lambda r:r["tick"])
            for old,new in zip(normal,crisis):
                if old["tick"] > 250: break
                for name in old:
                    assert old[name] == new[name] or (math.isnan(old[name]) and math.isnan(new[name])), (seed,variant,name)
    result = {}
    for key, samples in groups.items():
        samples.sort(key=lambda r: r["tick"])
        assert [r["tick"] for r in samples] == list(range(0, 601, 50)), key
        initial = samples[0]["bots"]
        previous_founders = initial
        for r in samples:
            assert r["bots"] == initial + r["births"] - r["deaths"] - r["crisis_removed"], key
            assert r["founder_richness"] <= previous_founders
            previous_founders = r["founder_richness"]
            if r["bots"]:
                assert abs(sum(r[k] for k in ("ps_pct", "predator_pct", "organic_pct", "mixed_pct", "unknown_pct"))-100) < 1e-5
                for name in ("brain", "founder"):
                    assert 1-1e-6 <= r[name+"_effective"] <= r[name+"_richness"]+1e-6
                assert 0 <= r["mean_energy"] <= 500
                if key[0] == "neutral":
                    assert r["brain_richness"] == 1 and 0 <= r["left_pct"] <= 100
        late = [r for r in samples if r["tick"] >= 450]
        aggregate = {k: stats.mean(r[k] for r in late) for k in
                     ("bots", "brain_effective", "founder_effective", "mean_energy", "mean_age", "low_energy_pct",
                      "ps_pct", "predator_pct", "organic_pct", "mixed_pct", "unknown_pct", "left_pct")}
        aggregate["final_bots"] = samples[-1]["bots"]
        aggregate["extinct"] = samples[-1]["extinction_tick"] >= 0
        aggregate["bottleneck"] = samples[-1]["min_bots"] < initial * .01
        aggregate["recovery_ticks"] = samples[-1]["recovery_ticks"]
        aggregate["not_recovered"] = key[0] == "crisis" and samples[-1]["recovery_ticks"] < 0
        aggregate["predator_absent"] = aggregate["predator_pct"] == 0
        if key[0] == "neutral":
            aggregate["team_effective"] = stats.mean(1 / ((r["left_pct"]/100)**2 + (1-r["left_pct"]/100)**2) for r in late)
        result[key] = aggregate
    return result


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--csv", default="CB3_ECOLOGY_RESULTS.csv")
    p.add_argument("--seeds", type=int, default=20)
    p.add_argument("--output-dir", default=".")
    args = p.parse_args()
    output = ROOT / args.output_dir
    output.mkdir(parents=True, exist_ok=True)
    data = load(ROOT / args.csv, args.seeds)
    seeds = sorted({key[3] for key in data})
    assert len(seeds) == args.seeds
    summary = []

    def effect(case, shift, metric, kind, lower, upper, before=None, after=None):
        b = before if before is not None else [data[(case, shift, "before", s)][metric] for s in seeds]
        a = after if after is not None else [data[(case, shift, "after", s)][metric] for s in seeds]
        values = []
        for old, new in zip(b, a):
            if not math.isfinite(old) or not math.isfinite(new):
                continue
            if kind == "ratio":
                if old <= 0 or new <= 0: continue
                values.append(math.log(new/old))
            elif metric == "recovery_ticks" and (old < 0 or new < 0):
                continue
            else:
                values.append(new-old)
        center, lo, hi = bootstrap(values)
        if kind == "ratio" and values:
            center, lo, hi = map(math.exp, (center, lo, hi))
        if len(values) != len(seeds): status = "incomplete"
        elif metric in ("predator_pct", "organic_pct", "mixed_pct") and all(v == 0 for v in b+a):
            status = "not_observed"
        elif lo >= lower and hi <= upper: status = "equivalent"
        elif hi < lower or lo > upper: status = "outside"
        else: status = "uncertain"
        summary.append(dict(scenario=case, shift=shift, metric=metric, kind=kind, pairs=len(values),
                            before_mean=stats.mean(b), after_mean=stats.mean(a), effect=center,
                            ci_low=lo, ci_high=hi, margin_low=lower, margin_high=upper, status=status))

    for case, shift in [("normal", 0), ("cluster", 0), ("scarce", 0), ("crisis", 0), ("normal", 359), ("neutral", 0), ("neutral", 359)]:
        for metric in ("bots", "final_bots"):
            effect(case, shift, metric, "ratio", .9, 1.1)
        if case != "neutral":
            for metric in ("brain_effective", "founder_effective"):
                effect(case, shift, metric, "ratio", .8, 1.2)
        for metric in ("ps_pct", "predator_pct", "organic_pct", "mixed_pct", "unknown_pct", "low_energy_pct"):
            effect(case, shift, metric, "difference", -5, 5)
        effect(case, shift, "mean_energy", "difference", -25, 25)
        if case == "crisis": effect(case, shift, "recovery_ticks", "difference", -50, 50)
        if case == "neutral":
            effect(case, shift, "left_pct", "difference", -5, 5)
            effect(case, shift, "team_effective", "ratio", .8, 1.2)

    # Difference of translation effects, matched on seed and initial world.
    for case in ("normal", "neutral"):
        b, a = [], []
        for variant, target in (("before", b), ("after", a)):
            for s in seeds:
                zero = data[(case, 0, variant, s)]["bots"]
                moved = data[(case, 359, variant, s)]["bots"]
                target.append(moved/zero if zero > 0 else math.nan)
        effect(case, 359, "translation_population", "ratio", .9, 1.1, b, a)
    b, a = [], []
    for variant, target in (("before", b), ("after", a)):
        for s in seeds:
            target.append(data[("neutral", 359, variant, s)]["left_pct"] - data[("neutral", 0, variant, s)]["left_pct"])
    effect("neutral", 359, "translation_left", "difference", -5, 5, b, a)

    with (output / "CB3_ECOLOGY_SUMMARY.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
        writer.writeheader(); writer.writerows(summary)
    events = []
    for case, shift in sorted({(key[0], key[1]) for key in data}):
        for name in ("extinct", "bottleneck", "predator_absent", "not_recovered"):
            if name == "not_recovered" and case != "crisis": continue
            if name == "predator_absent" and case == "neutral": continue
            b = [data[(case, shift, "before", s)][name] for s in seeds]
            a = [data[(case, shift, "after", s)][name] for s in seeds]
            events.append(dict(scenario=case, shift=shift, event=name, before=sum(b), after=sum(a),
                               before_ci=wilson(sum(b),len(b)), after_ci=wilson(sum(a),len(a)),
                               new_only=sum(new and not old for old,new in zip(b,a)),
                               old_only=sum(old and not new for old,new in zip(b,a))))
    details = ROOT / "x64/ecology/analysis.json" if args.output_dir == "." else output / "analysis.json"
    details.write_text(json.dumps(dict(effects=summary, events=events),indent=2))

    def formatted(r):
        if r["kind"] == "ratio":
            return f"{(r['effect']-1)*100:+.1f}% [{(r['ci_low']-1)*100:+.1f}; {(r['ci_high']-1)*100:+.1f}]"
        return f"{r['effect']:+.2f} [{r['ci_low']:+.2f}; {r['ci_high']:+.2f}]"
    labels = {"equivalent":"В допуске", "outside":"За допуском", "uncertain":"Недостаточная точность",
              "incomplete":"Есть неполные пары", "not_observed":"Стратегия не наблюдалась"}
    counts = {status: sum(r["status"] == status for r in summary) for status in labels}
    accepted = counts["outside"] == counts["uncertain"] == counts["incomplete"] == 0
    conclusion = ("В этой серии крупного ухудшения по выбранным критериям не обнаружено: все информативные сравнения уложились в заданные допуски."
                  if accepted else "Не все сравнения подтвердили соблюдение заданных допусков; выходы за границы и недостаточная точность перечислены ниже.")
    cluster = next(r for r in summary if r["scenario"] == "cluster" and r["metric"] == "final_bots")
    recovery = next(r for r in summary if r["scenario"] == "crisis" and r["metric"] == "recovery_ticks")
    report = ["# Влияние адаптивных полос CB3 на экосистему", "",
              conclusion, "",
              f"Всего сравнений: {len(summary)}. В допуске — {counts['equivalent']}, за допуском — {counts['outside']}, "
              f"с недостаточной точностью — {counts['uncertain']}, с неполными парами — {counts['incomplete']}; "
              f"ещё {counts['not_observed']} относятся к стратегиям, не наблюдавшимся в этих условиях. "
              "Последние не считаются доказательством сохранения редких ниш.", "",
              f"Отдельные траектории изменяются. Например, конечная численность в скоплении: {formatted(cluster)}. "
              f"Статус этого показателя — «{labels[cluster['status']].lower()}» при выбранном допуске ±10%. "
              f"Время восстановления после кризиса: {recovery['before_mean']:.2f} → {recovery['after_mean']:.2f} тика "
              "после возврата награды фотосинтеза; подробные интервалы и разнообразие приведены ниже.", "",
              "Исследование по заранее заданному [протоколу](CB3_ECOLOGY_PROTOCOL.md). "
              "20 независимых seed, 280 запусков, по 600 тиков, четыре потока, полный размер мира. "
              "Подробные наблюдения — в `CB3_ECOLOGY_RESULTS.csv`, все интервалы — в `CB3_ECOLOGY_SUMMARY.csv`.", "",
              "Стенд сравнивает настоящие исходники до балансировки (`e3456b9`) и после (`163e297`). "
              "До исследования оба сохранения seed 12345 на тике 450 совпали по SHA-256 с архивными мирами. "
              "Начальные миры совпали между версиями и смещениями; проверены клеточные координаты, баланс рождений/смертей/ручных удалений и наследование меток.", "",
              "Обычный и кризисный опыт каждой версии на каждом seed совпали на всех снимках до вмешательства (тик 250 включительно). "
              "Это дополнительная проверка воспроизводимости, не увеличение числа независимых seed.", "",
              "## Основные показатели", "",
              "Разнообразие и состояния — среднее снимков 450/500/550/600. "
              "В таблице изменение новой версии относительно старой и 95% парный bootstrap-интервал по seed. "
              "Для отношений используется геометрическое среднее парных отношений; для долей — разность в процентных пунктах. "
              "Повторы тиков не увеличивают число независимых наблюдений.", "",
              "| Условие | Показатель | Изменение [95% интервал] | Критерий |",
              "| --- | --- | --- | --- |"]
    primary = {"final_bots", "bots", "brain_effective", "founder_effective", "recovery_ticks", "left_pct", "translation_population", "translation_left"}
    for r in summary:
        if r["metric"] in primary:
            report.append(f"| {condition(r)} | {METRICS[r['metric']]} | {formatted(r)} | {labels[r['status']]} |")
    report += ["", "## Питание и энергия", "",
               "Средние поздние значения по seed: до → после. Доли приведены в процентах. "
               "Абсолютный допуск ±5 п.п. широк для редких классов; их фактические доли и случаи отсутствия "
               "нужно рассматривать отдельно. Способ питания не равен наследуемому виду, а смешанный класс может включать предацию.", "",
               "| Условие | Фотосинтез, % | Хищники, % | Смешанное, % | Мало энергии, % | Средняя энергия |",
               "| --- | --- | --- | --- | --- | --- |"]
    for case in ("normal", "cluster", "scarce", "crisis"):
        cells = []
        for metric in ("ps_pct", "predator_pct", "mixed_pct", "low_energy_pct", "mean_energy"):
            r = next(r for r in summary if r["scenario"] == case and r["shift"] == 0 and r["metric"] == metric)
            cells.append(f"{r['before_mean']:.3f} → {r['after_mean']:.3f}")
        report.append("| " + SCENARIOS[case] + " | " + " | ".join(cells) + " |")
    report += ["", "## Вымирания и потеря ниш", "",
               "Числа — миры из 20. Отсутствие специализированных хищников определяется по поздней доле класса Predator; "
               "смешанные стратегии с предацией сюда не включены. Для редких событий приведены интервалы Wilson, "
               "а не вырожденный bootstrap-интервал нулевой ширины.", "",
               "| Условие | Событие | До → после | Новое событие только после | 95% интервал риска после |",
               "| --- | --- | --- | ---: | --- |"]
    for e in events:
        lo, hi = e["after_ci"]
        report.append(f"| {condition(e)} | {EVENTS[e['event']]} | {e['before']} → {e['after']} | {e['new_only']} | {100*lo:.1f}–{100*hi:.1f}% |")
    report += ["", "## Показатели без подтверждённого допуска", "",
               "| Условие | Показатель | Изменение [95% интервал] | Допуск | Статус |",
               "| --- | --- | --- | --- | --- |"]
    for r in summary:
        if r["status"] != "equivalent":
            report.append(f"| {condition(r)} | {METRICS[r['metric']]} | {formatted(r)} | {r['margin_low']}…{r['margin_high']} | {labels[r['status']]} |")
    report += ["", "## Ограничения", "",
               "Границы допусков выбраны до основной серии, но являются рабочим определением приемлемого изменения. "
               "Интервалы исследовательские, без коррекции множественных сравнений. Выход за допуск показывает изменение, "
               "а не автоматически ухудшение; увеличение энергии, например, требует отдельной интерпретации. "
               "Неподтверждённая эквивалентность не равна обнаруженной регрессии.", "",
               "Для стратегий, ни разу не наблюдавшихся в позднем окне обеих версий, нулевой bootstrap-интервал "
               "не трактуется как доказательство сохранения редкой ниши. Такие строки отмечены отдельно.", "",
               "600 тиков охватывают три предельных возраста по умолчанию, но не долгосрочную эволюцию. "
               "В основных мирах появление яблок и органики выключено по умолчанию; полноценная пищевая сеть с ними отдельно не проверена. "
               "Другие параметры среды и число потоков остаются вне этой серии. Нулевое число вымираний "
               "на 20 seed не исключает редкий риск: верхняя граница 95% Wilson-интервала около 16%. "
               "Время выполнения этих запусков не является замером скорости.", "",
               "Воспроизведение: `tests/Build-CB3Ecology.ps1`, затем `python tests/Run-CB3Ecology.py` "
               "и `python tests/Analyze-CB3Ecology.py`. Закреплённые версии, хеши сборок/данных и контрольных миров — "
               "в `CB3_ECOLOGY_RUN.json`; полные логи — `x64/ecology/study/`.", ""]
    (output / "CB3_ECOLOGY.md").write_text("\n".join(report), encoding="utf-8")
    print("PASS: data invariants; reports written.")
    for status in labels:
        print(status, sum(r["status"] == status for r in summary))
    for r in summary:
        if r["status"] != "equivalent": print(r["scenario"],r["shift"],r["metric"],formatted(r),r["status"])


if __name__ == "__main__":
    main()
