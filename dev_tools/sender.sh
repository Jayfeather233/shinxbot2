#!/bin/bash

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" || exit ; pwd -P )
cd "$PARENT_PATH" || exit
PORT_FILE="../config/port.txt"
PORT=0

read -r -d '' CURL_TEMPLATE << EOM
{
    "time": %s,
    "self_id": 23333,
    "post_type": "%s",
    "message_type": "private",
    "sub_type": "friend",
    "message_id": 12,
    "user_id": 31415926,
    "message": "%s",
    "raw_message": "TODO",
    "font": 456,
    "sender": {
        "nickname": "Sender",
        "sex": "woman",
        "age": 13
    }
}
EOM

get_port() {
  if ! [ -f $PORT_FILE ]; then
    echo "Error:" $PORT_FILE "not exists. Please run \`cq_bot\` first to generate the file first"
    exit 1
  fi
  content=$(cat $PORT_FILE)
  PORT=($content)
  PORT=${PORT[1]}
  echo "Sending to port:" "$PORT"
}

send_msg() {
  printf -v curl_post "$CURL_TEMPLATE" \
    "$(date +%s)"\
    "message"\
    $1
  curl localhost:"$PORT" -X POST -H "Content-Type:application/json\r\nX-Self-ID: 23333" -d "$curl_post"
}

main() {
  get_port
  send_msg $1
  echo "Send:" $1
}

main $1
