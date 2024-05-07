

sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa -y
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-13 g++-13
sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 10 --slave /usr/bin/g++ g++ /usr/bin/g++-13
g++ --version

wget https://apt.llvm.org/llvm.sh
sudo ./llvm.sh 19

sudo update-alternatives --remove-all clang
sudo update-alternatives --remove-all clang++
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100
=================

git clone https://github.com/brendangregg/FlameGraph.git

sudo apt-get install linux-tools-<kernel-version>-generic linux-tools-generic
echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
sudo sysctl -p

=================

sudo apt-get install linux-tools-common linux-tools-generic linux-tools-<kernel-version>-generic linux-cloud-tools-<kernel-version>-generic

echo 'kernel.perf_event_paranoid=1' | sudo tee /etc/sysctl.d/99-perf.conf
sudo sysctl --system

=================

eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_rsa


=================

- Run the ```colorconvert``` and ```screenshots``` tasks on the dev vscode.

- Terminal to a fresh  .....Rack/plugins and clone: ```git clone https://github.com/imDanSable/SIM```

- Using the privateSIM terminal run: ```tools/sync2release.sh -vd ./ release_repo_dir``` and check the outpout carefully. If all looks good, remove ```-vd``` and run for real.


- Open the release_repo with vscode and manually check and modify the following files if needed. Pay attention to the version
    - Makefile
    - plugin.json

- Run make -j16 to see if all is in order


