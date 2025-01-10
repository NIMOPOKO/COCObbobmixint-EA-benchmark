import os
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

def process_and_plot(directory, groups):
    """
    指定されたグループ（例: ["L", "U-Lb",...]）のファイルを処理し、1つのグラフにプロットする。
    
    Parameters:
        directory (str): ファイルが存在するディレクトリのパス。
        groups (list of str): 比較するファイルのグループ名リスト。
    """
    data_by_group = {}
    colors = ['blue', 'orange', 'green', 'red', 'purple']  # グループごとの色を指定
    color_map = {group: colors[i % len(colors)] for i, group in enumerate(groups)}

    for group in groups:
        group_data = None
        
        # 各グループのファイルを順に読み込む
        for i in range(15):  # 0から14までのファイル
            filename = f"{group}-{i}.txt"
            filepath = os.path.join(directory, filename)
            
            if not os.path.isfile(filepath):
                print(f"ファイルが見つかりません: {filepath}")
                continue

            # ファイルを読み込んでデータを処理
            data = np.loadtxt(filepath)
            
            # 各行の2列目の値を加算
            if group_data is None:
                group_data = data[:, 1]  # 初回は2列目を初期化
            else:
                group_data += data[:, 1]  # 同じ行の値を加算
        
        # グループ名をキーとして保存
        data_by_group[group] = (data[:, 0], group_data/765)  # 1列目: x軸, 加算結果: y軸

    # プロット
    plt.figure(figsize=(12, 6))
    legend_lines = []  # 凡例用のLine2Dオブジェクトを格納
    legend_labels = []  # 凡例のラベルを格納

    for group, (x, y) in data_by_group.items():
        color = color_map[group]
        plt.plot(x, y, label=group, color=color, linewidth=2)
        legend_lines.append(Line2D([0], [0], color=color, linewidth=2))
        legend_labels.append(f"{group} data")

    # 凡例を右側に配置し、線でつなぐ
    ax = plt.gca()
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])  # グラフを左に寄せる
    for line, label in zip(legend_lines, legend_labels):
        plt.text(1.02, group_data[400]/765 - 0.05 - 0.05 * legend_labels.index(label), label, transform=ax.transAxes,
                 verticalalignment='center', fontsize=10)
        plt.plot([0.955, 1.02], [group_data[400]/765 - 0.045,group_data[400]/765 - 0.05 - 0.05 * legend_labels.index(label)], color=line.get_color(),
                 transform=ax.transAxes, clip_on=False)

    # グラフの設定
    plt.xlabel("evaluation", fontsize=12)
    plt.ylabel("target hit rate", fontsize=12)
    plt.title("benchmarking", fontsize=14)

    # グリッドと保存
    plt.grid(visible=True, which="both", linestyle="--", linewidth=0.5)


    # グラフを保存
    output_graph_path = os.path.join(directory, "comparison_graph.png")
    plt.savefig(output_graph_path)
    print(f"比較グラフを保存しました: {output_graph_path}")

# 使用例
directory = "/home/nimo/de2/coco/code-experiments/build/c/output/de/f1/5d"
groups = ["L", "U-Lf", "U-Lm", "U-Lb", "U2-L"]
process_and_plot(directory, groups)