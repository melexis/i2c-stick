import os
import yaml

from jinja2 import Environment, FileSystemLoader, select_autoescape

# Capture our current directory
THIS_DIR = os.path.dirname(os.path.abspath(__file__))


from yamlinclude import YamlIncludeConstructor

YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=THIS_DIR)


loader=FileSystemLoader(THIS_DIR)

with open('context.yml', 'r') as file:
    context = yaml.load(file, Loader=yaml.FullLoader)


env = Environment(
    loader=loader,
    autoescape=select_autoescape()
)

template = env.get_template("index.jinja2.html")

with open('index.html', 'w') as f:
    f.write(template.render(context))
