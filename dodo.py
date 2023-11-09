import sys
import subprocess
from glob import glob

DOIT_CONFIG = {'action_string_formatting': 'both',
               'default_tasks': ['dist', 'cleaner'],
               }
PIP_USER = " --user "
if sys.prefix != sys.base_prefix:  # check if we are in a virtual environment
    PIP_USER = ""

try:
    import doit
except (Exception,) as e:
    print("Installing the doit python package.")
    t = subprocess.check_output('pip install {} doit'.format(PIP_USER), text=True, shell=True, stderr=subprocess.STDOUT)
    print(t)
    print("\n\nPlease run this script again")
    print("Note: instead of 'python dodo.py', now you can use 'doit'")
    print("Note: use 'doit list' for a list of commands.")
    exit(0)

import shutil
import os

THIS_DIR = os.path.dirname(os.path.abspath(__file__))


def remove(path):
    """ param <path> could either be relative or absolute. """
    for p in glob(path):
        if os.path.isfile(p) or os.path.islink(p):
            os.remove(p)  # remove the file
        elif os.path.isdir(p):
            shutil.rmtree(p)  # remove dir and all contains
        else:
            raise ValueError("path {} is not a file or dir.".format(p))


def show_cmd(task):
    msg = task.name
    if task.verbosity >= 2:
        for action in task.actions:
            msg += "\n    - " + str(action)
    return msg


def task_cleaner():
    """ Clean the entire repository for a git commit & be ready to re-compile the entire project!"""

    def do_clean():
        patterns = ["dist",
                    ]
        for pattern in patterns:
            try:
                print(f"cleaner: {pattern}")
                remove(pattern)
            except Exception as err:
                print(str(err))

    return {
        "actions": None,
        "clean": [(do_clean, ),
                  "cd i2c-stick-arduino && doit clean",
                  "cd web-interface && doit clean",
                  ],
        'title': show_cmd,
    }


def task_pip():
    """Install required python packages using pip"""
    for rqt_file in ['web-interface/requirements.txt', 'i2c-stick-arduino/requirements.txt']:
        yield {
            'name': rqt_file,
            'actions': ["pip install {} -r {}".format(PIP_USER, rqt_file)],
            'file_dep': [rqt_file],
            'title': show_cmd,
        }


def task_dist():
    return {
        'actions': ["cd i2c-stick-arduino && doit dist",
                    "cd web-interface && doit dist",
                    ],
        'verbosity': 2,
        'title': show_cmd,
        'uptodate': [False],
    }


if __name__ == '__main__':
    import doit

    doit.run(globals())
