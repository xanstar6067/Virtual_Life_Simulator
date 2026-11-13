"""Per-bot, per-tick hardware counters from paired short/long PMC runs in x64/perf-audit/pmc."""
import re, sys, pathlib, collections

d = pathlib.Path(sys.argv[1])


def read_log(p):
    raw = p.read_bytes()
    text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8', 'replace')
    ticks = int(re.search(r'^RESULT ticks=(\d+)', text, re.M).group(1))
    bots = int(re.search(r'^MEASURE_START bots=(\d+)', text, re.M).group(1))
    return ticks, bots


def counters(p):
    """Pmc events carry cumulative per-CPU counter values at a context switch, tagged with the incoming thread.
    The difference between consecutive events on one CPU belongs to the thread that ran in between."""
    names, pid, threads, sums = None, None, set(), collections.Counter()
    pending, last = None, {}
    for line in open(p, encoding='utf-8', errors='replace'):
        s = line.strip()
        if s.startswith('Pmc,') and names is None:
            names = [x.strip() for x in s.split(',')[3:]]
        elif s.startswith(('P-Start,', 'P-DCStart,')) and 'cb3-perf.exe' in s and pid is None:
            pid = int(re.search(r'\((\s*\d+)\)', s).group(1))
        elif s.startswith(('T-Start,', 'T-DCStart,')) and pid is not None and re.search(rf'\(\s*{pid}\)', s):
            threads.add(int(s.split(',')[3]))
        elif s.startswith('Pmc,') and names:
            parts = [x.strip() for x in s.split(',')]
            if parts[1].isdigit():
                pending = (parts[1], int(parts[2]), [int(v) for v in parts[3:3 + len(names)]])
        elif s.startswith('CSwitch,') and pending:
            parts = [x.strip() for x in s.split(',')]
            if parts[1] == pending[0] and int(parts[3]) == pending[1]:
                cpu = int(parts[16])
                prev = last.get(cpu)
                if prev and prev[0] in threads:
                    deltas = [b - a for a, b in zip(prev[1], pending[2])]
                    if all(x >= 0 for x in deltas):
                        for n, v in zip(names, deltas):
                            sums[n] += v
                last[cpu] = (pending[1], pending[2])
            pending = None
    return sums


res = collections.defaultdict(dict)
for short in sorted(d.glob('*-2000.txt')):
    key = short.name[:-len('-2000.txt')]
    long = d / f'{key}-8000.txt'
    if not long.exists():
        continue
    layout, mode, group = key.split('-')
    t1, bots = read_log(d / f'{key}-2000.log')
    t2, _ = read_log(d / f'{key}-8000.log')
    c1, c2 = counters(short), counters(long)
    for n in c2:
        res[(layout, mode)][n] = (c2[n] - c1[n]) / (t2 - t1) / bots
    res[(layout, mode)]['bots'] = bots
    res[(layout, mode)]['dticks'] = t2 - t1

cols = ['InstructionRetired', 'TotalCycles', 'DcacheAccesses', 'DcacheMisses', 'CacheMisses', 'BranchMispredictions']
print('layout,mode,bots,' + ','.join(cols) + ',IPC')
for (layout, mode), v in sorted(res.items()):
    ipc = v.get('InstructionRetired', 0) / v['TotalCycles'] if v.get('TotalCycles') else float('nan')
    print(f'{layout},{mode},{v["bots"]},' + ','.join(f'{v.get(c, float("nan")):.2f}' for c in cols) + f',{ipc:.3f}')
# Brain share: fixed minus skipbrain.
print('\nbrain = fixed - skipbrain (per bot per tick)')
for layout in ['sequential', 'default', 'shuffled', 'aged']:
    f, s = res.get((layout, 'fixed')), res.get((layout, 'skipbrain'))
    if f and s:
        print(layout, ', '.join(f'{c}={f.get(c, 0) - s.get(c, 0):.2f}' for c in cols))
