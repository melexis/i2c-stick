import sys
import subprocess

CONTEXT_FILE = "context.yaml"

DOIT_CONFIG = {'action_string_formatting': 'both',
               'default_tasks': ['arduino-compile:i2c-stick', 'cleaner'],
               }
PIP_USER = " --user "
if sys.prefix != sys.base_prefix:  # check if we are in a virtual environment
    PIP_USER = ""

try:
    import doit
    import shutil
    import os
    import yaml
    import jinja2
    import yamlinclude
    from glob import glob
    from pathlib import Path
    import serial.tools.list_ports
    from doit.action import CmdAction
    from doit.tools import run_once
    import platform
except (Exception,) as e:
    print("installing python packages from requirements.txt")
    t = subprocess.check_output('pip install {} -r requirements.txt'.format(PIP_USER), text=True, shell=True, stderr=subprocess.STDOUT)
    print(t)
    print("Note: instead of 'python dodo.py', now you can use 'doit'")
    print("Note: use 'doit list' for a list of commands.")

import shutil
import os
import yaml
import jinja2
import yamlinclude
from glob import glob
from pathlib import Path
import serial.tools.list_ports
from doit.action import CmdAction
from doit.tools import run_once
from doit.tools import config_changed
import platform

this_dir = os.path.dirname(os.path.abspath(__file__))
yamlinclude.YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=this_dir)

with open(CONTEXT_FILE) as f:
    context = yaml.load(f, Loader=yaml.FullLoader)

for driver in context['drivers']:
    if 'disable' not in driver:
        driver['disable'] = 0

# Arduino compiles all cpp files in the directory; rename to <ori>.disable
for driver in context['drivers']:
    if driver['disable']:
        files = list(Path(".").glob(driver['src_name'] + "_*.cpp")) + list(Path(".").glob(driver['src_name'] + "_*.h"))
        for file in files:
            shutil.move(file, str(file) + ".disable")

# undo: Arduino compiles all cpp files in the directory; rename to <ori>.disable
for driver in context['drivers']:
    if driver['disable'] == 0:
        files = list(Path(".").glob(driver['src_name'] + "_*.disable"))
        for file in files:
            shutil.move(file, str(file).replace(".disable", ""))

# remove the disabled drivers
context['drivers'] = [driver for driver in context['drivers'] if driver['disable'] == 0]

# remove the disabled boards
for board in context['boards']:
    if 'disable' not in board:
        board['disable'] = 0

context['boards'] = [board for board in context['boards'] if board['disable'] == 0]

for index, driver in enumerate(context['drivers']):
    driver['id'] = index + 1

arduino_add_url = " ".join(["--additional-urls {}".format(x) for x in context['board_manager']['additional-urls']])

ARDUINO_CLI = os.path.join('tools', 'arduino-cli' + Path(sys.executable).suffix)


def str_presenter(dumper, data):
    if len(data.splitlines()) > 1:  # check for multiline string
        return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
    return dumper.represent_scalar('tag:yaml.org,2002:str', data)


yaml.add_representer(str, str_presenter)


def remove(path):
    """ param <path> could either be relative or absolute. """
    for p in glob(path):
        if os.path.isfile(p) or os.path.islink(p):
            os.remove(p)  # remove the file
        elif os.path.isdir(p):
            shutil.rmtree(p)  # remove dir and all contains
        else:
            raise ValueError("path {} is not a file or dir.".format(p))


def find_git_repo(path):
    """Find repository root from the path's parents"""
    for path in Path(path).parents:
        # Check whether "path/.git" exists and is a directory
        git_dir = path / ".git"
        if git_dir.is_dir():
            return path


def show_cmd(task):
    msg = task.name
    if task.verbosity >= 2:
        for action in task.actions:
            msg += "\n    - " + str(action)
    return msg


def task_cleaner():
    """ Clean the entire repository for a git commit & be ready to re-compile the entire project!"""

    def do_clean():
        patterns = ["build",
                    "package",
                    "*~",
                    "*.bak"]
        for pattern in patterns:
            try:
                remove(pattern)
            except Exception as err:
                print(str(err))

    return {
        "actions": None,
        "clean": [do_clean],
        'title': show_cmd,
    }


def task_pip():
    """Install required python packages using pip"""
    for rqt_file in ['requirements.txt']:
        yield {
            'name': rqt_file,
            'actions': ["pip install {} -r {}".format(PIP_USER, rqt_file)],
            'file_dep': [rqt_file],
            'title': show_cmd,
        }


