inout
=====

Author: Kazuki Komatsu

入出力を扱うモジュールです。
サーバーへのHTTP GETやPOSTは、シェルからcURLを叩くことで行っています。

使い方は、`example/test.cpp`を見てください。

+ requirements
    - C++11(14)が使えるC++コンパイラ
    - boost::optionalが使えるなるべく新しいboost
    - cURL

### boost導入方法

各自の環境でググってください。
Windowsでも結構簡単です。


### cURL導入方法

* UbuntuとかLinuxとかなら`sudo apt-get install curl`で入ると思います。
* Windowsの場合は、[ここから](http://curl.haxx.se/download.html)インストーラが落とせます。
* 他の環境の人はググってください
