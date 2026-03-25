@echo off
rmdir /s /q build
cmake -B build
cmake --build build --config Release
