from read_combined_data import read_track_data
from collections import defaultdict

def combine_tracks_data(tracks):
    combined_data = {}
    for track in tracks:
        track_data = read_track_data(track)
        for instance, data in track_data.items():
            unique_instance = f'{track}_{instance}'
            data['track'] = track
            combined_data[unique_instance] = data
    return combined_data


def find_minimum_n_edges_for_failure(all_data):
    algo_min_n_edges = {}
    sorted_instances = sorted(
        all_data.items(), 
        key=lambda item: item[1]['characteristics']['E']
    )
    for instance, data in sorted_instances:
        char = data['characteristics']
        n_edges = char['E']
        logs = data['logs']
        for alg, log_data in logs.items():
            if log_data == '-': # Algorithm did not finish
                if alg not in algo_min_n_edges or n_edges < algo_min_n_edges[alg][0]:
                    algo_min_n_edges[alg] = (n_edges, instance, data['track'])
    return algo_min_n_edges


def compute_runtime_and_cost_diff(all_data):
    running_times = defaultdict(list)
    cost_diffs = defaultdict(list)
    
    for _, data in all_data.items():
        logs = data.get('logs', {})
        for alg, log_data in logs.items():
            if isinstance(log_data, dict):
                avg_time = log_data.get('avg_time')
                if avg_time is not None:
                    running_times[alg].append(avg_time)

                if data.get('track') in ['Track1', 'Track2']:
                    cost = log_data.get('cost')
                    opt_cost = data.get('opt_cost')
                    if cost is not None and opt_cost is not None:
                        percentage_diff = abs(cost - opt_cost) / opt_cost * 100
                        cost_diffs[alg].append(percentage_diff)
                        #cost_diffs[alg].append(abs(cost - opt_cost))
    
    runtime_stats = {}
    for alg, times in running_times.items():
        if times:
            avg_rt = sum(times) / len(times)
            max_rt = max(times)
        else:
            avg_rt, max_rt = None, None
        runtime_stats[alg] = (avg_rt, max_rt)
    
    cost_diff_stats = {}
    for alg, diffs in cost_diffs.items():
        if diffs:
            avg_diff = sum(diffs) / len(diffs)
            max_diff = max(diffs)
        else:
            avg_diff, max_diff = None, None
        cost_diff_stats[alg] = (avg_diff, max_diff)
    
    return runtime_stats, cost_diff_stats


def find_max_n_edges(all_data):
    max_edges = None
    instance_with_max_edges = None
    track_of_instance = None
    
    for instance, data in all_data.items():
        n_edges = data['characteristics'].get('E')
        if n_edges is not None and (max_edges is None or n_edges > max_edges):
            max_edges = n_edges
            instance_with_max_edges = instance
            track_of_instance = data.get('track')
                
    return max_edges, instance_with_max_edges, track_of_instance


def main():
    tracks = ['Track1', 'Track2', 'Track3']
    
    all_data = combine_tracks_data(tracks)

    max_edges, instance, track = find_max_n_edges(all_data)
    print(f'=== Max |E| = {max_edges} (instance: {instance}, track: {track})\n')
    
    algo_min_n_edges = find_minimum_n_edges_for_failure(all_data)
    print('=== Minimum |E| for failure per algorithm:')
    for alg, (min_n_edges, instance, track) in algo_min_n_edges.items():
        print(f'Algorithm {alg}: smallest |E| = {min_n_edges} (instance: {instance}, track: {track})')
    
    runtime_stats, cost_diff_stats = compute_runtime_and_cost_diff(all_data)
    
    print('\n=== Running time statistics per algorithm:')
    for alg, (avg_rt, max_rt) in runtime_stats.items():
        if avg_rt is not None:
            print(f'Algorithm {alg}: average running time = {avg_rt:.2f} ms, maximum running time = {max_rt:.2f} ms')
        else:
            print(f'Algorithm {alg}: no running time data available.')
        if alg in cost_diff_stats and cost_diff_stats[alg][0] is not None:
            avg_diff, max_diff = cost_diff_stats[alg]
            print(f'average cost difference = {avg_diff:.2f}, maximum cost difference = {max_diff:.2f}\n')
        else:
            print('no cost difference data available.\n')


if __name__ == '__main__':
    main()
