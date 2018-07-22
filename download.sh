#!/bin/bash

set -eux

echo -e "\e[31mdownloading to shared directory\e[m"
echo -e "\e[31msee shared directory after execution\e[m"

gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/problemsL shared
gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/dfltTracesL shared
gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/problemsF shared
gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/dfltTracesF shared

echo -e "\e[31m download complete! \e[m"
echo -e "\e[31m see shared directory! \e[m"
