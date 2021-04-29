This is a project that was trying to recreate Subsurface Scattering Shaders, mainly effect like wax or Jade for Real Time applications. It is based on NGL Demos by Jon Macey





# BlankNGL

![alt tag](http://nccastaff.bournemouth.ac.uk/jmacey/GraphicsLib/Demos/BlankNGL.png)

This is an empty boilerplate framework for NGL projects, it creates an empty window and draws nothing.

To build you need NGL installed as per the instructions [here](https://github.com/NCCA/NGL)

Windows

```
mkdir build
cd build
cmake --build -DCMAKE_PREFIX_PATH=~/NGL ..
cmake --build .

```

Mac Linux


```
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=~/NGL ..
make 
```
