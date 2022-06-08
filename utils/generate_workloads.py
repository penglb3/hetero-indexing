# This script generates general purpose YCSB benchmark data and save into file
# Peng Lingbo 2022.03.24
import os
import time
import gc
try:
    from tqdm import tqdm
except ImportError:
    tqdm = lambda x: x
# ---------------- Parameters ----------------
# [Environment properties] Change the below parameters to fit your env!
WL_PATH = '../workload' # The directory to save generated files
YCSB_EXEC = '/home/plb/ycsb-0.17.0/bin/ycsb.sh' # The YCSB executable path

MILLION = 1000000 # The unit for workload
# [Generation properties] Change them to your need.
workload_types = 'abcd'
WL_COUNTS = [{'load':64, 'run':64}]#{'load': 3, 'run': 3}]
DISTRIBUTION = 'zipfian'
OVERRIDE = False
# ---------------- Parameters END ------------

OPS = ["INSERT", "READ", "UPDATE", "DELETE", "READMODIFYWRITE"]
proportions = {
    'a': 'readproportion=0.5\n updateproportion=0.5\n scanproportion=0\n insertproportion=0\n',
    'b': 'readproportion=0.95\n updateproportion=0.05\n scanproportion=0\n insertproportion=0\n',
    'c': 'readproportion=1\n updateproportion=0\n scanproportion=0\n insertproportion=0\n',
    'd': 'readproportion=0.95\n updateproportion=0\n scanproportion=0\n insertproportion=0.05\n',
    'e': 'readproportion=0\n updateproportion=0\n scanproportion=0.95\n insertproportion=0.05\n\
          maxscanlength=100\n scanlengthdistribution=uniform\n',
    'f': 'readproportion=0.5\n updateproportion=0\n scanproportion=0\n insertproportion=0\n\
          readmodifywriteproportion=0.5\n'
}

os.makedirs(WL_PATH, exist_ok=True)

for wcount in WL_COUNTS:
    print(f'\n================ Load={wcount["load"]}M Run={wcount["run"]}M ===================')
    for wtype in workload_types:
        # Generate YCSB core properties
        properties = f"fieldcount=1\n\
        fieldlength=1\n\
        recordcount={wcount['load']*MILLION}\n\
        operationcount={wcount['run']*MILLION}\n\
        workload=site.ycsb.workloads.CoreWorkload\n\
        readallfields=true\n"
        properties += proportions[wtype]
        if wtype in ['a', 'b', 'c', 'f']:
            properties += f'requestdistribution={DISTRIBUTION}\n'
        elif wtype == 'd':
            properties += 'requestdistribution=latest\n'
        elif wtype == 'e':
            properties += 'requestdistribution=uniform\n'
        else:
            exit(1)  # wtype should be in abcdef!

        # Write properties into file for YCSB to read.
        with open('properties.tmp', 'w') as tmp_file:
            tmp_file.write(properties)

        print(f'------------------- Workload {wtype} ---------------------')
        for ycsb_op in ['load', 'run']:
            # Check if parsed file already exists
            formatted_fname = os.path.join(
                WL_PATH, f'{wtype}-{wcount["load"]}M-{wcount["run"]}M-{ycsb_op}.log.formatted')
            if not OVERRIDE and (os.path.exists(formatted_fname) or os.path.exists(formatted_fname.replace('formatted','labeled'))):
                print('[Log]', formatted_fname, 'already exists, skip')
                continue

            # Use YCSB to generate LOAD/RUN's data.
            print(f'[Log] Start YCSB {ycsb_op} data generation')
            start = time.perf_counter()
            with os.popen(f'{YCSB_EXEC} {ycsb_op} basic -P properties.tmp') as p:
                data_lines = p.readlines()
            duration = time.perf_counter() - start
            print(f'[Log] YCSB {ycsb_op} generation done in {duration:.3f}s, start parsing now')
            
            # Parse logs and write formatted files.
            num_parsed = 0
            with open(formatted_fname, "w") as f:
                for l in tqdm(data_lines):
                    word = l.split()
                    parsed_line = None
                    if len(word) < 1:
                        continue
                    if word[0] in OPS:
                        parsed_line = f"{word[0]} {word[2][4:]}\n"
                    elif word[0] == "SCAN":
                        parsed_line = f"{word[0]} {word[2][4:]} {word[3]}\n"
                    if parsed_line:
                        f.write(parsed_line)
                        num_parsed += 1
            print(f'[Log] Parsing done, {num_parsed} ops parsed, saved in: {formatted_fname}\n')
            # Explicitly release memory, since gc may keep it even after use
            del data_lines
            gc.collect()
print("############## ALL FINISHED ##############")
os.remove("properties.tmp")