def task_arduino_install_cli():
    """Arduino: Install the arduino-cli tool"""

    def do_install(task):
        if not Path("tools").is_dir():
            os.mkdir('tools')
        system = platform.system().lower()
        os_dict = {
            'linux': 'Linux',
            'windows': 'Windows',
            'darwin': 'macOS',
        }
        bits = '32bit'
        if sys.maxsize > 2 ** 32:
            bits = '64bit'
        zip_suffix = 'tar.gz'
        if system == 'windows':
            zip_suffix = 'zip'

        url = "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_{}_{}.{}".format(os_dict[system], bits,
                                                                                            zip_suffix)
        print("downloading:", url)
        import io
        import zipfile
        import tarfile
        from contextlib import closing
        import requests

        if zip_suffix == 'tar.gz':
            response = requests.get(url, stream=True)
            tar_file = tarfile.open(fileobj=response.raw, mode="r|gz")
            tar_file.extractall(path="tools")
        else:
            r = requests.get(url)
            with closing(r), zipfile.ZipFile(io.BytesIO(r.content)) as archive:
                for member in archive.infolist():
                    if member.filename == Path(task.targets[0]).name:
                        print("file: {}".format(member.filename))
                        with open(task.targets[0], "wb") as target_file:
                            target_file.write(archive.read(member))

        return

    return {
        'basename': 'arduino-install-cli',
        'actions': [(do_install,)],
        'verbosity': 2,
        'targets': [ARDUINO_CLI],
        'uptodate': [run_once],
    }


def task_package():
    """Create the ZIP package file"""

    def do_package():
        shutil.copy('firmware_list.md', 'build')
        # remove("package")
        # os.mkdir("package")
        # dst = os.path.join("package", "ktc_trial")
        # os.mkdir(dst)
        # shutil.copy(os.path.join("dist", 'ktc_trial.exe'), dst)
        # shutil.copy("ktc_trial.yml", dst)
        # shutil.make_archive("ktc_trial", 'zip', "package")
        return

    return {
        'actions': [do_package],
        # 'targets': ['ktc_trial.zip'],
        'clean': True,
        # 'file_dep': ['ktc_trial.yml', os.path.join("dist", 'ktc_trial.exe')],
        'task_dep': ['arduino-compile', 'generate:firmware_list.md'],
        'title': show_cmd,
    }


def task_arduino_update_index():
    """Arduino: get a fresh copy of the board index"""
    return {
        'basename': 'arduino-update-index',
        'actions': ["{} {} core update-index".format(ARDUINO_CLI, arduino_add_url),
                    ],
        'uptodate': [run_once],
        'task_dep': ['arduino-install-cli'],
        'title': show_cmd,
    }


def task_arduino_install_core():
    """Arduino: install the core tool-chain for a platform"""
    cores = [board['core'] for board in context['boards']]
    for core in cores:
        yield {
            'basename': 'arduino-install-core',
            'name': core,
            'actions': ["{} {} core install {}".format(ARDUINO_CLI, arduino_add_url, core),
                        ],
            'task_dep': ['arduino-install-cli', 'arduino-update-index'],
            'uptodate': [run_once],
            'title': show_cmd,
        }


def task_arduino_lib_update_index():
    """Arduino: get a fresh copy of the library index"""
    return {
        'basename': 'arduino-lib-update-index',
        'actions': ["{} {} lib update-index".format(ARDUINO_CLI, arduino_add_url),
                    ],
        'uptodate': [run_once],
        'task_dep': ['arduino-install-cli'],
        'title': show_cmd,
    }


def task_arduino_install_libs():
    """Arduino: install the specific libraries"""
    yield {
        'basename': 'arduino-install-libs',
        'name': None,
    }
    libs = []
    if 'libraries' in context:
        libs = context['libraries']
    for lib in libs:
        lib_cli = ""
        lib_name = ""
        if 'name' in lib:
            lib_cli = lib['name']
            lib_name = lib['name']
            if 'version' in lib:
                lib_cli += "@" + str(lib['version'])
        elif 'git' in lib:
            lib_cli = '--git-url ' + lib['git']
            lib_name = lib['git'].split('/')[-1]
            lib_name = lib_name.replace(".git", "")
            os.environ['ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL'] = 'true'
            if 'version' in lib:
                lib_cli += "#" + str(lib['version'])
        elif 'zip' in lib:
            lib_cli = '--zip-path ' + lib['zip']
            lib_name = Path(lib['zip']).stem
            os.environ['ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL'] = 'true'

        yield {
            'basename': 'arduino-install-libs',
            'name': lib_name,
            'actions': ["{} lib install {}".format(ARDUINO_CLI, lib_cli)],
            'uptodate': [run_once],
            'task_dep': ['arduino-install-cli',
                         'arduino-lib-update-index'],
            'title': show_cmd,
        }


