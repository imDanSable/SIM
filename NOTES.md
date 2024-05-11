

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
# Release
- Run the ```colorconvert``` and ```screenshots``` tasks on the dev vscode.

- Terminal to a fresh  .....Rack/plugins and clone: ```git clone https://github.com/imDanSable/SIM```

- Using the privateSIM terminal run: ```tools/sync2release.sh -vd ./ release_repo_dir``` and check the output carefully. If all looks good, remove ```-vd``` and run for real.



- Open the release_repo with vscode and manually check and modify the following files if needed. Pay attention to the version
    - ./Makefile
    - ./plugin.json
    - src/config.hpp

- Run make -j16 to see if all is in order

- Run the toolchain for all including analyze:
```
cd ~/dev/rack-plugin-toolchain
make -j16 plugin-analyze PLUGIN_DIR=~/dev/ReleaseRack/plugins/SIM
make -j16 plugin-build PLUGIN_DIR=~/dev/Rack251/plugins/SIM
ls -lhia plugin-build/
```

Copy to lin and win and test. Check log file errors afterwards.
```
cp plugin-build/SIM-2.<ver>-win-x64.vcvplugin /home/b/windocs/Rack2/plugins-win-x64/

cp plugin-build/SIM-2.<ver>-lin-x64.vcvplugin /home/b/.Rack2/plugins-lin-x64/
```

Stage and commit all the changes using vscode or command line. Create a tag for the version and push:

```
eval "$(ssh-agent -s)" ssh-add ~/.ssh/id_rsa

git remote set-url origin git@github.com:imDanSable/SIM.git

git tag v2.1.1
git push origin --tags
git push origin




