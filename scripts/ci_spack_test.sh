set -exo pipefail

# Setup spack
[ -d spack ] || git clone https://github.com/BlueBrain/spack.git --depth=1 -b ${SPACK_BRANCH:-develop}

mkdir -p fake_home
export HOME=${PWD}/fake_home
. spack/share/spack/setup-env.sh
mkdir -p ~/.spack

cp /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/*.yaml spack/etc/spack/
cat << EOF > "spack/etc/spack/upstreams.yaml"
upstreams:
  applications:
    install_tree: /gpfs/bbp.cscs.ch/ssd/apps/hpc/jenkins/deploy/applications/latest
    modules:
      tcl: /gpfs/bbp.cscs.ch/ssd/apps/hpc/jenkins/deploy/applications/latest/modules
  libraries:
    install_tree: /gpfs/bbp.cscs.ch/ssd/apps/hpc/jenkins/deploy/libraries/latest
    modules:
      tcl: /gpfs/bbp.cscs.ch/ssd/apps/hpc/jenkins/deploy/libraries/latest/modules
EOF

spack dev-build --test=root spatial-index@develop