def task_arduino_lib_upgrade():
    """Arduino: upgrade the libraries to its latest revision"""
    yield {
        'basename': 'arduino-upgrade-libs',
        'name': None,
    }
    libs = []
    if 'libraries' in context:
        libs = context['libraries']
    for lib in libs:
        lib_cli = ""
        lib_name = ""
        cli_action = 'install'
        if 'name' in lib:
            lib_cli = lib['name']
            lib_name = lib['name']
            cli_action = 'upgrade'
        elif 'git' in lib:
            lib_cli = '--git-url ' + lib['git']
            lib_name = lib['git'].split('/')[-1]
            lib_name = lib_name.replace(".git", "")
            os.environ['ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL'] = 'true'
        elif 'zip' in lib:
            lib_cli = '--zip-path ' + lib['zip']
            lib_name = Path(lib['zip']).stem
            os.environ['ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL'] = 'true'

        yield {
            'basename': 'arduino-upgrade-libs',
            'name': lib_name,
            'actions': ["{} lib {} {}".format(ARDUINO_CLI, cli_action, lib_cli)],
            'uptodate': [False],
            'verbosity': 2,
            'task_dep': ['arduino-install-cli',
                         'arduino-lib-update-index'],
            'title': show_cmd,
        }


def task_arduino_libs():
    """Arduino: list installed libraries"""
    return {
        'basename': 'arduino-libs',
        'actions': ["{} {} lib list".format(ARDUINO_CLI, arduino_add_url),
                    ],
        'uptodate': [False],
        'verbosity': 2,
        'task_dep': ['arduino-install-cli'],
        'title': show_cmd,
    }


def task_arduino_board_details():
    """Arduino: get board details, a list of parameters one normally sets via the menu interface."""
    for board in context['boards']:
        fqbn = board['fqbn']
        yield {
            'basename': 'arduino-board-details',
            'name': board['nick'],
            'verbosity': 2,
            'actions': ["{} board details --fqbn {}".format(ARDUINO_CLI, fqbn)],
            'task_dep': ['arduino-install-cli',
                         'arduino-install-core:' + board['core'],
                         ],
            'uptodate': [False],  # force to run the task always
            'title': show_cmd,
        }


def task_arduino_compile():
    """Arduino: Compile the sources into a UF2 file (and bin-file)"""
    working_directory = Path('.')
    headers = list(working_directory.glob('*.h'))
    cpp_files = list(working_directory.glob('*.cpp'))

    # dependency manager is defined for all code inside the generator:
    dep_manager = doit.Globals.dep_manager

    def store(fqbn_store, extra_flags_store):
        return dict(fqbn=fqbn_store, extra_flags=extra_flags_store)

    for board in context['boards']:
        fqbn = board['fqbn']
        if 'parameters' in board:
            parameters = ",".join("{}={}".format(k, v) for k, v in board['parameters'].items())
            if parameters != "":
                fqbn += ":" + parameters
        extra_flags = ""
        if 'extra_defines' in board:
            for k, v in board['extra_defines'].items():
                if type(v) is str:
                    v = '"' + v + '"'
                flag = "compiler.cpp.extra_flags=\"-D{}={}\"".format(k, v)
                flag = flag.replace('"', '\\"')
                extra_flags += '--build-property "{}" '.format(flag)
        result = dep_manager.get_result("{}:{}".format("arduino-compile", board['nick']))
        clean_flag = ""
        if type(result) is dict:
            if result['fqbn'] != fqbn:
                clean_flag = " --clean"
            if result['extra_flags'] != extra_flags:
                clean_flag = " --clean"
        else:
            clean_flag = " --clean"

        yield {
            'basename': 'arduino-compile',
            'name': board['nick'],
            'actions': ["{} compile --fqbn {} {} {}.ino -e {}".format(
                ARDUINO_CLI, fqbn, extra_flags, context['ino'], clean_flag),
                (store, [fqbn, extra_flags]),
            ],
            'task_dep': ['arduino-install-cli',
                         'arduino-install-core:' + board['core'],
                         'arduino-install-libs',
                         'copy-plugins',
                         'generate:i2c_stick_dispatcher.h',
                         'generate:i2c_stick_dispatcher.cpp',
                         ],
            'title': show_cmd,
            'file_dep': [CONTEXT_FILE,
                         '{}.ino'.format(context['ino']),
                         'i2c_stick_dispatcher.h',
                         'i2c_stick_dispatcher.cpp',
                         ] + headers + cpp_files,
            'targets': ['build/{}/{}.ino.{}'.format(
                board['fqbn'].replace(":", "."), context['ino'], board['bin_extension'])],
        }


