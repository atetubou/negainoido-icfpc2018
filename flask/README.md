

# init
最初にmysqlでデータベースとユーザを作成しておく

```
cat secret.sh <<<EOF
export DBNAME=[作成したデータベースの名前
export DBUSER=[作成したユーザの名前]
export DBPASS=[作成したユーザのパスワード]
export FLASK_ENV=production
EOF
./deploy.sh
```

# local server 起動
```
./local_server.sh
```
localhost:5000にサーバが立ちます。

# test
以下のコマンドを打つとtests以下のテストが走ります。
```
./run_test.sh
```
テストに失敗するとデバッグ用のシェルが起動します。
その際localhost:8080にデバッグ用サーバが起動しています。
masterにpushするときは必ずテストを通してください。


