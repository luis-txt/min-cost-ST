import os
import time
import shlex
import subprocess
import signal

BASE_DIRECTORY = 'graphs'
RESULTS_DIRECTORY = 'benches'
MAX_TOTAL_TIME_MS = 50_000 # 50 seconds
MIN_REPETITIONS = 0
MAX_REPETITIONS = 1000
TIMEOUT = 300 # 5 minutes

def parse_mem(stderr_text):
    mem_kb = 0
    for line in stderr_text.splitlines():
        if 'Maximum resident set size' in line:
            tokens = line.strip().split()
            if tokens:
                possible_mem = tokens[-1]
                if possible_mem.isdigit():
                    mem_kb = int(possible_mem)
    return mem_kb


def parse_cost(stdout_text):
    for line in stdout_text.splitlines():
        if line.startswith('Total cost:'):
            try:
                cost_str = line.split(':', 1)[1].strip()
                return float(cost_str)
            except Exception:
                return None
    return None


def estimate_num_runs(first_run_time_ms):
    if (first_run_time_ms is None) or (first_run_time_ms <= 0):
        return MIN_REPETITIONS
    possible = int(MAX_TOTAL_TIME_MS // first_run_time_ms)
    return max(MIN_REPETITIONS, min(possible, MAX_REPETITIONS))


def run_bench(option, file_path):
    try:
        start_time = time.perf_counter()
        process = subprocess.Popen(
            ['/usr/bin/time', '-v', './min-cost-ST'] + shlex.split(option) + [file_path],
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )
        try:
            stdout, stderr = process.communicate(timeout=TIMEOUT)
            end_time = time.perf_counter()
            rc = process.returncode

            if rc == 0:
                elapsed_sec = end_time - start_time
                elapsed_ms = elapsed_sec * 1000.0
                mem_kb = parse_mem(stderr)
                cost = parse_cost(stdout)
                return (elapsed_ms, mem_kb, cost)
            elif rc == 137 or (rc == 2 and 'out of memory' in (stderr or '').lower()):
                print(f'Error: OOM for {file_path}')
                return (None, None, None)
            else:
                print(f'Error: Non-zero return code ({rc}) for {file_path}')
                return (None, None, None)

        except subprocess.TimeoutExpired:
            print(f'Error: Call timed out for {file_path}')
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            return (None, None, None)

    except Exception as e:
        print(f'Unexpected error: {e}')
        return (None, None, None)


def benchmark_file(option, sub_directory, log_file):
    for file_name in os.listdir(sub_directory):
        file_path = os.path.join(sub_directory, file_name)
        if not os.path.isfile(file_path):
            continue

        # Initial run
        print(f'Processing {file_path} {option}...')
        first_time_ms, first_mem_kb, cost = run_bench(option, file_path)
        if first_time_ms is None or first_mem_kb is None or cost is None:
            with open(log_file, 'a') as lf:
                lf.write(f'{file_name}: -\n')
            continue

        n_runs_total = estimate_num_runs(first_time_ms)
        
        # Accumulate sums
        sum_time_ms = first_time_ms
        sum_mem_kb = first_mem_kb
        runs_completed = 1

        # Runs if time left
        budget_spent_ms = first_time_ms
        for _ in range(1, n_runs_total):
            if budget_spent_ms >= MAX_TOTAL_TIME_MS:
                break
            t_ms, mem_kb, _ = run_bench(option, file_path)
            if t_ms is None or mem_kb is None:
                break
            runs_completed += 1
            sum_time_ms += t_ms
            sum_mem_kb += mem_kb
            budget_spent_ms += t_ms

        # Compute average
        avg_time_ms = sum_time_ms / runs_completed
        avg_mem_kb = sum_mem_kb / runs_completed

        # Log final
        with open(log_file, 'a') as lf:
            lf.write(
                f'{file_name}: '
                f'cost: {cost:.2f}, '
                f'avg_time: {avg_time_ms:.2f} ms, '
                f'avg-memory: {avg_mem_kb:.2f} KB, '
                f'runs: {runs_completed}\n'
            )


def process_sub_directory(sub_directory):
    sub_folder_name = os.path.basename(sub_directory)
    result_dir = os.path.join(RESULTS_DIRECTORY, sub_folder_name)
    os.makedirs(result_dir, exist_ok=True)
    
    options = {
        'tm.log': '-h -c',
        'pruned-mst.log': '-s -c',
        'mst.log': '-m -c',
        'two-apx.log': '-a -c',
        'two-apx-parallel.log': '-a -p -c',
        'exact.log': '-x -c',
        'exact-ub-reduct.log': '-x -u -r -c',
    }

    for log_filename, option in options.items():
        log_file_path = os.path.join(result_dir, log_filename)
        open(log_file_path, 'w').close()
        print(f'=== Running benches for file {log_filename}')
        benchmark_file(option, sub_directory, log_file_path)


def main():
    for sub_directory in os.listdir(BASE_DIRECTORY):
        full_sub_directory_path = os.path.join(BASE_DIRECTORY, sub_directory)
        if os.path.isdir(full_sub_directory_path):
            print(f'--- Running benches on directory {sub_directory}')
            process_sub_directory(full_sub_directory_path)


if __name__ == '__main__':
    main()
