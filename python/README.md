# Lamppost

A python library that provide a interface for user to receive data and interact to our system. The python binding shared object of each module in our system will be coped into here.

To let the interface more friendly, add python wrapping.

### How to Install

```
sudo apt install -y python3-pip
sudo pip3 install setuptools
bash prepare.sh
python3 setup.py build
sudo pip3 install --upgrade cython
pip3 install --user .
```
