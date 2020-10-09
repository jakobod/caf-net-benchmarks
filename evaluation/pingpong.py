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
  # pingpong_net_fix = calculate(
  #     'evaluation/out/blank-pingpong-net-message-size-prefix-fix.out', 'net-fix')
  # pingpong_io_fix = calculate(
  #     'evaluation/out/blank-pingpong-io-message-size-prefix-fix.out', 'io-fix')
  pingpong_net = calculate(
      'evaluation/out/pingpong-tcp-net-message-size.out', 'net')
  pingpong_io = calculate(
      'evaluation/out/pingpong-tcp-io-message-size.out', 'io')
  pingpong_raw = calculate(
      'evaluation/out/pingpong-tcp-raw-message-size.out', 'raw')

  # io_fix_df = pd.DataFrame(pingpong_io_fix, columns=[
  #     'message_size', 'values', 'label'])
  # net_fix_df = pd.DataFrame(pingpong_net_fix, columns=[
  #     'message_size', 'values', 'label'])
  io_df = pd.DataFrame(pingpong_io, columns=[
      'message_size', 'values', 'label'])
  net_df = pd.DataFrame(pingpong_net, columns=[
      'message_size', 'values', 'label'])
  raw_df = pd.DataFrame(pingpong_raw, columns=[
      'message_size', 'values', 'label'])
  frames = [raw_df, io_df, net_df]
  df = pd.concat(frames)
  # Apply the default theme
  sns.set_theme()
  ax = sns.lineplot(data=df, x="message_size", y="values",
                    hue="label", err_style="bars", err_kws={'capsize': 5})
  ax.set_xscale('log', base=2)
  ax.set(xlabel='message size [Byte]', ylabel='duration [ms]')
  plt.ticklabel_format(style='plain', axis='y')
  plt.title('Pingpong', loc='left', fontsize=18)

  plotname = 'figs/pingpong.pdf'
  print(f'plotting {plotname}')

  plt.savefig(plotname)
  # # # Clear plt object for next plot.. Maybe
  plt.clf()
  plt.cla()
  plt.close()


if __name__ == '__main__':
  main()
