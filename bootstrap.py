# coding: utf-8
#
# Copyright (c) 2019 quentinl-c.
#
# This file is part of ABCShadow 
# (see https://github.com/quentinl-c/ABCShadow_article_assets).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

import argparse
import json
from multiprocessing import Pool
from os import cpu_count
from pathlib import Path
import subprocess
# Default value
INPUT_FILE = './team_stats.json'
CPU_COUNT = min(cpu_count(), 25)
CMD = "./shadow"
ACTION = "abc_estim"


def exec_abc(config_file, name, y_obs, size):
    args = [ACTION, config_file, name, ','.join(map(str, y_obs)), *map(str,size)]
    cmd = [CMD]
    cmd.extend(args)
    print(cmd)
    p = subprocess.Popen(cmd)
    p.wait()
    print(f"{name} ended with returncode {p.returncode}")
   
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('input_file', type=str, default=INPUT_FILE)
    parser.add_argument('config_file', type=str, default="./config.txt")
    parser.add_argument('output_dir', type=str, default="./")

    args = parser.parse_args()

    input_file = Path(args.input_file)
    config_file = Path(args.config_file)

    if not input_file.is_file():
        raise ValueError("Invalid input file !")

    if not config_file.is_file():
        raise ValueError("Invalide config file !")

    # Ensure output directory exists
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    with open(input_file, 'r') as json_f:
        teams = json.load(json_f)

    params = [(args.config_file, str(output_dir / name), team["y_obs"], team["size"]) for name, team in teams.items()]

    pool = Pool(min(CPU_COUNT, len(params)))
    pool.starmap(exec_abc, params)
