@echo off
cd /d %~dp0
if exist release rmdir /s /q release
mkdir release
copy build-release\Remont_Uchet.exe release\
copy build-release\*.dll release\
xcopy /E /I /Y resources release\resources
copy README.md release\
powershell Compress-Archive -Path release\* -DestinationPath Remont_Uchet.zip -Force
echo Готово: Remont_Uchet.zip
