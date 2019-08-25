# Real-time SPH
This implementation features real-time simulation of fluid and solids, including two-way coupling[4]. Fluids solvers implemetented are:

 - WCSPH [1][2]
 - PCISPH [3]

This project is part of my master thesis.

Here is a sample: https://www.youtube.com/watch?v=_64g8nXxZ-0

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

Here is a list of this project dependencies that must be installed before building it

 - Bullet Physics (>= v2.85.1)
 - Qt (>= v5.7)
 - OpenCL v1.2
 - clogs v1.5.1

Also, the following packages and tools are needed

 - autoconf
 - make
 - git
 - g++


**Important**: this project runs on **Linux**. I used Mint 18.1, but there should be no problem setting up the project with any ubuntu-like distro.

#### Bullet physics v2.85.1 (or higher)
You can download Bullet Physics from its official download site

 - 2.85.1: https://github.com/bulletphysics/bullet3/archive/2.85.1.tar.gz

If you want to use a higher version, do it, probably it will work. Once you have downloaded the tarball, extract it to any directory you want. You should have a directory tree like this

```
bullet3-2.85.1/
├── appveyor.yml
├── AUTHORS.txt
├── bin
├── build3
├── build_and_run_cmake.sh
├── build_and_run_premake.sh
├── build_visual_studio.bat
├── build_visual_studio_vr_pybullet_double.bat
├── BulletConfig.cmake.in
├── bullet.pc.cmake
├── CMakeLists.txt
├── data
├── docs
├── Doxyfile
├── examples
├── Extras
├── LICENSE.txt
├── README.md
├── src
├── test
├── UseBullet.cmake
├── VERSION
└── xcode.command
```
#### QT v5.7 (or higher)
Download QT version 5.7 or higher from its official site

https://www.qt.io/download-qt-for-application-development

Choose *open source* package (unless you want to buy Qt for commercial usage)

Follow the installation steps, and after the installation, you should have a directory similar to this (the root name may change)

```
Qt/
├── 5.7
├── components.xml
├── dist
├── Docs
├── Examples
├── InstallationLog.txt
├── Licenses
├── MaintenanceTool
├── MaintenanceTool.dat
├── MaintenanceTool.ini
├── network.xml
├── QtIcon.png
├── Tools
└── update.rcc
```

#### OpenCL v1.2 (NVIDIA)
If you are using NVIDIA hardware, then, in order to be able to compile the project, you need CUDA 7.5. The installer can be fetched from

https://developer.nvidia.com/cuda-75-downloads-archive

Also, you will need NVIDIA GPU drivers version 352.63 (or higher). I have used that version for development and testing. That version ca be fetched from 

http://www.nvidia.com/content/DriverDownload-March2009/confirmation.php?&url=/XFree86/Linux-x86_64/352.63/NVIDIA-Linux-x86_64-352.63.run&lang=us&type=geforce&agreed=no

#### clogs v1.5.1
Download clogs package from

https://sourceforge.net/projects/clogs/files/1.5.1/clogs-1.5.1.tar.bz2/download

Once downloaded, unpack it, and follow the README instructions to install it.

### Building project

 1. Clone the project into any directory. You should have a tree like this

```
.
├── config.pci
├── configure.ac
├── config.wc
├── data
├── external
├── filters
├── fluid
├── gui
├── kernels
├── main.cpp
├── Makefile.in
├── math
├── mesh
├── opencl
├── opengl
├── runtimeexception.h
├── scene
├── settings
├── shaders
└── textures
```

 2. Run ``autoconf`` command. This will generate a ``configure`` script.

```
$autoconf
```

 3. Run the ``configure`` script, with the following options (replacing the paths for your particular paths)

```
$./configure --with-qt-install-path=/home/santiago/Qt/5.7/gcc_64/ --with-bt-install-path=/home/santiago/bullet3-2.85.1/ --with-cl-include-path=/usr/local/cuda/include
```

 4. If the ``configure`` script worked, then it generated a ``Makefile`` file. To build the project, run

