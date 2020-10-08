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
  streaming_net = calculate(
      'evaluation/out/blank-streaming-net-message-size.out', 'net')
  streaming_io = calculate(
      'evaluation/out/blank-streaming-io-message-size.out', 'io')

  io_df = pd.DataFrame(streaming_io, columns=[
      'message_size', 'values', 'label'])
  net_df = pd.DataFrame(streaming_net, columns=[
      'message_size', 'values', 'label'])
  frames = [io_df, net_df]
  df = pd.concat(frames)
  # Apply the default theme
  sns.set_theme()
  fig, ax = plt.subplots()
  ax.set_xscale('log', base=2)
  ax.set(xlabel='message size [Byte]', ylabel='duration [ms]')
  plt.ticklabel_format(style='plain', axis='y')

  sns.lineplot(data=df, x="message_size", y="values",
               hue="label", err_style="bars")

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

  plotname = 'figs/blank_streaming.pdf'
  print(f'plotting {plotname}')

  plt.savefig(plotname)
  # # # Clear plt object for next plot.. Maybe
  plt.clf()
  plt.cla()
  plt.close()


if __name__ == '__main__':
  main()