def task_arduino_upload():
    """Arduino: Upload to the target board"""

    def do_upload(board_cfg, port):
        if port == 'auto':
            method = 'vid_pid'
            if 'port_discovery_method' in board_cfg:
                method = board_cfg['port_discovery_method']

            if method == 'vid_pid':  # special case for teensy
                filtered_ports = []
                pid = None
                vid = None
                if 'USB_VID' in board_cfg:
                    vid = board_cfg['USB_VID']
                if 'USB_PID' in board_cfg:
                    pid = board_cfg['USB_PID']
                for p in serial.tools.list_ports.comports(include_links=False):
                    if vid is None:
                        if pid is None:
                            filtered_ports.append(p)
                            continue

                    if vid is None:
                        if p.pid == pid:
                            filtered_ports.append(p)
                            continue
                    if pid is None:
                        if p.vid == vid:
                            filtered_ports.append(p)
                            continue

                    if p.vid == vid:
                        if p.pid == pid:
                            filtered_ports.append(p)
                port = filtered_ports[0].name

            if method == 'arduino-cli':
                output = subprocess.check_output("{} board list --fqbn {}".format(ARDUINO_CLI, board_cfg['fqbn']),
                                                 text=True)
                lines = output.split("\n")
                if len(lines) >= 2:
                    port = lines[1].split(' ')[0]  # we only use one; the first one; to upload...

        fqbn = board_cfg['fqbn']
        if 'parameters' in board_cfg:
            parameters = ",".join("{}={}".format(k, v) for k, v in board_cfg['parameters'].items())
            if parameters != "":
                fqbn += ":" + parameters
        return "{} upload --fqbn {} {}.ino --port {}".format(ARDUINO_CLI, fqbn, context['ino'], port)

    for board in context['boards']:
        yield {
            'basename': 'arduino-upload',
            'name': board['nick'],
            'params': [
                {'name': 'port',
                 'short': 'p',
                 'long': 'port',
                 'type': str,
                 'default': 'auto',
                 },
            ],
            'actions': [CmdAction((do_upload, [board], {}))],
            'task_dep': ['arduino-install-cli',
                         'arduino-compile:{}'.format(board['nick']),
                         ],
            'file_dep': [CONTEXT_FILE],
            'uptodate': [False],  # force to run the task always
            'verbosity': 2,
        }


def task_generate():
    """Generate file using context.yaml and jinja2 template files"""

    def do_generate(template, output):
        yamlinclude.YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=this_dir)
        loader = jinja2.FileSystemLoader(this_dir)

        env = jinja2.Environment(
            loader=loader,
            autoescape=jinja2.select_autoescape()
        )

        jinja_t = env.get_template(template)

        with open(output, 'w') as output_f:
            output_f.write(jinja_t.render(context))
        return context

    working_directory = Path('.')
    for jinja2_file in working_directory.glob('*.jinja2'):
        if jinja2_file.with_suffix('').with_suffix('').name.endswith('driver_cmd'):
            continue
        output_file = jinja2_file.with_suffix('')
        yield {
            'name': output_file.name,
            'actions': [(do_generate, [jinja2_file.name, output_file.name])],
            'file_dep': [jinja2_file.name] + list(glob('*.yaml')),
            'task_dep': ['pip:requirements.txt'],
            'targets': [output_file.name],
            'uptodate': [config_changed(context)],
            'title': show_cmd,
        }


