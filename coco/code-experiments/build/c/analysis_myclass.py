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
    colors = [(24/255, 116/255, 180/255),    # blue
          (250/255, 112/255, 9/255)]
    color_map = {group: colors[i % len(colors)] for i, group in enumerate(groups)}

    for group in groups:
        group_data = None
        
        # 各グループのファイルを順に読み込む
        for i in range(15):  # 0から14までのファイル
            if group == "U-L":
                filename = f"U-Lb-{i}.txt"
            else:
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
    fig, ax = plt.subplots(figsize=(5.8, 4.5))

    # 凡例用のデータを保存
    legend_info = []
    for group, (x, y) in data_by_group.items():
        smoothed_y = gaussian_filter1d(y, sigma=1.0)  # 平滑化
        ax.plot(x, smoothed_y, label=None, color=color_map[group], linewidth=2)  # 凡例なしで描画
        # legend_info.append((group, x[-1], smoothed_y[-1], color_map[group]))  # グラフ右端の値を保存
        # 縦軸 1 に初めて到達した x を取得
        x_reach_1 = np.min(x[smoothed_y >= 1]) if np.any(smoothed_y >= 1) else float('inf')
        # x=4 の y 値を取得（最も近い点を選択）
        x_4_index = np.argmin(np.abs(x - 4))
        y_at_x_4 = smoothed_y[x_4_index]
        legend_info.append((group, x_reach_1, y_at_x_4, color_map[group]))
    
    # 凡例の並び順を決定
    legend_info.sort(key=lambda v: (-v[2], v[1]))  # (到達順, x=4のy値降順)
    legend_x = 1.05

    # グラフ外に凡例を配置し、線を引く
    for idx, (group, x_reach_1, y_at_x_4, color) in enumerate(legend_info):
        legend_y = 0.9 - idx * 0.2 * 5 / len(groups)  # 凡例間隔を調整（Axes座標系）

        # グラフ外に凡例ラベルを配置
        ax.text(legend_x, legend_y, group, fontsize=15, color="black", transform=ax.transAxes)

        # # x=4 の y 値の高さで水平線を描画
        ax.plot([4, legend_x + 3.15], [y_at_x_4, legend_y +0.025], color=color,linewidth=2,clip_on=False)
        # ax.hlines(y_at_x_4, xmin=3.8, xmax=4.2, colors=color, linestyles='dashed')

    # 軸ラベルとタイトルを設定
    plt.xlabel("log10(# f-evals / dimension)", fontsize=15)
    plt.ylabel("Fraction of function,target pairs", fontsize=15)
    plt.ylim(0, 1)
    plt.xlim(0, 4)
    plt.grid(True, linewidth=0.5)
    # x軸の目盛りを1間隔で設定
    plt.xticks(np.arange(0, 5, 1))  # 0から4まで1間隔
    plt.subplots_adjust(left=0.15, right=0.86, top=0.97, bottom=0.13)
    plt.tick_params(axis='both', labelsize=15)  # 軸とラベルのフォントサイズを14に設定

    # 出力ディレクトリが存在しない場合は作成
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

# # グラフ外に凡例を配置し、線を引く
#     for idx, (group, x_end, y_end, color) in enumerate(legend_info):
#         # グラフ外の凡例位置を計算
#         legend_x = 1.05  # グラフ右端の位置（Axes座標系）
#         legend_y = 0.9 - idx * 0.2*5/len(groups)  # 凡例間隔を調整（Axes座標系）
        
#         # グラフ外に凡例ラベルを配置
#         ax.text(legend_x, legend_y, group, fontsize=10, color=color, transform=ax.transAxes)

    # グラフを保存
    output_graph_path = os.path.join(output_directory, output_filename)
    plt.savefig(output_graph_path)
    print(f"比較グラフを保存しました: {output_graph_path}")
    plt.close()

# 使用例
for func in ["f1", "f3", "f8"]:
    for t in [1,2,3,4]:
        for i in [2,3,4,5,6,7,8,9,10]:
            for j in ["5d", "10d", "20d", "40d", "80d", "160d"]:
                directory = "./output/de/"+func+"/"+str(t)+"/"+str(i)+"/"+ j
                #output_directory = "/home/nimo/de2/coco/code-experiments/build/c/plot1/"+str(i)
                output_directory = "./plotb/"+func+"/"+str(t)+"/"+str(i)
                output_filename = func + "_ratio" + str(t) + "_range" + str(i) + "_" + j +".pdf"
                groups = ["B","U-B"]
                process_and_plot(directory, groups, output_directory, output_filename)