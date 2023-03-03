import os
import sys
import yaml

from jinja2 import Environment, FileSystemLoader, select_autoescape

# Capture our current directory
THIS_DIR = os.path.dirname(os.path.abspath(__file__))


from yamlinclude import YamlIncludeConstructor

YamlIncludeConstructor.add_to_loader_class(loader_class=yaml.FullLoader, base_dir=THIS_DIR)
loader=FileSystemLoader(THIS_DIR)



import argparse

argParser = argparse.ArgumentParser()
argParser.add_argument("-c", "--context", default="context.yaml", help="the context yaml file")
argParser.add_argument("-t", "--template",                        help="the template file to be used")
argParser.add_argument("-o", "--output",                          help="the output file name")

args = argParser.parse_args()


if args.template is None:
    print("ERROR: Please supply a template file")
    sys.exit(0)

if args.output is None:
    output = args.template.replace(".jinja2", "").replace("jinja2", "")
    args.output = output

if args.output == args.template:
    print("ERROR: Please supply an output file different from template file.")
    sys.exit(0)


with open(args.context, 'r') as file:
    context = yaml.load(file, Loader=yaml.FullLoader)


env = Environment(
    loader=loader,
    autoescape=select_autoescape()
)

template = env.get_template(args.template)

with open(args.output, 'w') as f:
    f.write(template.render(context))
