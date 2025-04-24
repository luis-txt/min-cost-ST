import os
import argparse
import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter, NullFormatter
from read_combined_data import read_track_data

LABELS = {
    'E': '$|E|$',
    'density': 'Density',
    'max_deg': 'Maximum Degree',
    'R': '$|R|$',
    'cost_std': '$\\sigma(c(E_T))$',
    'avg_time': 'Time (ms)',
    'cost': '$c(E_T)$',
    'cost_diff': '$|c(E_T)- c(E^{OPT}_T)|$',
    'avg_memory': 'Memory Usage (KB)',
    'V': '|V|',
    'opt_cost': '$c(E^{OPT}_T)$',
    'exact.log': 'ILP',
    'exact-ub-reduct.log': 'OILP',
    'two-apx.log': '2-APX',
    'two-apx-parallel.log': '2-APX-P',
    'tm.log': 'TM',
    'pruned-mst.log': 'PMST',
    'mst.log': 'MST'
}

def data_to_dataframe(track_data):
    records = []
    for instance, entry in track_data.items():
        char = entry.get('characteristics', {})
        logs = entry.get('logs', {})
        for log_name, log_data in logs.items():
            record = {
                'instance': instance,
                'algorithm': log_name,
                'V': char.get('V'),
                'E': char.get('E'),
                'density': char.get('density'),
                'max_deg': char.get('max_deg'),
                'R': char.get('R'),
                'cost_std': char.get('cost_std'),
                'opt_cost': entry.get('opt_cost')
            }
            if isinstance(log_data, dict):
                record.update(log_data)
            else:
                record['cost'] = np.nan
                record['avg_time'] = np.nan
                record['avg_memory'] = np.nan
                record['runs'] = np.nan
            records.append(record)
    df = pd.DataFrame(records)
    return df


def format_ticks(value, _):
    if value >= 1 and value < 1000:
        return f'{value:.0f}'
    elif value >= 1000:
        return f'$10^{{{int(np.log10(value))}}}$'
    elif value > 0:
        return f'{value:.3f}'
    else:
        return '0'


def set_axes_scaling(ax, log_scale_x=True, log_scale_y=True):
    if log_scale_x:
        ax.set_xscale('log')
        ax.xaxis.set_major_locator(LogLocator(base=10.0))
        ax.xaxis.set_minor_locator(LogLocator(base=10.0, subs='auto'))
    else:
        ax.set_xscale('linear')

    if log_scale_y:
        ax.set_yscale('log')
        ax.yaxis.set_major_locator(LogLocator(base=10.0))
        ax.yaxis.set_minor_locator(LogLocator(base=10.0, subs='auto'))
    else:
        ax.set_yscale('linear')

    ax.xaxis.set_minor_formatter(NullFormatter())
    ax.yaxis.set_minor_formatter(NullFormatter())
    ax.xaxis.set_major_formatter(FuncFormatter(format_ticks))
    ax.yaxis.set_major_formatter(FuncFormatter(format_ticks))


def plot_with_distinct_styles(x, y, label, index, points_only=False):
    line_styles = ['-', '--', '-.', ':']
    markers = ['s', 'D', '^', 'o', 'x']
    if points_only:
        plt.scatter(x, y, label=label, marker=markers[index % len(markers)], s=7, zorder=3)
    else:
        plt.plot(x, y, label=label,
                 linestyle=line_styles[index % len(line_styles)],
                 marker=markers[index % len(markers)],
                 markersize=3, linewidth=1, zorder=3)


