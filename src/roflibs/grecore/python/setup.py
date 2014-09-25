#!/usr/bin/env python

# setup.py.in.distutils
#
# Copyright 2012, 2013 Brandon Invergo <brandon@invergo.net>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


from distutils.core import setup, Extension
import platform


if platform.system() == 'Linux':
    doc_dir = '/home/andreas/local/share/doc/grecore'
else:
    try:
        from win32com.shell import shellcon, shell
        homedir = shell.SHGetFolderPath(0, shellcon.CSIDL_APPDATA, 0, 0)
        appdir = 'grecore'
        doc_dir = os.path.join(homedir, appdir)
    except:
        pass

long_desc = \
"""
"""

sgwumgt = Extension('grecore',
                    define_macros = [('MAJOR_VERSION', '0'),
                                     ('MINOR_VERSION', '5')],
                    include_dirs = ['..', '/home/andreas/local/include'],
                    libraries = ['roflibs_grecore', 'rofl'],
                    library_dirs = ['..', '/home/andreas/local/lib'],
		    extra_compile_args = ['-Wno-write-strings'],
                    sources = ['grecoremodule.cpp'])

setup(name='grecore',
      version='0.5',
      author='Andreas Koepsel',
      author_email='rofl@bisdn.de',
      maintainer='',
      maintainer_email='',
      url='ssh://git.bisdn.de:2222/vmcore',
      description="""pythonized version of libroflibs_grecore for control and management""",
      long_description=long_desc,
      download_url='',
      classifiers=[''],
      platforms=[''],
      license='(c) bisdn GmbH',
      requires=['Python (>2.3)'],
      provides=[],
      obsoletes=[],
      packages=[],
      py_modules=[],
      scripts=[],
      data_files=[(doc_dir, [])],
      package_data={},
      ext_modules=[sgwumgt],
     )
