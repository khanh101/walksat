import os

import setuptools
from Cython import Distutils

MODULE = "walksat"
AUTHOR = "khanh"

if __name__ == "__main__":
    setuptools.setup(
        install_requires=["cython", ],
        packages=[MODULE, ],
        zip_safe=False,
        name=MODULE,
        author=AUTHOR,
        cmdclass={"build_ext": Distutils.build_ext},
        ext_modules=[
            setuptools.Extension(
                name=f"{MODULE}.wrapper",
                sources=[
                    os.path.join(MODULE, "src/walksat.cpp"),
                    os.path.join(MODULE, "wrapper.pyx")
                ],
                language="c++",
                libraries=[],
                include_dirs=[
                    os.path.join(MODULE, "inc"),
                ],
                extra_compile_args=[
                    "-O3",
                ],
            )
        ],

    )