def task_copy_plugins():
    """ Copy the plugin's arduino source files to this arduino directory """
    def do_copy(task):
        # print(task.file_dep)
        exclude_file = os.path.join(find_git_repo(__file__), '.git', 'info', 'exclude')
        with open(exclude_file, "r") as ge:
            git_exclude = [x.strip() for x in ge.readlines()]

        for src in task.file_dep:
            destination = Path(src).name
            if ("i2c-stick-arduino/" + destination) not in git_exclude:
                git_exclude.append("i2c-stick-arduino/" + destination)
                with open(exclude_file, "w") as ge:
                    ge.write('\n'.join(git_exclude))

            must_copy = 0
            if os.path.isfile(destination):
                if os.stat(src).st_mtime - os.stat(destination).st_mtime > 1:
                    must_copy = 1  # when older!
            else:
                must_copy = 1  # when new file!
            if must_copy:
                print(f"copy plugin file: {src} => {destination}")
                shutil.copy2(src, destination)

    return {
        'basename': 'copy-plugins',
        'actions': [do_copy],
        'file_dep': list(glob('../plugins/*/i2c-stick-arduino/*')),
        'verbosity': 2,
    }


def task_add_driver():
    """Add a templated entry for a new driver for a sensor to the framework"""

    def do_generate(template, output, data):
        yamlinclude.YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=this_dir)
        loader = jinja2.FileSystemLoader(this_dir)

        env = jinja2.Environment(
            loader=loader,
            autoescape=jinja2.select_autoescape()
        )

        tpl = env.get_template(template)
        print("output:", output)
        with open(output, 'w') as output_f:
            output_f.write(tpl.render(data))
            output_f.write("\n")

    def do_add_driver(driver, src_name, function_id, sa_list):
        if driver is None:
            print("Please provide parameters about the drivers")
            print("Run 'doit info add-driver' for more information")
            return
        if src_name is None:
            src_name = driver.lower()
        if function_id is None:
            if src_name.startswith('mlx'):
                function_id = src_name[3:]
        if function_id is None:
            print("Please provide parameters about the drivers")
            print("Run 'doit info add-driver' for more information")
            return
        if len(sa_list) == 0:
            print("Please provide parameters about the drivers")
            print("Run 'doit info add-driver' for more information")
            return

        driver_data = {'driver': {
            'name': driver,
            'src_name': src_name,
            'function_id': function_id,
            'sa_list': sa_list,
            },
        }

        for d in context['drivers']:
            if d['name'] == driver:
                print("ERROR: Driver '{}' already exists;".format(driver) +
                      " edit the file 'context.yaml' manually to resolve this issue")
                return

        do_generate("driver_cmd.h.jinja2", "{}_cmd.h".format(src_name), driver_data)
        do_generate("driver_cmd.cpp.jinja2", "{}_cmd.cpp".format(src_name), driver_data)
        # now update the context.yaml file!

        yaml_file = src_name+"_driver.yaml"
        print("output:", yaml_file)
        with open(yaml_file, 'w') as output_f:
            output_f.write(yaml.dump({
                'name': driver,
                'src_name': src_name,
                'function_id': function_id,
            }))
            output_f.write("\n")

        # and finally re-generate the dispatcher for the newly added driver.
        # from doit.doit_cmd import DoitMain
        # DoitMain().run(["--always", "generate:i2c_stick_dispatcher.h"])
        # DoitMain().run(["--always", "generate:i2c_stick_dispatcher.cpp"])

    return {
        'basename': 'add-driver',
        'actions': [(do_add_driver,)],
        'file_dep': ['driver_cmd.h.jinja2'],
        'params': [
            {'name': 'driver',
             'short': 'd',
             'long': 'driver',
             'type': str,
             'default': None,
             },
            {'name': 'src_name',
             'short': 's',
             'long': 'src_name',
             'type': str,
             'default': None,
             },
            {'name': 'function_id',
             'short': 'f',
             'long': 'function_id',
             'type': str,
             'default': None,
             },
            {'name': 'sa_list',
             'short': 'a',
             'long': 'sa',
             'type': list,
             'default': [],
             },
        ],
        'uptodate': [False],  # make to run the task always
        'verbosity': 2,
    }


def task_dist():
    def make_dist_dir():
        if not Path("../dist").is_dir():
            os.mkdir('../dist')

    def do_copy():
        shutil.copytree("build", "../dist", dirs_exist_ok=True)


    return {
        'actions': [(make_dist_dir,),
                    (do_copy, )
                    ],
        'verbosity': 2,
        'task_dep': ['arduino-compile', ],
        'title': show_cmd,
        'uptodate': [False],
    }


if __name__ == '__main__':
    import doit

    doit.run(globals())
