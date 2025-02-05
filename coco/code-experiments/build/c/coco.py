import cocopp
from cocopp import genericsettings

# 横軸の最大値を変更
genericsettings.xlimit_pprldmany = 1e4
# 手法ごとの結果フォルダを指定して比較
cocopp.main(['./exdata/L','./exdata/U-L'])
#cocopp.main(['./exdata/B','./exdata/U-B'])