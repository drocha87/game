@echo off
md build 2>nul
cmake -S . -B build
cmake --build build
