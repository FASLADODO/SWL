@echo off
setlocal

rem Tests run in files ending with '_test'.
rem python -m unittest discover -p "*_test.py"
python -m unittest discover -p "*_test.py" -v

endlocal
echo on
