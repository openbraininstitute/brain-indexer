#! /usr/bin/env bash

VERSION=v3.10.5
URL=https://raw.githubusercontent.com/nlohmann/json/${VERSION}/single_include/nlohmann/json.hpp
curl ${URL} -o include/nlohmann/json.hpp
