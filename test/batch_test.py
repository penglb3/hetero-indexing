from doctest import UnexpectedException
import os
import time

# ---------------- Parameters ----------------
# [Environment properties] Change the below parameters to fit your env!
WL_PATH = '../workload' # The directory to save generated files
YCSB_EXEC = '/home/plb/ycsb-0.17.0/bin/ycsb.sh' # The YCSB executable path


# [Test setting] Change them to your need.
workload_types = 'a'
workload_counts = [{'load': i, 'run': i} for i in [4, 16, 64, 256]]
models = ['art+inb']# ,'art','unordered_map','level_hash']
bench_latency = True # To get accurate average latency, set this to False
USE_STABLE_BIN = False 
RESULT_PATH = 'hetero-struct/art+inb'

ANSI_COLOR = {
    'RED'    : "\x1b[31m",
    'GREEN'  : "\x1b[32m",
    'YELLOW' : "\x1b[33m",
    'BLUE'   : "\x1b[34m",
    'MAGENTA': "\x1b[35m",
    'CYAN'   : "\x1b[36m",
    'RESET'  : "\x1b[0m"
}

do_latency = '--bench=latency' if bench_latency else ''
bin_postfix = '.test.stable' if USE_STABLE_BIN else '.test'

# Check if necessary files exists
if not os.path.exists(WL_PATH):
    raise FileNotFoundError(f'Workload directory {WL_PATH} not found!')
for model in models:
    if not os.path.exists(model + bin_postfix):
        raise FileNotFoundError(f'Test object {model + bin_postfix} not found!')

os.makedirs(RESULT_PATH, exist_ok=True)

for wcount in workload_counts:
    for wtype in workload_types:
        setting = f"{wtype}-{wcount['load']}M-{wcount['run']}M"
        print(f'--------------- {setting} --------------')

        load_file = os.path.join(WL_PATH, f'{setting}-load.log.labeled')
        run_file = os.path.join(WL_PATH, f'{setting}-run.log.formatted')
        if not os.path.exists(load_file) or not os.path.exists(run_file):
            raise FileNotFoundError(f'Workload {setting} not found!')
        for model in models:
            result_file = os.path.join(RESULT_PATH, f'{setting}-{model}.log')
            print(f'[Log] Start testing {model} ({RESULT_PATH})')
            start = time.perf_counter()
            if os.path.exists(result_file):
                print(f'Test log {result_file} already exists. skip.')
                continue
            cmd = f"./{model + bin_postfix} --mode=ycsb {do_latency} --load_file={load_file} --run_file={run_file}"
            try:
                with os.popen(cmd) as p:
                    log = p.readlines()
            except UnexpectedException as e:
                raise RuntimeError(f'Run {model} failed.') from e
            print(f'[Log] done in {time.perf_counter() - start :.3f}s. ')
            result = []
            for line in [log[-8],log[-7],log[-2],log[-1]]:
                print(line, end='')
                # Clear color codes
                for code in ANSI_COLOR.values():
                    line = line.replace(code, '')
                if line[:8] == '[results':
                    result.append(line)
                else:
                    break
            if len(result) < 4:
                print(f'[Log] Error / Fault. Skip. Command = {cmd}')
                continue
            with open(result_file, 'w') as f:
                f.writelines(result)
