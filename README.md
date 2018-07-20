# negainoido icfpc 2018

## One time setup

TODO(tikuta): write here

## Build and Test

`bazel test //...`

下はおまけなので無視してください

`gcloud container builds submit --config=cloudbuild.yaml .`



## Pull Request

このリポジトリにmasterではないブランチを作って、そこからmasterにpull request送るとCIが走ってくれます。

## Cloud storage usagge

### File copy

```
$ gsutil cp nanika gs://negainoido-icfpc2018-shared-bucket
```

```
$ gsutil cp gs://negainoido-icfpc2018-shared-bucket/nanika .
```

### File ls

```
$ gsutil ls gs://negainoido-icfpc2018-shared-bucket
```
