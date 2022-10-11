#! /usr/bin/env bash

set -e

boost_dir="boost_1_79_0"
boost_zip_name="boost_1_79_0.tar.gz"
boost_url=https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/${boost_zip_name}

if [[ ! -f "${boost_zip_name}" ]]
then
    wget ${boost_url}
fi

tar -xzf ${boost_zip_name}
pushd ${boost_dir}
./bootstrap.sh
./b2 install --with-test --with-serialization --with-filesystem --with-headers --with-system --with-timer
popd
rm -rf "${boost_dir}" "${boost_zip_name}"
