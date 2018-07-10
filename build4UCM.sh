#########################################################################
# File Name: build4UCM.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2018年07月10日 星期二 09时37分28秒
#########################################################################
#!/bin/bash

export CUR_PATH=$(cd `dirname $0`; pwd)

export INSTALL_DIR="/opt/usr"

export CFLAGS="-I${INSTALL_DIR}/include -g -fPIC -DGRANDSTREAM_NETWORKS"
export CXXFLAGS="-I${INSTALL_DIR}/include -g -fPIC -DGRANDSTREAM_NETWORKS"
export CPPFLAGS="-I${INSTALL_DIR}/include -g -fPIC -DGRANDSTREAM_NETWORKS"
export LDFLAGS="-L${INSTALL_DIR}/lib"

./autogen.sh

./configure                 \
    --prefix=${INSTALL_DIR} \
    --enable-tests

