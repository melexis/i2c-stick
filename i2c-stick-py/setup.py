from setuptools import setup
import sys
import platform
from setuptools import find_namespace_packages


version='0.1.2'

requires = ['bincopy>=17.14.5',
            'pyserial>=3.5',
            ]

with open("README.md", "r") as fh:
    long_description = fh.read()

# This is a `native namespace package`
#
# See more info here:
# https://packaging.python.org/en/latest/guides/packaging-namespace-packages/#native-namespace-packages
# 

setup(
    name='melexis-i2c-stick',
    version=version,
    description='Python package for I2C Stick',
    long_description=long_description,
    long_description_content_type="text/markdown",
    license='Apache License, Version 2.0',
    install_requires=requires,
    url = 'https://github.com/melexis/i2c-stick',   # Provide either the link to your github or to your website
    packages=find_namespace_packages(include=['mynamespace.*']),
    classifiers=[
        # complete classifier list: http://pypi.python.org/pypi?%3Aaction=list_classifiers
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Topic :: Utilities',
    ],
    zip_safe=False,
)
