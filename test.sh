# websocketd --ssl --sslkey certs/server.key --sslcert certs/server_cert.pem --port 8080 -- sh -c 'stdbuf -i0 -o0 ./test.sh'
echo '{"Greeter":"HELO"}'
while read line; do
  echo '{"response":'"$line"'}'
done
echo '{"Greeter":"BYE"}'
