#!/usr/bin/env python
# -*- coding: utf-8 -*-

import io
import time
import sys

from os import path
from os import popen
from os import listdir
from os import remove
from os import chdir
from os import path

from subprocess import PIPE
from subprocess import STDOUT
from subprocess import Popen

from shutil import move
from shutil import copyfile
from shutil import copyfileobj

from glob import glob
from re import search

#stuff for threads:
import commands
from threading import Thread
from Queue import Queue

chdir(project_settings.getLayoutDir())

command = ["/usr/bin/tclsh"]
command.append("/home/leviathan/qtflow/scripts/blif2cel.tcl")
command.append("--lef")
command.append(project_settings.getStandardCellsLEF())
command.append("--blif")
command.append(path.join(project_settings.getSynthesisDir(),project_settings.getTopLevel()+".blif"))
command.append("--cel")
command.append(path.join(project_settings.getLayoutDir(),project_settings.getTopLevel()+".cel"))
command.append("--units")
command.append("100") # is constant 100, qrouter can't handle anything else
command.append("--pad-width")
command.append("100")
command.append("--pad-height")
command.append("100")
if project_settings.isAsic():
	print "Is ASIC"
	command.append("--no-pads") #we will provide our own padframe mapping

p=Popen(command, stdout=PIPE, stdin=PIPE, stderr=STDOUT, bufsize=1)

for line in iter(p.stdout.readline, ''):
	print line

p.stdout.close()

command = ["/usr/bin/tclsh"]
command.append("/home/leviathan/qtflow/scripts/decongest.tcl")
command.append(project_settings.getTopLevel())
command.append(project_settings.getStandardCellsLEF())
command.append("FILL")
command.append("0.1")

p=Popen(command, stdout=PIPE, stdin=PIPE, stderr=STDOUT, bufsize=1)

for line in iter(p.stdout.readline, ''):
	print line

p.stdout.close()


command = ["/usr/bin/tclsh"]
command.append("/home/leviathan/qtflow/scripts/powerbus.tcl")
command.append(project_settings.getTopLevel())
command.append(project_settings.getStandardCellsLEF())
command.append("FILL")

p=Popen(command, stdout=PIPE, stdin=PIPE, stderr=STDOUT, bufsize=1)

for line in iter(p.stdout.readline, ''):
	print line

p.stdout.close()

#---------------------------------------------------
# 2) Prepare DEF and .cfg files for qrouter
#---------------------------------------------------

# First prepare a simple .cfg file that can be used to point qrouter
# to the LEF files when generating layer information using the "-i" option.

cfgfile = open(project_settings.getTopLevel()+".cfg","w")

for f in project_settings.getLibraryFiles():
	cfgfile.write("read_lef "+f+"\n");

cfgfile.write("vdd vdd\n");
cfgfile.write("gnd gnd\n");
cfgfile.write("clk clk\n");

cfgfile.close()

command = []
command.append(settings.getQRouter())
command.append("-i")
command.append(project_settings.getTopLevel()+".info")
command.append("-c")
command.append(project_settings.getTopLevel()+".cfg")

qrouter = Popen(command, stdout=PIPE, stdin=PIPE, stderr=STDOUT, bufsize=1)

for line in iter(qrouter.stdout.readline, ''):
	print line
qrouter.stdout.close()

copyfile(project_settings.getParametersFile(),project_settings.getTopLevel()+".par")

command = []
command.append(settings.getGrayWolf())
command.append("-n")
command.append(project_settings.getTopLevel())
graywolf = Popen(command, stdout=PIPE, stdin=PIPE, stderr=STDOUT, bufsize=1)

for line in iter(graywolf.stdout.readline, ''):
	print line
graywolf.stdout.close()