```
$make
```

## Usage
The project executable is named ``sph``. Run the following command to get some help

```
./sph --help
Option ‘scene’ is required but not present
SPH fluid simulation
Usage:
  sph [OPTION...]

      --help                   Print help
  -s, --scene arg              Scene file path
  -c, --config arg             Config file path
  -o, --perfomance_output arg  Fps performance output file path

```

The program options are

 - ``-s`` ( or ``--scene``): the path to a JSON file describing the scene to simulate
 - ``-c`` ( or ``--config``): the path to a config file where all the initial parameters of the simulation are set
 - ``-o`` (or ``--perfomance_output``): the path to an output file where the FPS will be output. This is useful dump the performance of the application to a file.
 

### Scene format
A scene file looks like this

```json
{
    "lights": [
        {
            "ambient_color": [100, 100, 100],
            "diffuse_color": [255, 255, 255],
            "specular_color": [255, 255, 255],
            "direction": [-1.0, -1.0, -1.0]
        }
    ],
    "fluid_volumes": [
        {
            "type": "box",
            "name": "fluid_1",
            "size": [0.5, 1.0, 0.7],
            "center": [0.74, 0.0, -0.1]
        },
        {
            "type": "box",
            "name": "fluid_2",
            "size": [0.5, 1.0, 0.7],
            "center": [-0.74, 0.0, 0.1]
        }
    ],
    "container": {
        "width": 2.0,
        "height": 1.0,
        "depth": 1.0
    },
    "rigid_bodies": [
        {
            "type": "cube",
            "name": "cube_1",
            "size": 0.3,
            "mass": 12.0,
            "center": [0.0, -0.35, 0.0],
            "rotation": [0, 1, 0, 1]
        },
        {
            "type": "model",
            "name": "bunny",
            "obj_filename": "data/models/bunny.obj",
            "mass": 10.0,
            "center": [-0.10, -0.4, 0.1],
            "rotation": [0, 1, 0, 0.5]
        }
        {
            "type": "sphere",
            "name": "sphere_1",
            "size": 0.1,
            "mass": 0.2,
            "center": [0.0, 0.1, 0.0]
        }
    ]
}
```

The keys are
 - ``lights``: a list of objects that define the scene directional lights.
 - ``fluid_volumes``: a list of objects describing the fluid masses present at the scene. Each object describes the initial shape of the fluid mass. Currently, the only supported shape is ``box``.
 - ``container``: the size of the box-shaped container of the whole simulation.
 - ``rigid_bodies``: a list of objects, each describing a rigid body in the scene. The possible bodies available are: ``sphere``, ``cube``, ``wall`` and ``model``.

### Config file
Here is a sample of the configuration file

```
# Simulation settings
time_step=0.005
max_vel=20.0
fluid_particle_radius=0.008
fluid_support_radius=0.032
boundary_particle_radius=0.008
boundary_support_radius=0.032
method=pcisph

# Physics settings
rest_density=1000.0
k_viscosity=1.7
gravity=9.8
gas_stiffness=500
surface_tension=1.0

#Graphics settings
render_method=particles
```

 - The ``render_method`` can have two possible values: ``particles`` or ``screenspace``.
 - The method key defines the solver to use. Possible values are ``pcisph`` or ``wcsph``. If ``pcisph`` is used, then the ``gas_stiffness`` is ignored.


## References
 - [1] *Particle-based fluid simulation for interactive applications*, Matthias Müller, David Charypar and Markus Gross.
 - [2] *Weakly compressible SPH for free surface flows*, Markus Becker and Matthias Teschner
 - [3] *Predictive-Corrective Incompressible SPH*, Barbara Solenthaler and R. Pajarola.
 - [4] *Versatile Rigid-Fluid Coupling for Incompressible SPH*, Nadir Akinci, Markus Ihmsen, Gizem Akinci, Barbara Solenthaler and Matthias Teschner.