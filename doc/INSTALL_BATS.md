## Step 1: Install the BATS stuff, IP and Network

Please contact the related IT support for this step

## Step 2: Install the Docker

https://docs.docker.com/engine/install/

## Step 3: Setup the etcd

Go to the `bat_side/etcd_config`, select the corresponding file.
`bash ./xxx_start.sh`

## Step 3: Setup the rss

`bash bat_side/rss.sh`

## Step 4: Setup the query service

`nohup python3 bat_side/qip.py &`
