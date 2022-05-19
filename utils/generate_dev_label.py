# Generates device label for hetero-indexing.
# Labeled load logs won't cause errors for other indexes, nor will they do anything.
# %%
from doctest import UnexpectedException
import os
from heapq import nlargest
from tqdm import tqdm
import gc

# ---------------- Parameters ----------------
# [Environment properties] Change the below parameters to fit your env!
WL_PATH = '../workload' # The directory to save generated files
YCSB_EXEC = '/home/plb/ycsb-0.17.0/bin/ycsb.sh' # The YCSB executable path

# [Generation properties] Change them to your need.

MEMTYPE_PERCENTAGE = 20  # If 20, Then TOP 20% will be marked as mem_type

# Will operate only on this specific setting
# Set to None if you want to generate for all logs
# !! UNIT is million !!
MILLION = 1000000
WL_COUNTS = [{'load': 1024, 'run': 1024}]
# Will operate only on these workload types
workload_types = 'a'

# ---------------- Parameters END ------------
OPS = ["INSERT", "READ", "UPDATE", "DELETE", "READMODIFYWRITE"]

# %%
# --------------- Main Program -----------------
for wcount in WL_COUNTS:
    # Number of keys to be labeled as mem_type. You can also fix it to a certain value.
    N_MEMTYPE = int(wcount['load'] * MILLION * MEMTYPE_PERCENTAGE / 100)
    print(f'\n================ Load={wcount["load"]}M Run={wcount["run"]}M ===================')
    for wtype in workload_types:
        print(f'------------------- Workload {wtype} ---------------------')

        # Check if formatted run log exists
        filename = f"{wtype}-{wcount['load']}M-{wcount['run']}M-run.log.formatted"
        run_log_name = os.path.join(WL_PATH, filename)
        load_log_name = run_log_name.replace('run', 'load')
        output_fname = load_log_name.replace('formatted', 'labeled')
        if not os.path.exists(run_log_name) or not os.path.exists(load_log_name):
            print(f"[Info] Config {wtype}-{wcount['run']}M-{wcount['load']}M load/log file not found, skip.")
            continue
        if os.path.exists(output_fname):
            print(f"[Info] Config {wtype}-{wcount['run']}M-{wcount['load']}M already labeled, skip.")
            continue
        # Read RUN logs
        print(f'[Log] Read RUN log from <<< {filename}')
        lines = None
        with open(run_log_name, 'r') as f:
            lines = f.readlines()
        if not lines:
            raise FileNotFoundError('Read log failed')
        # Record keys' frequencies
        freq = {}
        for l in tqdm(lines, desc='[Log] Recording frequency'):
            key = l.split()[1]
            if key not in freq:
                freq[key] = 1
            else:
                freq[key] += 1
        del lines
        gc.collect()
        print(f'[Info] {MEMTYPE_PERCENTAGE}% ({N_MEMTYPE}) will be marked as MEM_TYPE ')
        # Pick out most frequent ones
        memtype_keys = nlargest(N_MEMTYPE, freq, key=freq.get)
        top_freqs = sum(freq[key] for key in memtype_keys) / (wcount["run"] * MILLION)
        print(f'[Info] Top {MEMTYPE_PERCENTAGE}% has total frequency {top_freqs:.3f}')
        print('[Info] Top 3 keys:', {key: freq[key] for key in memtype_keys[:3]})
        memtype_keys = set(memtype_keys) # important speed up technique.

        # Read LOAD logs
        print(f'[Log] Read LOAD log from <<< {load_log_name}')
        with open(load_log_name, 'r') as f:
            lines = f.readlines()

        # Label LOAD logs and write into files
        
        print(f'[Log] Write LABELED LOAD log into >>> {output_fname}')
        with open(output_fname, 'w') as f:
            for line in tqdm(lines, desc="[Log] Labelling and writing"):
                wordlist = line.split()
                if wordlist[1] in memtype_keys:
                    wordlist[0] += '_MT'
                # wordlist.append(str(int(wordlist[1] in memtype_keys))+'\n')
                f.write(' '.join(wordlist) + '\n')
        print()
        del lines
        gc.collect()
print("############## ALL FINISHED ##############")
# %%
