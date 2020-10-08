#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Plot csv data
"""

__maintainer__ = "Jakob Otto"
__email__ = "jakob.otto@haw-hamburg.de"
__copyright__ = "Copyright 2020"

import argparse
import csv
import os

import matplotlib.colors as mpc
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

from pathlib import Path

my_cs = ["#375E97", "#FB6542"]
# plt.rc('figure', figsize=(8 * 0.7, 3 * 0.7))
plt.rc('font', size=12)

def calculate(file):
  print(f'interpreting {file}')
  print(f'reading {file}')
  with open(file, 'r', newline='') as f:
    # [('key1', 'mean', 'sdev')]
    runs = []
    vals = []
    for i, line in enumerate(f):
      # Skip first line with labels
      if i == 0:
        continue
      split_line = line.rstrip(',\n').split(', ')
      run = int(split_line[0])
      values = split_line[1:]
      values = np.array(values).astype(np.long)
      for r in range(run):
        runs.append(run)
      vals.extend(values)
      # sdevs.append(np.std(values))
    return {'runs': runs, 'values': vals}


def main():
  # parser = argparse.ArgumentParser(description='Plot Given CSV Data')
  # parser.add_argument(
  #     'file', type=str, help='file to process')
  # parser.add_argument(
  #     'plotname', type=str, help='name of the resulting pdf')
  # args = parser.parse_args()

  # if not Path(args.file).is_file():
  #   print('`file` must be a file!')
  #   return
  reg = calculate('evaluation/regression/regression.out')
  df = pd.DataFrame(reg, columns = ['runs', 'values'])
  print (df)

  # Apply the default theme
  sns.set_theme()
  sns.boxplot(data=df, x="runs", y="values")

  # plt.grid(True)
  # plt.errorbar(reg[0], reg[1], yerr=reg[2], capsize=5)
  # # Plot Description
  # plt.title('Convergence', loc='left', fontsize=18)
  # plt.xlabel('accumulated runs [#]')
  # plt.ylabel('Duration [ms]')

  # # plt.legend(bbox_to_anchor=(0, 0, .4, 1))

  # # plt.yticks(y_ticks, y_labels)
  # # plt.xticks(rotation=45)
  # # ax.set_yticklabels(y_labels)

  # # plt.xticks(fontsize=17)
  # # plt.yticks(fontsize=17)

  plotname = 'convergence.pdf'
  print(f'plotting {plotname}')

  plt.savefig(plotname)
  # # # Clear plt object for next plot.. Maybe
  plt.clf()
  plt.cla()
  plt.close()


if __name__ == '__main__':
  main()
