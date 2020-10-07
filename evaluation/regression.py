#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Analyze some malware source things ...
"""

__maintainer__ = "Raphael Hiesgen"
__email__ = "raphael.hiesgen@haw-hamburg.de"
__copyright__ = "Copyright 2018-2020"

import argparse
import csv
import os

import matplotlib.colors as mpc
import matplotlib.dates as md
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

from collections import defaultdict
from datetime import datetime, timedelta, timezone
from pathlib import Path

my_cs = ["#375E97", "#FB6542"]
# plt.rc('figure', figsize=(8 * 0.7, 3 * 0.7))
plt.rc('font', size=12)


# def round_to_hour(ts):
#     if ts % 3600 != 0:
#         # int(datetime.timestamp(itr_date))
#         unix = datetime.fromtimestamp(ts).astimezone(timezone.utc)
#         formatted = unix.strftime('%Y-%m-%d %H:%M:%S')
#         print(f'{ts} --> {formatted}')
#         exit()
#     return ts


def main():
  parser = argparse.ArgumentParser(description='Plot Given CSV Data')
  parser.add_argument('input', type=str, help='file to process')
  args = parser.parse_args()

  if not Path(args.input).is_file():
    print('input must be a file')
    return
    
  file = args.input
  print(f'reading {file}')
  with open(file, 'r', newline='') as fh:
    # header = ['host', 'server', 'event', 'type',
    # 'url', 'malware', 'port', 'hash', 'timestamp']
    reader = csv.DictReader(fh)
    for row in reader:
      print(row["name"])
      
  # _, ax = plt.subplots()
  # for each malware, make a line that connects each active day.
  # malware_to_time_tp[malware].add(hour)
  # x,y,yerr = rand(3,10)
  fig = plt.figure()
  x = np.arange(1,11)
  y = 2.5 * np.sin(x / 20 * np.pi)
  yerr = x/5

  plt.grid(True)
  plt.errorbar(x, y, yerr=yerr, capsize=5)

  # plt.plot(xs_1, ys_1, label="hoch", color=my_cs[0], marker="*")
  # plt.plot(xs_2, ys_2, label="runter", color=my_cs[1], marker=">")

  # def format_x(x, pos):
  #     date = datetime.fromtimestamp(x)
  #     pretty = date.astimezone(timezone.utc).strftime('%d.%m@%Hh')
  #     return pretty

  # ax.xaxis.set_major_formatter(plt.FuncFormatter(format_x))

  # plt.grid(True)
  plt.title('Malware activity', loc='left', fontsize=18)

  plt.xlabel('message size [Byte]')  # , fontsize=20)
  plt.ylabel('Duration [ms]')  # , fontsize=20)

  # plt.legend(bbox_to_anchor=(0, 0, .4, 1))

  # plt.yticks(y_ticks, y_labels)
  # plt.xticks(rotation=45)
  # ax.set_yticklabels(y_labels)

  # plt.xticks(fontsize=17)
  # plt.yticks(fontsize=17)

  tag = 1
  plotname = f'test-{tag}.pdf'
  print(f'plotting {plotname}')

  plt.savefig(plotname)
  # Clear plt object for next plot.. Maybe
  plt.clf()
  plt.cla()
  plt.close()


if __name__ == '__main__':
  main()
