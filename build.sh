set -x

g++ main.cpp -o app.out -std=c++17 -Wall -Wextra -Wpedantic -g3 -lpcap $@
#sudo setcap cap_net_bind_service,cap_net_raw+ep `realpath app.out`