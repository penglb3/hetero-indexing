# %%
import os

os.chdir('../workload')
for file in os.listdir('.'):
    wtype, ycsb_op, n_records, n_ops = file.split('-')
    n_ops = n_ops.split('.')[0]
    wtype = wtype[-1]
    new_name = '-'.join([wtype, n_records, n_ops, ycsb_op])
    if not os.path.exists(new_name):
        os.rename(file, new_name)
    else:
        raise FileExistsError(f"{new_name} already exists!")
# %%
