@echo off

echo Launching script with administrator privileges...
powershell -command "   Start-Process PowerShell -Verb RunAs \""-Command `\""cd '%cd%'; & '.\CreateElevatedTask.ps1';`\""\""   "
