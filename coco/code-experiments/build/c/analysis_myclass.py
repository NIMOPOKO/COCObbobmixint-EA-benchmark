import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.ndimage import gaussian_filter1d

def process_and_plot(directory, groups, output_directory, output_filename):
    """
    指定されたグループのデータを処理し、指定した形式でグラフを描画します。

    Parameters:
        directory (str): データが存在するディレクトリのパス。
        groups (list of str): データファイルのグループ名リスト。
        output_directory (str): 出力ディレクトリ。
        output_filename (str): 出力ファイル名。
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
    fig, ax = plt.subplots(figsize=(10, 6))

    # 凡例用のデータを保存
    legend_info = []
    for group, (x, y) in data_by_group.items():
        smoothed_y = gaussian_filter1d(y, sigma=1.0)  # 平滑化
        ax.plot(x, smoothed_y, label=None, color=color_map[group], linewidth=2)  # 凡例なしで描画
        legend_info.append((group, x[-1], smoothed_y[-1], color_map[group]))  # グラフ右端の値を保存

    # 軸ラベルとタイトルを設定
    plt.xlabel("log10(# f-evals / dimension)", fontsize=12)
    plt.ylabel("Fraction of function, target pairs", fontsize=12)
    plt.title("Benchmarking Comparison", fontsize=14)
    plt.ylim(0, 1)
    plt.xlim(0, 4)
    plt.grid(True, linestyle="--", linewidth=0.5)

    # 出力ディレクトリが存在しない場合は作成
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

# グラフ外に凡例を配置し、線を引く
    for idx, (group, x_end, y_end, color) in enumerate(legend_info):
        # グラフ外の凡例位置を計算
        legend_x = 1.05  # グラフ右端の位置（Axes座標系）
        legend_y = 0.9 - idx * 0.2*5/len(groups)  # 凡例間隔を調整（Axes座標系）
        
        # グラフ外に凡例ラベルを配置
        ax.text(legend_x, legend_y, group, fontsize=10, color=color, transform=ax.transAxes)

    # グラフを保存
    output_graph_path = os.path.join(output_directory, output_filename)
    plt.savefig(output_graph_path, bbox_inches='tight')
    print(f"比較グラフを保存しました: {output_graph_path}")
    plt.close()

# 使用例
for i in [1,3,7,15,31]:
    for j in ["5d", "10d", "20d"]:#, "40d", "80d", "160d"]:
        directory = "/home/nimo/de2/coco/code-experiments/build/c/output/de/f1/"+str(i)+"/"+ j
        #output_directory = "/home/nimo/de2/coco/code-experiments/build/c/plot1/"+str(i)
        output_directory = "/home/nimo/de2/coco/code-experiments/build/c/plot2/"+str(i)
        output_filename = j +".png"
        groups = ["L", "U-Lb", "U-Lm", "U-Lb", "U2-L"]
        process_and_plot(directory, groups, output_directory, output_filename)