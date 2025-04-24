import os
import math

def read_graph_section(lines):
    num_nodes = None
    num_edges = None
    degrees = []
    costs = []

    for line in lines:
        line = line.strip()
        tokens = line.split()
        if not tokens:
            continue

        if tokens[0] == 'Nodes':
            try:
                num_nodes = int(tokens[1])
            except ValueError:
                raise ValueError('Error reading number of nodes.')
            degrees = [0] * num_nodes
        elif tokens[0] == 'Edges':
            try:
                num_edges = int(tokens[1])
            except ValueError:
                raise ValueError('Error reading number of edges.')
        elif tokens[0] == 'E':
            try:
                u = int(tokens[1])
                v = int(tokens[2])
                cost = float(tokens[3])
            except (IndexError, ValueError):
                print(f'Warning: skipping malformed edge line: {line}')
                continue

            # Increase degree count for both endpoints (assuming vertices are 1-indexed)
            if num_nodes is not None:
                if 1 <= u <= num_nodes:
                    degrees[u - 1] += 1
                if 1 <= v <= num_nodes:
                    degrees[v - 1] += 1
            costs.append(cost)
    return num_nodes, num_edges, degrees, costs


def read_terminals_section(lines):
    num_terminals = 0
    for line in lines:
        line = line.strip()
        tokens = line.split()
        if not tokens:
            continue
        if tokens[0] == 'Terminals':
            try:
                num_terminals = int(tokens[1])
            except ValueError:
                print('Warning: error reading number of terminals.')
            break
    return num_terminals


def extract_section(lines, section_name):
    section_lines = []
    in_section = False
    header = f'SECTION {section_name}'
    for line in lines:
        line = line.rstrip()
        if line == header:
            in_section = True
            continue
        if in_section:
            if line.startswith('SECTION') or line == 'END':
                break
            section_lines.append(line)
    return section_lines


def process_graph_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    graph_lines = extract_section(lines, 'Graph')
    terminals_lines = extract_section(lines, 'Terminals')
    
    try:
        num_nodes, num_edges, degrees, costs = read_graph_section(graph_lines)
    except ValueError as e:
        print(f'Error in file {filepath}: {e}')
        return None

    if num_nodes is None or num_edges is None:
        print(f'File {filepath} missing Nodes or Edges info.')
        return None

    num_terminals = read_terminals_section(terminals_lines)

    # Compute density: |E|/(|V| choose 2) = (2*|E|)/( |V|*(|V|-1) )
    density = (2 * num_edges) / (num_nodes * (num_nodes - 1)) if num_nodes > 1 else 0

    max_deg = max(degrees) if degrees else 0

    if costs:
        mean_cost = sum(costs) / len(costs)
        variance = sum((c - mean_cost) ** 2 for c in costs) / len(costs)
        cost_std = math.sqrt(variance)
    else:
        cost_std = 0.0

    return num_nodes, num_edges, density, max_deg, cost_std, num_terminals


def main():
    base_dir = 'graphs'
    output_dir = 'graph_characteristics'
    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    tracks = ['Track1', 'Track2', 'Track3']
    
    for track in tracks:
        track_dir = os.path.join(base_dir, track)
        output_file = os.path.join(output_dir, f'{track}.info')
        with open(output_file, 'w') as out_f:
            try:
                files = sorted([f for f in os.listdir(track_dir) if f.endswith('.gr')])
            except FileNotFoundError:
                print(f'Directory {track_dir} not found.')
                continue

            for filename in files:
                filepath = os.path.join(track_dir, filename)
                result = process_graph_file(filepath)
                if result is None:
                    continue
                num_nodes, num_edges, density, max_deg, cost_std, num_terminals = result
                
                out_line = (
                    f'{filename}: |V|: {num_nodes}, |E|: {num_edges}, |R|: {num_terminals}, '
                    f'density: {density}, max-deg: {max_deg}, cost-std: {cost_std}\n'
                )
                out_f.write(out_line)
                print(f'Processed {filename} in {track}')


if __name__ == '__main__':
    main()
