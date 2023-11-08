# todo:
# [ ] bumpver version synchronization between firmware / web / python
# [ ]

import sys
import subprocess

CONTEXT_FILE = "context.yaml"

DOIT_CONFIG = {'action_string_formatting': 'both',
               'default_tasks': ['web'],
               }
PIP_USER = " --user "
if sys.prefix != sys.base_prefix:  # check if we are in a virtual environment
    PIP_USER = ""

try:
    import doit
except (Exception,) as e:
    print("installing python packages from requirements.txt")
    t = subprocess.check_output('pip install {} -r requirements.txt'.format(PIP_USER), text=True)
    print(t)
    print("\n\nPlease run this script again")
    print("Note: instead of 'python dodo.py', now you can use 'doit'")
    print("Note: use 'doit list' for a list of commands.")
    exit(1)

import shutil
import os
import yaml
import jinja2
import yamlinclude
from glob import glob
from pathlib import Path
# from doit.action import CmdAction
from doit.tools import run_once
import platform
import urllib.request
from html.parser import HTMLParser

from yamlinclude import YamlIncludeConstructor

THIS_DIR = os.path.dirname(os.path.abspath(__file__))

YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=THIS_DIR)

cmd_suffix = ''
system = platform.system().lower()
if system == 'windows':
    cmd_suffix = '.cmd'

TERSER = os.path.join('theme', 'node_modules', '.bin', 'terser' + cmd_suffix)
CSSNANO = os.path.join('theme', 'node_modules', '.bin', 'cssnano' + cmd_suffix)
PANDOC = os.path.join('tools', 'pandoc' + Path(sys.executable).suffix)

with open(CONTEXT_FILE) as f:
    context = yaml.load(f, Loader=yaml.FullLoader)


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


def task_install_nodejs():
    """Install the Node.js (and npm) tool"""

    class MyHTMLParser(HTMLParser):
        def __init__(self):
            super().__init__()
            self.pkg_list = []

        def handle_starttag(self, tag, attrs):
            if tag == 'a':
                for attr in attrs:
                    if attr[0] == 'href':
                        if attr[1].startswith('node-v'):
                            # yes, we have a valid href link to a package
                            self.pkg_list.append(attr[1])

    def do_install():
        if not Path("tools").is_dir():
            os.mkdir('tools')
        this_system = platform.system().lower()
        os_dict = {
            'linux': 'linux',
            'windows': 'win',
            'darwin': 'darwin',
        }
        node_url = "https://nodejs.org/download/release/latest-v19.x"
        parser = MyHTMLParser()
        fp = urllib.request.urlopen(node_url)
        html = fp.read().decode("utf8")

        parser.feed(html)
        version = ""
        for pkg in parser.pkg_list:
            s = pkg.split("-")
            if len(s) > 3:
                print("nodejs version:", s[1])
                version = s[1]
                break

        bits = 'x86'
        if sys.maxsize > 2 ** 32:
            bits = 'x64'

        zip_suffix = 'tar.gz'
        if this_system == 'windows':
            zip_suffix = 'zip'

        url = node_url + "/node-{}-{}-{}.{}".format(version, os_dict[this_system], bits, zip_suffix)

        print("downloading:", url)

        import io
        import zipfile
        from contextlib import closing
        import requests

        r = requests.get(url)
        with closing(r), zipfile.ZipFile(io.BytesIO(r.content)) as archive:
            for member in archive.infolist():
                l = list(Path(member.filename).parts)
                l[0] = 'tools'
                output = os.sep.join(l)

                if member.is_dir():
                    if not Path(output).is_dir():
                        os.mkdir(output)
                else:
                    with open(output, "wb") as file:
                        file.write(archive.read(member))
        return

    return {
        'basename': 'install-nodejs',
        'actions': [(do_install,)],
        'verbosity': 2,
        'uptodate': [run_once],
    }


def task_install_nodejs_packages():
    """Install nodejs packages: bulma css framework, cssnano, terser"""
    return {
        'basename': 'install-nodejs-package',
        'actions': ["cd theme && npm install"],
        'file_dep': ['theme/package.json'],
        'verbosity': 2,
        'uptodate': [run_once],
    }





def task_install_pandoc():
    """Install the pandoc tool (markdown converter)"""

    def do_install():
        if not Path("tools").is_dir():
            os.mkdir('tools')
        this_system = platform.system().lower()
        suffix_dict = {
            'linux': 'linux-amd64.tar.gz',
            'windows': 'windows-x86_64.zip',
            'darwin': 'x86_64-macOS.zip',
        }
        suffix = suffix_dict[this_system]
        version = '3.1.9'

        url = f"https://github.com/jgm/pandoc/releases/download/{version}/pandoc-{version}-{suffix}"

        print("downloading:", url)

        import io
        import zipfile
        from contextlib import closing
        import requests

        r = requests.get(url)
        with closing(r), zipfile.ZipFile(io.BytesIO(r.content)) as archive:
            for member in archive.infolist():
                l = list(Path(member.filename).parts)
                l[0] = 'tools'
                if len(l) > 1:
                    if l[1] == 'bin':
                        del l[1]
                    if not l[1].startswith('pandoc'):
                        continue
                output = os.sep.join(l)
                if member.is_dir():
                    if not Path(output).is_dir():
                        os.mkdir(output)
                else:
                    print(f"unzip file: {output}\n")
                    with open(output, "wb") as file:
                        file.write(archive.read(member))
        return

    return {
        'basename': 'install-pandoc',
        'actions': [(do_install,)],
        'verbosity': 2,
        'uptodate': [run_once],
    }


