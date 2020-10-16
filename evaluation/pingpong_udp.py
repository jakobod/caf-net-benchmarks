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

from matplotlib.ticker import ScalarFormatter
from pathlib import Path

my_cs = ["#375E97", "#FB6542"]
plt.rc('figure', figsize=(4*1.3, 2.5*1.2))
plt.rc('font', size=12)


def calculate(file, label):
  print(f'interpreting {file}')
  print(f'reading {file}')
  with open(file, 'r', newline='') as f:
    # [('key1', 'mean', 'sdev')]
    message_sizes = []
    vals = []
    labels = []
    for i, line in enumerate(f):
      # Skip first line with labels
      if i == 0:
        continue
      split_line = line.rstrip(',\n').split(', ')
      message_size = int(split_line[0])
      values = split_line[1:-1]
      values = np.array(values).astype(np.long)
      for i in range(len(values)):
        message_sizes.append(message_size)
      # values are in useconds and should be displayed in mseconds
      vals.extend(values/1000)
      labels = [label] * len(message_sizes)
      # sdevs.append(np.std(values))
    return {'message_size': message_sizes, 'values': vals, 'label': labels}


def x_formatter(x, pos):
  if x >= 1024:
    return '{:>}Ki'.format(int(x / 1024))
  return '{:>}'.format(int(x))


def main():
  pingpong_udp = calculate(
      'evaluation/out/pingpong-udp-net-message-size.out', 'udp')
  udp_df = pd.DataFrame(pingpong_udp, columns=[
      'message_size', 'values', 'label'])

  # Apply the default theme
  # sns.set_theme()
  sns.set_style("ticks")
  udp_df = pd.DataFrame(pingpong_udp, columns=[
      'message_size', 'values', 'label'])

  # Apply the default theme
  sns.set_style("ticks")
  # ax = sns.lineplot(data=df, x="message_size", y="values",
  #                   hue="label", style="label", markers=True,
  #                   dashes=False, err_style="bars", err_kws={'capsize': 5})

  palette = iter(sns.color_palette())
  colors = [next(palette) for i in range(0, 3)]
  ax = sns.lineplot(data=udp_df, palette=[colors[2]], x="message_size", y="values",
                    hue="label", style="label", markers="s",
                    dashes=False, err_style="bars", err_kws={'capsize': 5})
  # ax = sns.lineplot(data=net_df, palette=[colors[1]], x="message_size", y="values",
  #                   hue="label", style="label", markers='x',
  #                   dashes=False, err_style="bars", err_kws={'capsize': 5})
  # ax = sns.lineplot(data=udp_df, palette=[colors[2]], x="message_size", y="values",
  #                   markers='+', s=10, dashes=False, err_style="bars", err_kws={'capsize': 5})
  plt.ylim(0, 8700)
  ax.tick_params(bottom=True, top=True, left=True, right=True, direction="in")
  ax.legend(loc='upper center',
            ncol=4, fancybox=False, shadow=False, bbox_to_anchor=(0.49, 1.20),)
  ax.set_xscale('log', base=2)
  for axis in [ax.xaxis, ax.yaxis]:
    axis.set_major_formatter(ScalarFormatter())
  ax.set(xlabel='Message Size [Byte]', ylabel='Duration [ms]')
  plt.ticklabel_format(style='plain', axis='y')
  ax.xaxis.set_major_formatter(plt.FuncFormatter(x_formatter))

  plotname = 'figs/pingpong_udp.pdf'
  print(f'plotting {plotname}')

  plt.savefig(plotname, bbox_inches='tight')
  # # # Clear plt object for next plot.. Maybe
  plt.clf()
  plt.cla()
  plt.close()


if __name__ == '__main__':
  main()
