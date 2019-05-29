./Release/ppm.exe init test1
cd ./test1/
premake5 vs2017
msbuild
cd ..
./Release/ppm.exe init lib test2
cd ./test2/
premake5 vs2017
msbuild
cd ..
Remove-Item ./test1/ -Recurse -Confirm:$False
Remove-Item ./test2/ -Recurse -Confirm:$False