def task_generate():
    """Generate file using context.yaml and jinja2 template files"""

    def do_generate(template, output):
        this_dir = os.path.dirname(os.path.abspath(__file__))
        yamlinclude.YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=this_dir)
        loader = jinja2.FileSystemLoader(this_dir)

        env = jinja2.Environment(
            loader=loader,
            autoescape=jinja2.select_autoescape()
        )

        jinja_t = env.get_template(template)

        with open(output, 'w') as output_f:
            output_f.write(jinja_t.render(context))

    working_directory = Path('.')
    for jinja2_file in working_directory.glob('*.jinja2'):
        if jinja2_file.with_suffix('').with_suffix('').name.endswith('driver_cmd'):
            continue
        output_file = jinja2_file.with_suffix('')
        yield {
            'name': output_file.name,
            'actions': [(do_generate, [jinja2_file.name, output_file.name])],
            'file_dep': [jinja2_file.name],
            'task_dep': ['pip:requirements.txt'],
            'targets': [output_file.name],
            'title': show_cmd,
        }


def task_bulma():
    return {
        'targets': ['melexis-bulma.css'],
        'actions': ["cd theme && npm run css-build", ],
        'file_dep': ['theme/sass/melexis-bulma.scss'],
        'task_dep': [],  # add task to install the theme
        'title': show_cmd,
    }


def task_minify_js():
    working_directory = Path('.')
    js_files = list(working_directory.glob('*.js'))
    for js_file in js_files:
        if js_file.with_suffix("").suffix == '.min':
            continue
        min_js_file = js_file.with_suffix("").with_suffix(".min.js")
        yield {
            'name': min_js_file,
            'actions': ["{} {} --compress --mangle --toplevel --output {}".format(TERSER, js_file, min_js_file), ],
            'file_dep': [js_file],
            'task_dep': ['pip:requirements.txt'],
            'targets': [min_js_file],
            'title': show_cmd,
        }


def task_minify_css():
    working_directory = Path('.')
    css_files = list(working_directory.glob('*.css'))

    if not Path("melexis-bulma.css") in css_files:
        css_files.append(Path("melexis-bulma.css"))

    for css_file in css_files:
        if css_file.with_suffix("").suffix == '.min':
            continue
        min_css_file = css_file.with_suffix("").with_suffix(".min.css")
        yield {
            'name': min_css_file,
            'actions': ["{} {} {}".format(CSSNANO, css_file, min_css_file), ],
            'file_dep': [css_file],
            'task_dep': ['pip:requirements.txt'],
            'targets': [min_css_file],
            'title': show_cmd,
        }


def task_convert_md():
    working_directory = Path('.')
    md_files = list(working_directory.glob('*.md'))
    for md_file in md_files:
        html_file = md_file.with_suffix("").with_suffix(".html")
        yield {
            'name': html_file,
            'actions': [
                "{} {} -f markdown+lists_without_preceding_blankline+autolink_bare_uris+hard_line_breaks+smart --mathjax -o {}".format(
                    PANDOC, md_file, html_file), ],
            'file_dep': [md_file],
            'task_dep': ['pip:requirements.txt'],
            'targets': [html_file],
            'title': show_cmd,
        }


def task_web():
    def do_generate(template, output):
        this_dir = os.path.dirname(os.path.abspath(__file__))
        yamlinclude.YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=this_dir)
        loader = jinja2.FileSystemLoader(this_dir)

        env = jinja2.Environment(
            loader=loader,
            autoescape=jinja2.select_autoescape()
        )

        jinja_t = env.get_template(template)

        with open(output, 'w') as output_f:
            output_f.write(jinja_t.render(context))

    jinja2_file = Path("index.jinja2.html")
    html_file = jinja2_file.with_suffix("").with_suffix(".html")
    return {
        'actions': [(do_generate, [jinja2_file.name, html_file.name])],
        'file_dep': [jinja2_file.name,
                     'interface.min.js',
                     'melexis-bulma.min.css',
                     'products.html',
                     'i2c-stick.html',
                     'firmware.html',
                     'firmware_list.html',
                     ],
        'task_dep': ['pip:requirements.txt',
                     'bulma',
                     'convert_md',
                     'minify_css',
                     'minify_js',
                     ],
        'targets': [html_file.name],
        'title': show_cmd,
    }


def task_serve():
    return {
        'actions': ['python -m http.server'],
        'title': show_cmd,
        'file_dep': ['index.html'],
    }


if __name__ == '__main__':
    import doit

    doit.run(globals())
