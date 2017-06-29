# linux_server
この自作HTTPサーバーはRASPBERRY PI 3 MODEL Bで実行させ、http://seyama.se をホストします。

ホームサーバーなので、WANipもない状況でngrokを使ってホーストします。（ https://ngrok.com/ ） <br />
DNSサーバーはfreedns( http://freedns.afraid.org )を使って、ドメイン名はloopia（ https://www.loopia.se/ ）から借りています。

## webサイトについて ##

Portfolioとして使っているつもりです。


## サーバーについて ##

メインプロセスはメモリーを漏らず、OS側APIのDLLとプロトコルごとのDLLをロード、リロードするだけです。　こうしますと、サーバーを更新する時に再起動しなくてもアップデートできます。

OS側APIのunix_api.cppはそれぞれのOSから求めてる機能を定義しています。

サーバーはseyama.seにくる全てのTCPメッセージに当てるプロトコルモジュールを見つけて、そのモジュールに1MBを与えてフォークします。TCP接続が終わるとそのプロセスと与えられたメモリーも自動的にOSに解放される。

例えばgithub_webhook.cppはあるHTTPプロトコルモジュールで、"POST /github-push-event"という形のメッセージだけを受け取って、本repositoryが更新する時にそれをダウンロードして、サーバーの新しいコードを再コンパイルします。
