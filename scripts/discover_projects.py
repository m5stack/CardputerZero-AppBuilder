#!/usr/bin/env python3
"""
Scan a repository for app-builder.json files and output a GitHub Actions matrix.

Each app-builder.json marks a project directory that can be built and packaged.
Expected format:
{
  "package_name": "userdemo",
  "version": "0.1",
  "app_name": "UserDemo",
  "bin_name": "M5CardputerZero-UserDemo",
  "description": "..."
}
"""
import json
import os
import sys


def discover(repo_root):
    projects = []
    for dirpath, _, filenames in os.walk(repo_root):
        if 'app-builder.json' not in filenames:
            continue
        config_path = os.path.join(dirpath, 'app-builder.json')
        with open(config_path) as f:
            config = json.load(f)
        rel_path = os.path.relpath(dirpath, repo_root)
        projects.append({
            'path': rel_path,
            'package_name': config['package_name'],
            'version': config.get('version', '0.1'),
            'app_name': config.get('app_name', config['package_name']),
            'bin_name': config['bin_name'],
        })
    return projects


def main():
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    projects = discover(repo_root)

    if not projects:
        print('::error::No app-builder.json found in repository')
        sys.exit(1)

    for p in projects:
        print(f"  Found: {p['app_name']} at {p['path']}")

    matrix_json = json.dumps(projects)
    github_output = os.environ.get('GITHUB_OUTPUT', '')
    if github_output:
        with open(github_output, 'a') as f:
            f.write(f'projects={matrix_json}\n')
    else:
        print(f'projects={matrix_json}')


if __name__ == '__main__':
    main()
