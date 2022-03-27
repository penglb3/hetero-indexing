# %%
import os

os.chdir('../workload')
files = os.listdir('.')
for file in files:
    wtype, n_records, n_ops, ycsb_op = file.split('-')
    n_records = n_records.removesuffix('000000') + 'M'
    n_ops = n_ops.removesuffix('000000') + 'M'
    new_name = '-'.join([wtype, n_records, n_ops, ycsb_op])
    ok = input(f'{file} -> {new_name}. OK? [y / any other key]')
    if not os.path.exists(new_name) and ok=='y':
        os.rename(file, new_name)
    else:
        raise FileExistsError(f"{new_name} already exists!")
# %%
