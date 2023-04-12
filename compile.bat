@echo off
set root=%1
if "%1"=="" set root=.

g++ %root%\server.cpp -std=c++11 -Wall -pedantic -g3 -opt3 -lws2_32 -o %root%\server.exe