def plot_metric(df, x_metric, y_metric, output_dir, log_scale_x=True, log_scale_y=True, algorithms_to_plot=None, points_only=False, output_ext='pgf'):
    if y_metric == 'cost_diff':
        df = df[~df['algorithm'].isin(['exact.log', 'exact-ub-reduct.log'])]
        
    if algorithms_to_plot is not None:
        df = df[df['algorithm'].isin(algorithms_to_plot)]
    
    plt.figure(figsize=(3.3, 2.5))
    ax = plt.gca()
    set_axes_scaling(ax, log_scale_x=log_scale_x, log_scale_y=log_scale_y)
    
    algorithms = df['algorithm'].unique()
    for i, alg in enumerate(algorithms):
        sub_df = df[df['algorithm'] == alg].copy()
        sub_df = sub_df.dropna(subset=[x_metric, y_metric])
        if sub_df.empty:
            continue
        sub_df.sort_values(by=x_metric, inplace=True)
        if y_metric == 'cost_diff':
            epsilon = 1e-6
            sub_df.loc[sub_df[y_metric] <= 0, y_metric] = epsilon
        x = sub_df[x_metric].values
        y = sub_df[y_metric].values
        legend_label = LABELS.get(alg, alg)
        plot_with_distinct_styles(x, y, label=legend_label, index=i, points_only=points_only)
    
    plt.xlabel(LABELS.get(x_metric, x_metric))
    plt.ylabel(LABELS.get(y_metric, y_metric))
    plt.grid(True, which='both', ls='--')
    plt.legend()
    
    plt.tight_layout()
    
    style_suffix = 'points' if points_only else 'lines'
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    output_path = os.path.join(output_dir, f'{y_metric}_vs_{x_metric}_{style_suffix}.{output_ext}') # change to pdf if needed
    plt.savefig(output_path)
    plt.close()
    print(f'Saved plot: {output_path}')


def main():
    mpl.rcParams.update({
        'pgf.texsystem': 'pdflatex',
        'text.usetex': True,
        'font.family': 'serif',
        'lines.markersize': 3,
        'axes.formatter.useoffset': False,
        'lines.linewidth': 1,
        'font.size': 10,
        'axes.labelsize': 10,
        'xtick.labelsize': 8,
        'ytick.labelsize': 8,
        'legend.fontsize': 8,
    })

    parser = argparse.ArgumentParser(
        description='Plot benchmark results for all tracks with subdirectories per track, characteristic, and group.'
    )
    parser.add_argument('--tracks', type=str, nargs='*', default=['Track1', 'Track2', 'Track3'], help='Tracks to process (e.g., Track1 Track2 Track3)')
    parser.add_argument('--output_dir', type=str, default='plots', help='Base directory to save plots')
    parser.add_argument('--pdf', action='store_true', help='Output plots as PDF instead of PGF')
    args = parser.parse_args()

    output_ext = 'pdf' if args.pdf else 'pgf'
    
    for track in args.tracks:
        print(f'Processing {track} ...')
        track_data = read_track_data(track)
        df = data_to_dataframe(track_data)
        print(f'Total records for {track}: {len(df)}')
        
        if track in ['Track1', 'Track2']:
            df['cost_diff'] = df.apply(lambda row: abs(row['cost'] - row['opt_cost'])
                                       if not np.isnan(row['cost']) and row['opt_cost'] is not None 
                                       else np.nan, axis=1)
            overall_performance_metrics = ['avg_time', 'avg_memory', 'cost_diff']
        else:
            overall_performance_metrics = ['avg_time', 'avg_memory']
        
        characteristics = ['E', 'density', 'max_deg', 'R']
        
        overall_track_dir = os.path.join(args.output_dir, track)
        for x_metric in characteristics:
            char_dir = os.path.join(overall_track_dir, x_metric)
            if not os.path.exists(char_dir):
                os.makedirs(char_dir)
            for y_metric in overall_performance_metrics:
                for points_only in [False, True]:
                    plot_metric(
                                    df, x_metric=x_metric, y_metric=y_metric,
                                    output_dir=char_dir,
                                    log_scale_x=True, log_scale_y=True,
                                    points_only=points_only,
                                    output_ext=output_ext,
                                )
        
        group_definitions = {
            'exact': ['exact.log', 'exact-ub-reduct.log'],
            'approx': ['two-apx.log', 'two-apx-parallel.log'],
            'heuristic': ['tm.log', 'pruned-mst.log', 'mst.log']
        }
        for group_name, algos in group_definitions.items():
            for x_metric in characteristics:
                group_dir = os.path.join(overall_track_dir, group_name, x_metric)
                if not os.path.exists(group_dir):
                    os.makedirs(group_dir)
                for y_metric in overall_performance_metrics:
                    if group_name == 'exact' and y_metric == 'cost_diff':
                        continue
                    for points_only in [False, True]:
                        plot_metric(
                                        df, x_metric=x_metric, y_metric=y_metric,
                                        output_dir=group_dir,
                                        log_scale_x=True, log_scale_y=True,
                                        algorithms_to_plot=algos,
                                        points_only=points_only,
                                        output_ext=output_ext,
                                    )


if __name__ == '__main__':
    main()
