import matplotlib.pyplot as plt
import numpy as np
import sys

# ファイルを読み込む関数
def read_file(filename):
    with open(filename, 'r') as f:
        data = []
        for line in f:
            # 行ごとのデータをスペースで分割して浮動小数点に変換
            row = list(map(float, line.split()))
            data.append(row)
    return np.array(data)

# グラフを描く関数（ファイルに保存）
def save_plot(filename, output_filename, xlim=None, dimention =None,instance=None):
    tmp = np.array([])  # NumPy配列で初期化

    for i in filename:
        data = read_file(i)  # read_fileで読み込んだデータを取得
        if tmp.size == 0:  # tmpが空であれば、最初のデータをそのまま入れる
            tmp = data
        else:  # tmpに要素を加算
            tmp += data  # NumPy配列同士の加算（要素ごとに加算）

    # 最後にすべての要素を15で割る
    tmp /= 15  # NumPy配列の場合、全要素に15を割る

    data = tmp  # 結果をdataに代入

    generations = data.shape[0]  # 行数（世代数）
    variables = data.shape[1]    # 列数（変数の数）

    plt.figure(figsize=(12, 8))

    # 5色のカラーパレット
    colors = ['b', 'g', 'r', 'c', 'm']
    group_size = max(variables // 5, 1)
    for i in range((int)(variables-variables/5)):
        # 変数グループに対応した色とラベルを選択
        color_index = (i // group_size) % 5
        # グループごとの範囲に基づいたラベル設定
        if i % group_size == 0:
            start = i + 1
            end = min(i + group_size, variables)
            if(start == end):
                label = f'Variable {start}'
            else:
                label = f'Variable {start}-{end}'
        else:
            label = None
        # 行ごとの平均が0より大きい場合にマークをする
        for j in range(data.shape[0]):  # 各行を確認
            if data[j, i] > 0:  # 行ごとのデータが0より大きい場合
                plt.scatter(j, data[j, i], color=colors[color_index], marker='o', s=100, zorder=5)  # マークを表示
        plt.plot(data[:, i], label=label, color=colors[color_index])
    # plt.title('Instance '+str(instance+1), fontsize=40)
    plt.title(str(dimention)+'D', fontsize=40)
    plt.xlabel('Generation', fontsize=30)
    plt.ylabel('SD', fontsize=30)
    # plt.yscale('log')
    plt.xticks(np.linspace(0, xlim[1], 5, dtype=int), fontsize=25)  # X軸に5つの目盛り
    plt.yticks(np.linspace(0, 4.0, 5, dtype=int), fontsize=25)  # X軸に5つの目盛り
    # plt.yticks(np.logspace(-8,1,4), fontsize=25) # Y軸に5つの目盛り（値はデータに合わせて調整）
    # plt.title('Generation-wise Variable Values')
    lines, labels = plt.gca().get_legend_handles_labels()  # 左側の凡例
    plt.grid(True)
    # 世代数（X軸）の範囲を設定
    if xlim:
        plt.xlim(xlim)
    plt2 = plt.twinx()
    for i in range((int)(variables-variables/5),variables):
        # 変数グループに対応した色とラベルを選択
        color_index = (i // group_size) % 5
        # グループごとの範囲に基づいたラベル設定
        if i % group_size == 0:
            start = i + 1
            end = min(i + group_size, variables)
            if(start == end):
                label = f'Variable {start}'
            else:
                label = f'Variable {start}-{end}'
        else:
            label = None
        plt2.plot(data[:, i], label=label, color=colors[color_index])
    plt2.set_yscale('log')
    plt2.set_yticks(np.logspace(-8,1,4)) # Y軸に5つの目盛り（値はデータに合わせて調整）
    plt2.tick_params(axis='y', labelsize=25)  # Y軸の目盛りラベルサイズの設定
    lines2, labels2 = plt.gca().get_legend_handles_labels()  # 左側の凡例
    # 凡例を表示
    plt.legend(lines + lines2, labels + labels2, loc='upper right', borderaxespad=0.5,fontsize=30)
    # 画像ファイルに保存
    plt.savefig(output_filename)
    plt.close()

# メイン関数
if __name__ == "__main__":
    args = sys.argv
    method = ["baldwinian","lamarckian"]
    rme = ["baldwinian","nbaldwinian"]
    rme2 = ["lamarckian","nlamarckian_best","nlamarckian_lower","nlamarckian_upper"]
    dimention = [5,10,20,40,80,160]
    filename = []
    # dimention = [20] 
    for f in range(1,25):
        for num in dimention:
                for m in method:
                    if(m == "baldwinian"):
                        if(num == 5):
                            xlim = (0, 20)
                        elif(num == 10):
                            xlim = (0, 20)
                        elif(num == 20):
                            xlim = (0, 30)
                        elif(num == 40):
                            xlim = (0, 35)
                        elif(num == 80):
                            xlim = (0, 40)
                        elif(num == 160):
                            xlim = (0, 50)
                        for roundmethod in rme:
                            for i in range(0,15):
                                filename.append('output/0/'+"f"+str(f)+'/'+str(num)+'d/'+m+'/'+str(num)+'D-'+roundmethod+'-'+str(i)+'.txt')  # 読み込むファイル名
                            output_filename = 'plot1/'+"f"+str(f)+'/'+str(num)+'d/'+m+'/'+str(num)+'D-'+roundmethod+'.pdf'  # 読み込むファイル名  # 保存する画像のファイル名
                            save_plot(filename, output_filename,xlim,num,i)
                            filename = []
                    else:
                        if(num == 5):
                            xlim = (0, 20)
                        elif(num == 10):
                            xlim = (0, 20)
                        elif(num == 20):
                            xlim = (0, 30)
                        elif(num == 40):
                            xlim = (0, 40)
                        elif(num == 80):
                            xlim = (0, 50)
                        elif(num == 160):
                            xlim = (0, 60)
                        for roundmethod in rme2:
                            for i in range(0,15):
                                filename.append('output/0/'+"f"+str(f)+'/'+str(num)+'d/'+m+'/'+str(num)+'D-'+roundmethod+'-'+str(i)+'.txt')  # 読み込むファイル名
                            output_filename = 'plot1/'+"f"+str(f)+'/'+str(num)+'d/'+m+'/'+str(num)+'D-'+roundmethod+'.pdf'  # 読み込むファイル名  # 保存する画像のファイル名
                            save_plot(filename, output_filename,xlim,num,i)
                            filename = []