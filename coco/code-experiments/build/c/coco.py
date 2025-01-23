import cocopp
from cocopp import genericsettings

# 横軸の最大値を変更
genericsettings.xlimit_pprldmany = 1e4
# 手法ごとの結果フォルダを指定して比較
#cocopp.main(['./exdata/B-DE','./exdata/U-B-DE','./exdata/U2-B-DE'])
cocopp.main(['./exdata/B-DE','./exdata/U-B-DE'])