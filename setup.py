import os
import re
import sys
import subprocess
from distutils.version import LooseVersion
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir='', cmake_opts=None):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)
        self.cmake_opts = cmake_opts or []


class CMakeBuild(build_ext):
    def run(self):
        cmake = self._find_cmake()
        for ext in self.extensions:
            self.build_extension(ext, cmake)

    def build_extension(self, ext, cmake):
        self.outdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        print("Building lib to:", self.outdir)
        print("Python interpreter: ", sys.executable)
        cmake_args = [
            '-DEXTENSION_OUTPUT_DIRECTORY=' + self.outdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable,
        ] + ext.cmake_opts

        cfg = 'Debug' if self.debug else 'Release'
        cmake_args += [
            '-DCMAKE_BUILD_TYPE=' + cfg,
        ]
        build_args = ['--config', cfg, '--', '-j4']

        env = os.environ.copy()
        env['CXXFLAGS'] = "{}".format(env.get('CXXFLAGS', ''))
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        try:
            subprocess.Popen(
                "echo $CXX", shell=True, stdout=subprocess.PIPE)
            subprocess.check_call([cmake, ext.sourcedir] + cmake_args,
                                  cwd=self.build_temp, env=env)
            subprocess.check_call([cmake, '--build', '.'] + build_args,
                                  cwd=self.build_temp)
        except subprocess.CalledProcessError as exc:
            print("Status : FAIL", exc.returncode, exc.output)
            raise

    @staticmethod
    def _find_cmake():
        for candidate in ['cmake', 'cmake3']:
            try:
                out = subprocess.check_output([candidate, '--version'])
                cmake_version = LooseVersion(
                    re.search(r'version\s*([\d.]+)', out.decode()).group(1))
                if cmake_version >= '3.5.0':
                    return candidate
            except OSError:
                pass

        raise RuntimeError("Project requires CMake >=3.5.0")


package_info = dict(
    name="spatial-index",
    python_requires='>=3.8.0',
    use_scm_version=True,
    packages=["spatial_index"],
    ext_modules=[CMakeExtension(
        'spatial_index._spatial_index',
        cmake_opts=[
            '-DSI_UNIT_TESTS=OFF',
            '-DSI_MPI={}'.format(
                os.environ["SI_MPI"] if "SI_MPI" in os.environ else "On"
            ),
        ]
    )],
    entry_points=dict(console_scripts=[
        'spatial-index-nodes=spatial_index.commands:spatial_index_nodes',
        'spatial-index-synapses=spatial_index.commands:spatial_index_synapses',
        'spatial-index-circuit=spatial_index.commands:spatial_index_circuit',
        'spatial-index-compare=spatial_index.commands:spatial_index_compare',
    ]),
    cmdclass=dict(build_ext=CMakeBuild),
    include_package_data=True,
    install_requires=[
        "numpy>=1.13.1",
        "numpy-quaternion",
        "libsonata",
        "morphio",
        "docopt",
        "tqdm",
    ],
    extras_require={
        "mpi": [
            "mpi4py"
        ]
    },
    tests_require=["pytest", "pytest-mpi", ],
    setup_requires=(["setuptools_scm", "pytest-runner" if "test" in sys.argv else ""])
)


if __name__ == "__main__":
    setup(**package_info)
