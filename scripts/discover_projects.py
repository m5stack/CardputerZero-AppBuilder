#!/usr/bin/env python3
"""
Scan a repository for app-builder.json files and output a GitHub Actions matrix.

Each app-builder.json marks a project directory that can be built and packaged.
See docs/APP_BUILDER_JSON.md for the full schema.
"""
import json
import os
import sys


DEFAULT_RUNTIME = 'lvgl-dlopen'
DEFAULT_ENTRY = 'app_main'
DEFAULT_EVENT_ENTRY = 'app_event'
DEFAULT_LVGL_VERSION = '9.5'


def discover(repo_root):
    projects = []
    for dirpath, _, filenames in os.walk(repo_root):
        if 'app-builder.json' not in filenames:
            continue
        config_path = os.path.join(dirpath, 'app-builder.json')
        with open(config_path) as f:
            config = json.load(f)

        rel_path = os.path.relpath(dirpath, repo_root)
        pkg = config['package_name']
        projects.append({
            # Packaging fields (existing).
            'path': rel_path,
            'package_name': pkg,
            'version': config.get('version', '0.1'),
            'app_name': config.get('app_name', pkg),
            'bin_name': config['bin_name'],
            'description': config.get('description', ''),
            # Desktop-dev fields (new, optional, back-compat).
            'runtime': config.get('runtime', DEFAULT_RUNTIME),
            'entry': config.get('entry', DEFAULT_ENTRY),
            'event_entry': config.get('event_entry', DEFAULT_EVENT_ENTRY),
            'lvgl_version': config.get('lvgl_version', DEFAULT_LVGL_VERSION),
            'caps': config.get('caps', []),
            'assets': config.get('assets', []),
        })
    return projects


def main():
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    projects = discover(repo_root)

    if not projects:
        print('::error::No app-builder.json found in repository')
        sys.exit(1)

    for p in projects:
        print(f"  Found: {p['app_name']} at {p['path']} "
              f"(runtime={p['runtime']}, lvgl={p['lvgl_version']})")

    matrix_json = json.dumps(projects)
    github_output = os.environ.get('GITHUB_OUTPUT', '')
    if github_output:
        with open(github_output, 'a') as f:
            f.write(f'projects={matrix_json}\n')
    else:
        print(f'projects={matrix_json}')


if __name__ == '__main__':
    main()
