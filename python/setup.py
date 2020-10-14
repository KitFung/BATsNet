import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="lamppost",
    version="0.0.1",
    author="KitFung",
    author_email="sa9510@gmail.com",
    description="The python api for the lamppost project",
    long_description=long_description,
    long_description_content_type="text/markdown",
    packages=['lamppost'],
    package_data={'lamppost': [
        'service_discovery_py.cpython-38-x86_64-linux-gnu.so',
        'transport_py.cpython-38-x86_64-linux-gnu.so'
    ]},
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)
