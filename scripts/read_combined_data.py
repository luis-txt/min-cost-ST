import os
import csv
import re

def read_characteristics(track):
    char_file = os.path.join('graph_characteristics', f'{track}.info')
    characteristics = {}
    with open(char_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.endswith('.'):
                line = line[:-1]
            try:
                instance_part, rest = line.split(': ', 1)
            except ValueError:
                continue
            instance_name = instance_part.strip()
            fields = {}
            for item in rest.split(', '):
                if ': ' in item:
                    key, value = item.split(': ', 1)
                    key = key.strip()
                    value = value.strip()
                    if key in ['|V|', '|E|', '|R|']:
                        fields[key.strip('|')] = int(value)
                    elif key == 'density':
                        fields[key] = float(value)
                    elif key == 'max-deg':
                        fields['max_deg'] = int(float(value))
                    elif key == 'cost-std':
                        fields['cost_std'] = float(value)
            characteristics[instance_name] = fields
    return characteristics


def read_benches(track):
    bench_dir = os.path.join('benches', track)
    logs_data = {}
    if not os.path.exists(bench_dir):
        print(f'Bench directory {bench_dir} not found.')
        return logs_data
    for fname in os.listdir(bench_dir):
        if not fname.endswith('.log'):
            continue
        log_file_path = os.path.join(bench_dir, fname)
        logs_data[fname] = {}
        with open(log_file_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                if re.search(r':\s*-', line):
                    instance_name = line.split(':')[0].strip()
                    logs_data[fname][instance_name] = '-'
                    continue
                try:
                    instance_part, rest = line.split(': ', 1)
                except ValueError:
                    continue
                instance_name = instance_part.strip()
                log_fields = {}
                for item in rest.split(', '):
                    if ': ' in item:
                        k, v = item.split(': ', 1)
                        k = k.strip()
                        v = v.strip()
                        if k == 'avg_time' and v.endswith(' ms'):
                            v = v[:-3].strip()
                        if k == 'avg-memory' and v.endswith(' KB'):
                            v = v[:-3].strip()
                        if k == 'runs':
                            log_fields[k] = int(v)
                        else:
                            log_fields[k] = float(v)
                if 'avg-memory' in log_fields:
                    log_fields['avg_memory'] = log_fields.pop('avg-memory')
                logs_data[fname][instance_name] = log_fields
    return logs_data


def read_optimum(track):
    opt = {}
    if track not in ['Track1', 'Track2']:
        return opt
    csv_filename = os.path.join('graphs', f'{track.lower()}.csv')
    if not os.path.exists(csv_filename):
        print(f'CSV file {csv_filename} not found.')
        return opt
    with open(csv_filename, 'r', newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            instance_name = row['paceName'].strip()
            try:
                opt_val = float(row['opt'].strip())
            except ValueError:
                opt_val = None
            opt[instance_name] = opt_val
    return opt


def read_track_data(track):
    characteristics = read_characteristics(track)
    benches = read_benches(track)
    opt_costs = read_optimum(track)
    
    combined_data = {}
    for instance, char_data in characteristics.items():
        entry = {
            'characteristics': char_data,
            'logs': {},
            'opt_cost': None
        }
        for log_file, log_dict in benches.items():
            entry['logs'][log_file] = log_dict.get(instance, None)
        if instance in opt_costs:
            entry['opt_cost'] = opt_costs[instance]
        combined_data[instance] = entry
    return combined_data
