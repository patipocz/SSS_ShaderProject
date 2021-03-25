# BlankNGL

![alt tag](http://nccastaff.bournemouth.ac.uk/jmacey/GraphicsLib/Demos/BlankNGL.png)

This is a project based on SimpleNGL.

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


// You might have to add to your paths:
-DCMAKE_TOOLCHAIN_FILE = your vcpkg.cmake path
-DVCPKG_TARGET_TRIPLET=x64-windows
-DCMAKE_PREFIX_PATH=C:..Your NGL Installation Path.../NGL; /.....Your Qt Path...../Qt/5.9.9/msvc2017_64