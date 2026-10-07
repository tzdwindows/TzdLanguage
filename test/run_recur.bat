@echo off
cd /d "%~dp0"
x64\Release\TzdTools.exe "import \"C:/Users/tzdwindows 7/source/repos/TzdTools/test_recur.tzd\";" 1>rr_out.txt 2>rr_err.txt
exit /b %errorlevel%
