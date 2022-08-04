ARG MANYLINUX_IMAGE=manylinux2014_x86_64

FROM quay.io/pypa/$MANYLINUX_IMAGE
LABEL authors="Fernando Pereira"

RUN yum -y install wget \
  && wget https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.gz \
  && tar -xzf boost_1_79_0.tar.gz \
  && pushd boost_1_79_0 \
  && ./bootstrap.sh \
  && ./b2 install --with-test --with-serialization --with-filesystem --with-headers --with-system --with-timer \
  && popd \
  && rm -rf boost_1_79_0 boost_1_79_0.tar.gz

