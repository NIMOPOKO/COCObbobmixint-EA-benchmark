import subprocess
from concurrent.futures import ThreadPoolExecutor
import os

# NUMAノードの設定（NUMAノード0と1にジョブを分ける）
numa_nodes = [0, 1]

# オプションの組み合わせを定義（[Lamarckian/Baldwinian, Normal/Revised]の順）
job_options = [
    (0, 0, 0),  # DEラマルク型
    (0, 0, 1),  # DEボールドウィン型
    (0, 1, 0),  # UDEラマルク型疑似round
    (0, 1, 1),  # UDEラマルク型middle
    (0, 1, 2),  # UDEラマルク型best
    (0, 1, 3),   # UDEボールドウィン型
    (0, 2, 0),  # U2DEラマルク型
    (0, 2, 1)   # U2DEボールドウィン型   
]

# 非同期にジョブを実行するためのプロセスリスト
processes = []

def submit_job_make(i, numa_node):
    algorithm, encoding, approach = job_options[i]
    # ジョブスクリプトの内容を定義
    job_script = f"""#!/bin/bash
#PBS -N MymixintDEJob_{i}
#PBS -l nodes=1:ppn=4:mem=16gb
#PBS -l numactrl={numa_node}  # NUMAノードの指定
#PBS -o ./out/output_{i}.log
#PBS -e ./out/error_{i}.log

make ALGORITHM={algorithm} ENCODING={encoding} APPROACH={approach} NUM={i}
"""

    # ジョブスクリプトを一時ファイルに保存
    script_name = f"sh/temp_job_{i}.sh"
    with open(script_name, "w") as file:
        file.write(job_script)

    # 実行権限を付与
    os.chmod(script_name, 0o755)

    # nohupでバックグラウンド実行
    process = subprocess.Popen(
        ["nohup", script_name, "&"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    processes.append(process)

    # 各ジョブの結果を表示
    for i, process in enumerate(processes):
        stdout, stderr = process.communicate()  # 出力とエラーを取得
        print(f"Job {i} submitted:", stdout)
        print("Errors:", stderr)

def submit_job_exe(i, numa_node):
    # ジョブスクリプトの内容を定義
    job_script = f"""#!/bin/bash
#PBS -N MymixintDEJob_{i}
#PBS -l nodes=1:ppn=4:mem=16gb
#PBS -l numactrl={numa_node}  # NUMAノードの指定
#PBS -o ./out/output_{i}.log
#PBS -e ./out/error_{i}.log

./example_experiment{i}
"""

    # ジョブスクリプトを一時ファイルに保存
    script_name = f"sh/temp_job_{i}.sh"
    with open(script_name, "w") as file:
        file.write(job_script)

    # nohupでバックグラウンド実行
    process = subprocess.Popen(
        ["nohup", script_name, "&"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    processes.append(process)

    # 各ジョブの結果を表示
    for i, process in enumerate(processes):
        stdout, stderr = process.communicate()  # 出力とエラーを取得
        print(f"Job {i} submitted:", stdout)
        print("Errors:", stderr)

# ThreadPoolExecutorで並列実行
with ThreadPoolExecutor(max_workers=8) as executor:
    for i in range(0,8):
        # NUMAノードを交互に割り当て
        numa_node = numa_nodes[i % len(numa_nodes)]
        executor.submit(submit_job_make, i, numa_node)

with ThreadPoolExecutor(max_workers=8) as executor:
    for i in range(0,8):
        # NUMAノードを交互に割り当て
        numa_node = numa_nodes[i % len(numa_nodes)]
        executor.submit(submit_job_exe, i, numa_node)